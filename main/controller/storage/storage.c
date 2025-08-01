#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "errno.h"
#include "model/model.h"
#include "storage.h"
#include <esp_log.h>
#include "config/app_config.h"
#include "services/serializer.h"


#define PARAMETERS_SERIALIZED_SIZE 8
#define PROGRAM_SERIALIZED_SIZE                                                                                        \
    (PROGRAM_NAME_SIZE + sizeof(program_digital_channel_schedule_t) * PROGRAM_NUM_CHANNELS +                           \
     PROGRAM_NUM_TIME_UNITS * 2 + 2 + PROGRAM_PRESSURE_LEVELS * 2 + PROGRAM_SENSOR_LEVELS * 2)

#define STORAGE_VERSION 0

#define DIR_CHECK(x)                                                                                                   \
    {                                                                                                                  \
        int res = x;                                                                                                   \
        if (res < 0)                                                                                                   \
            ESP_LOGE(TAG, "Errore nel maneggiare una cartella: %s", strerror(errno));                                  \
    }


static int read_exactly(uint8_t *buffer, size_t length, FILE *fp);
static int write_exactly(const uint8_t *buffer, size_t length, FILE *fp);
static int is_dir(const char *path);


static const char *TAG = __FILE_NAME__;


int storage_load_configuration(const char *path, configuration_t *config) {
    if (storage_is_file(path)) {
        FILE *fp = fopen(path, "rb");

        if (!fp) {
            ESP_LOGE(TAG, "Failed to open file %s: %s", path, strerror(errno));
            return -1;
        }

        uint8_t buffer[PARAMETERS_SERIALIZED_SIZE + 1] = {0};
        if (read_exactly(buffer, sizeof(buffer), fp) < 0) {
            ESP_LOGE(TAG, "Failed to read file %s: %s", path, strerror(errno));
            fclose(fp);
            return -1;
        }

        uint16_t parameters_buffer_index = 1;     // Skip the version
        parameters_buffer_index += deserialize_uint16_be(&config->headgap_offset_up, &buffer[parameters_buffer_index]);
        parameters_buffer_index +=
            deserialize_uint16_be(&config->headgap_offset_down, &buffer[parameters_buffer_index]);
        parameters_buffer_index += deserialize_uint16_be(&config->ma4_20_offset, &buffer[parameters_buffer_index]);
        parameters_buffer_index +=
            deserialize_uint16_be(&config->position_sensor_scale_mm, &buffer[parameters_buffer_index]);

        for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
            if (read_exactly((uint8_t *)config->channel_names[i], sizeof(name_t), fp) < 0) {
                ESP_LOGE(TAG, "Failed to read file %s: %s", path, strerror(errno));
                fclose(fp);
                return -1;
            }
        }

        for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
            uint8_t buffer[PROGRAM_SERIALIZED_SIZE] = {0};

            if (read_exactly(buffer, sizeof(buffer), fp) < 0) {
                ESP_LOGE(TAG, "Failed to read file %s: %s", path, strerror(errno));
                fclose(fp);
                return -1;
            }

            uint16_t   program_buffer_index = 0;
            program_t *program              = &config->programs[i];
            memcpy(program->name, &buffer[program_buffer_index], PROGRAM_NAME_SIZE);
            program->name[PROGRAM_NAME_LENGTH] = '\0';
            program_buffer_index += PROGRAM_NAME_SIZE;

            for (uint16_t j = 0; j < PROGRAM_NUM_PROGRAMMABLE_CHANNELS; j++) {
                program_buffer_index +=
                    deserialize_uint32_be(&program->digital_channels[j], &buffer[program_buffer_index]);
            }

            for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
                uint8_t value = 0;
                program_buffer_index += deserialize_uint8(&value, &buffer[program_buffer_index]);
                program->pressure_channel[j] = value;
            }

            for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
                uint8_t value = 0;
                program_buffer_index += deserialize_uint8(&value, &buffer[program_buffer_index]);
                program->sensor_channel[j] = value;
            }

            program_buffer_index += deserialize_uint16_be(&program->time_unit_decisecs, &buffer[program_buffer_index]);

            for (uint16_t j = 0; j < PROGRAM_PRESSURE_LEVELS; j++) {
                program_buffer_index +=
                    deserialize_uint16_be(&program->pressure_levels[j], &buffer[program_buffer_index]);
            }

            for (uint16_t j = 0; j < PROGRAM_SENSOR_LEVELS; j++) {
                program_buffer_index +=
                    deserialize_uint16_be(&program->position_levels[j], &buffer[program_buffer_index]);
            }
        }

        fclose(fp);
        return 0;
    } else {
        ESP_LOGE(TAG, "No such file: %s", path);
        return -1;
    }
}


int storage_save_configuration(const char *path, const configuration_t *config) {
    FILE *fp = fopen(path, "wb");

    if (!fp) {
        ESP_LOGE(TAG, "Failed to open file %s: %s", path, strerror(errno));
        return -1;
    }

    uint8_t buffer[PARAMETERS_SERIALIZED_SIZE + 1] = {0};
    buffer[0]                                      = STORAGE_VERSION;
    uint16_t parameters_buffer_index               = 1;     // Skip the version
    parameters_buffer_index += serialize_uint16_be(&buffer[parameters_buffer_index], config->headgap_offset_up);
    parameters_buffer_index += serialize_uint16_be(&buffer[parameters_buffer_index], config->headgap_offset_down);
    parameters_buffer_index += serialize_uint16_be(&buffer[parameters_buffer_index], config->ma4_20_offset);
    parameters_buffer_index += serialize_uint16_be(&buffer[parameters_buffer_index], config->position_sensor_scale_mm);

    if (write_exactly(buffer, sizeof(buffer), fp) < 0) {
        ESP_LOGE(TAG, "Failed to write file %s: %s", path, strerror(errno));
        fclose(fp);
        return -1;
    }

    for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
        if (write_exactly((const uint8_t *)config->channel_names[i], sizeof(name_t), fp) < 0) {
            ESP_LOGE(TAG, "Failed to write file %s: %s", path, strerror(errno));
            fclose(fp);
            return -1;
        }
    }

    for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
        uint8_t buffer[PROGRAM_SERIALIZED_SIZE] = {0};

        uint16_t         program_buffer_index = 0;
        const program_t *program              = &config->programs[i];
        memcpy(&buffer[program_buffer_index], program->name, PROGRAM_NAME_SIZE);
        program_buffer_index += PROGRAM_NAME_SIZE;

        for (uint16_t j = 0; j < PROGRAM_NUM_PROGRAMMABLE_CHANNELS; j++) {
            program_buffer_index += serialize_uint32_be(&buffer[program_buffer_index], program->digital_channels[j]);
        }

        for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
            program_buffer_index += serialize_uint8(&buffer[program_buffer_index], program->pressure_channel[j]);
        }

        for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
            program_buffer_index += serialize_uint8(&buffer[program_buffer_index], program->sensor_channel[j]);
        }

        program_buffer_index += serialize_uint16_be(&buffer[program_buffer_index], program->time_unit_decisecs);

        for (uint16_t j = 0; j < PROGRAM_PRESSURE_LEVELS; j++) {
            program_buffer_index += serialize_uint16_be(&buffer[program_buffer_index], program->pressure_levels[j]);
        }

        for (uint16_t j = 0; j < PROGRAM_SENSOR_LEVELS; j++) {
            program_buffer_index += serialize_uint16_be(&buffer[program_buffer_index], program->position_levels[j]);
        }

        if (write_exactly(buffer, sizeof(buffer), fp) < 0) {
            ESP_LOGE(TAG, "Failed to read file %s: %s", path, strerror(errno));
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}


char *storage_read_file(char *name) {
    unsigned long size = 0;
    char         *r    = NULL;
    FILE         *fp   = fopen(name, "r");

    if (fp != NULL) {
        fseek(fp, 0L, SEEK_END);
        size = ftell(fp);
        rewind(fp);

        r = malloc(size);
        fread(r, sizeof(char), size, fp);
        r[size] = 0;
    } else {
        ESP_LOGE(TAG, "Non sono riuscito a leggere il file %s: %s", name, strerror(errno));
    }

    return r;
}


size_t storage_get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        return 0;
    }
    return st.st_size;
}


void storage_clear_file(const char *path) {
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fclose(fp);
    }
}


/*
 *  Chiavetta USB
 */

char storage_is_drive_plugged(void) {
    return is_dir(APP_CONFIG_DRIVE_MOUNT_PATH);
}


int storage_mount_drive(void) {
    return 0;
}


void storage_unmount_drive(void) {}


int storage_is_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISREG(path_stat.st_mode);
}


int storage_update_final_firmware(char *dest) {
    int res = 0;
    ESP_LOGI(TAG, "Aggiornamento del firmare nella locazione %s", dest);

    return res;
}



int storage_update_temporary_firmware(char *app_path, char *temporary_path) {
#ifdef BUILD_CONFIG_SIMULATOR
    (void)app_path;
    (void)temporary_path;
    return 0;
#else
    int res = 0;

    if (storage_is_file(app_path)) {
        ESP_LOGI(TAG, "Update temporary firmware at %s", temporary_path);

        if ((res = storage_copy_file(temporary_path, app_path)) < 0) {
            ESP_LOGE(TAG, "Non sono riuscito ad aggiornare il firmware");
        }
    } else {
        ESP_LOGE(TAG, "Count not find %s", app_path);
        return -1;
    }

    return res;
#endif
}


/*
 *  Static functions
 */


static int read_exactly(uint8_t *buffer, size_t length, FILE *fp) {
    size_t count = 0;
    while (count < length) {
        size_t bytes_read = fread(&buffer[count], length - count, 1, fp);
        if (0 == bytes_read) {
            return -1;
        } else {
            count += bytes_read;
        }
    }
    return count;
}


static int write_exactly(const uint8_t *buffer, size_t length, FILE *fp) {
    size_t count = 0;
    while (count < length) {
        size_t bytes_written = fwrite(&buffer[count], length - count, 1, fp);
        if (0 == bytes_written) {
            return -1;
        } else {
            count += bytes_written;
        }
    }
    return count;
}



static int is_dir(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISDIR(path_stat.st_mode);
}


void storage_create_dir(char *name) {
    DIR_CHECK(mkdir(name, 0766));
}


int storage_copy_file(const char *to, const char *from) {
    int     fd_to, fd_from;
    char    buf[4096];
    ssize_t nread;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0) {
        ESP_LOGW(TAG, "Non sono riuscito ad aprire %s: %s", from, strerror(errno));
        return -1;
    }

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if (fd_to < 0) {
        ESP_LOGW(TAG, "Non sono riuscito ad aprire %s: %s", to, strerror(errno));
        close(fd_from);
        return -1;
    }

    while ((nread = read(fd_from, buf, sizeof buf)) > 0) {
        char   *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else {
                close(fd_from);
                close(fd_to);
                return -1;
            }
        } while (nread > 0);
    }

    close(fd_to);
    close(fd_from);

    return 0;
}
