#include <string.h>
#include <unistd.h>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include "errno.h"
#include "model/model.h"
#include "storage.h"
#include "log.h"
#include "config/app_config.h"
#include "services/serializer.h"


#define PARAMETERS_SERIALIZED_SIZE 4
#define PROGRAM_SERIALIZED_SIZE                                                                                        \
    (PROGRAM_NAME_SIZE + sizeof(program_digital_channel_schedule_t) * PROGRAM_NUM_DIGITAL_CHANNELS +                   \
     PROGRAM_NUM_TIME_UNITS * 2 + 2 + PROGRAM_DAC_LEVELS + PROGRAM_SENSOR_LEVELS)

#define STORAGE_VERSION 0

#define DIR_CHECK(x)                                                                                                   \
    {                                                                                                                  \
        int res = x;                                                                                                   \
        if (res < 0)                                                                                                   \
            log_error("Errore nel maneggiare una cartella: %s", strerror(errno));                                      \
    }


static int is_drive(const char *path);
static int dir_exists(char *name);
static int read_exactly(uint8_t *buffer, size_t length, FILE *fp);
static int write_exactly(const uint8_t *buffer, size_t length, FILE *fp);
static int is_dir(const char *path);


#ifdef BUILD_CONFIG_SIMULATOR
#define remount_ro()
#define remount_rw()
#else
static inline void remount_ro() {
    if (mount("/dev/mmcblk0p3", APP_CONFIG_DATA_PATH, "ext2", MS_REMOUNT | MS_RDONLY, NULL) < 0)
        log_error("Errore nel montare la partizione %s: %s (%i)", APP_CONFIG_DATA_PATH, strerror(errno), errno);
}

static inline void remount_rw() {
    if (mount("/dev/mmcblk0p3", APP_CONFIG_DATA_PATH, "ext2", MS_REMOUNT, NULL) < 0)
        log_error("Errore nel montare la partizione %s: %s (%i)", APP_CONFIG_DATA_PATH, strerror(errno), errno);
}
#endif


int storage_load_configuration(const char *path, configuration_t *config) {
    if (storage_is_file(path)) {
        FILE *fp = fopen(path, "rb");

        if (!fp) {
            log_error("Failed to open file %s: %s", path, strerror(errno));
            return -1;
        }

        uint8_t buffer[PARAMETERS_SERIALIZED_SIZE + 1] = {0};
        if (read_exactly(buffer, sizeof(buffer), fp) < 0) {
            log_error("Failed to read file %s: %s", path, strerror(errno));
            fclose(fp);
            return -1;
        }

        uint16_t parameters_buffer_index = 1;     // Skip the version
        parameters_buffer_index += deserialize_uint16_be(&config->headgap_offset_up, &buffer[parameters_buffer_index]);
        parameters_buffer_index +=
            deserialize_uint16_be(&config->headgap_offset_down, &buffer[parameters_buffer_index]);

        for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
            if (read_exactly((uint8_t *)config->channel_names[i], sizeof(name_t), fp) < 0) {
                log_error("Failed to read file %s: %s", path, strerror(errno));
                fclose(fp);
                return -1;
            }
        }

        for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
            uint8_t  buffer[PROGRAM_SERIALIZED_SIZE] = {0};
            uint16_t programs_buffer_index           = 0;

            if (read_exactly(buffer, sizeof(buffer), fp) < 0) {
                log_error("Failed to read file %s: %s", path, strerror(errno));
                fclose(fp);
                return -1;
            }

            program_t *program = &config->programs[i];
            memcpy(program->name, &buffer[programs_buffer_index], PROGRAM_NAME_SIZE);
            program->name[PROGRAM_NAME_LENGTH] = '\0';
            programs_buffer_index += PROGRAM_NAME_SIZE;

            for (uint16_t j = 0; j < PROGRAM_NUM_DIGITAL_CHANNELS; j++) {
                programs_buffer_index +=
                    deserialize_uint32_be(&program->digital_channels[j], &buffer[programs_buffer_index]);
            }

            for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
                uint8_t value = 0;
                programs_buffer_index += deserialize_uint8(&value, &buffer[programs_buffer_index]);
                program->dac_channel[j] = value;
            }

            for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
                uint8_t value = 0;
                programs_buffer_index += deserialize_uint8(&value, &buffer[programs_buffer_index]);
                program->sensor_channel[j] = value;
            }

            programs_buffer_index +=
                deserialize_uint16_be(&program->time_unit_decisecs, &buffer[programs_buffer_index]);

            for (uint16_t j = 0; j < PROGRAM_DAC_LEVELS; j++) {
                uint8_t value = 0;
                programs_buffer_index += deserialize_uint8(&value, &buffer[programs_buffer_index]);
                program->dac_levels[j] = value;
            }

            for (uint16_t j = 0; j < PROGRAM_SENSOR_LEVELS; j++) {
                uint8_t value = 0;
                programs_buffer_index += deserialize_uint8(&value, &buffer[programs_buffer_index]);
                program->sensor_levels[j] = value;
            }
        }

        fclose(fp);
        return 0;
    } else {
        log_error("No such file: %s", path);
        return -1;
    }
}


int storage_save_configuration(const char *path, const configuration_t *config) {
    FILE *fp = fopen(path, "wb");

    if (!fp) {
        log_error("Failed to open file %s: %s", path, strerror(errno));
        return -1;
    }

    uint8_t buffer[PARAMETERS_SERIALIZED_SIZE + 1] = {0};
    buffer[0]                                      = STORAGE_VERSION;
    uint16_t parameters_buffer_index               = 1;     // Skip the version
    parameters_buffer_index += serialize_uint16_be(&buffer[parameters_buffer_index], config->headgap_offset_up);
    parameters_buffer_index += serialize_uint16_be(&buffer[parameters_buffer_index], config->headgap_offset_down);

    if (write_exactly(buffer, sizeof(buffer), fp) < 0) {
        log_error("Failed to write file %s: %s", path, strerror(errno));
        fclose(fp);
        return -1;
    }

    for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
        if (write_exactly((const uint8_t *)config->channel_names[i], sizeof(name_t), fp) < 0) {
            log_error("Failed to write file %s: %s", path, strerror(errno));
            fclose(fp);
            return -1;
        }
    }

    for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
        uint16_t programs_buffer_index           = 0;
        uint8_t  buffer[PROGRAM_SERIALIZED_SIZE] = {0};

        const program_t *program = &config->programs[i];
        memcpy(&buffer[programs_buffer_index], program->name, PROGRAM_NAME_SIZE);
        programs_buffer_index += PROGRAM_NAME_SIZE;

        for (uint16_t j = 0; j < PROGRAM_NUM_DIGITAL_CHANNELS; j++) {
            programs_buffer_index += serialize_uint32_be(&buffer[programs_buffer_index], program->digital_channels[j]);
        }

        for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
            if (i == 0) {
                log_info("%i %i", j, program->dac_channel[j]);
            }
            programs_buffer_index += serialize_uint8(&buffer[programs_buffer_index], program->dac_channel[j]);
        }

        for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
            programs_buffer_index += serialize_uint8(&buffer[programs_buffer_index], program->sensor_channel[j]);
        }

        programs_buffer_index += serialize_uint16_be(&buffer[programs_buffer_index], program->time_unit_decisecs);

        for (uint16_t j = 0; j < PROGRAM_DAC_LEVELS; j++) {
            programs_buffer_index += serialize_uint8(&buffer[programs_buffer_index], program->dac_levels[j]);
        }

        for (uint16_t j = 0; j < PROGRAM_SENSOR_LEVELS; j++) {
            programs_buffer_index += serialize_uint8(&buffer[programs_buffer_index], program->sensor_levels[j]);
        }

        if (write_exactly(buffer, sizeof(buffer), fp) < 0) {
            log_error("Failed to read file %s: %s", path, strerror(errno));
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
        log_error("Non sono riuscito a leggere il file %s: %s", name, strerror(errno));
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
#ifdef BUILD_CONFIG_SIMULATOR
    return is_dir(APP_CONFIG_DRIVE_MOUNT_PATH);
#else
    char drive[32];
    for (char c = 'a'; c < 'h'; c++) {
        snprintf(drive, sizeof(drive), "/dev/sd%c", c);
        if (is_drive(drive))
            return c;
    }
    return 0;
#endif
}


int storage_mount_drive(void) {
#ifdef BUILD_CONFIG_SIMULATOR
    return 0;
#else
    char path[80];
    char drive[64];

    if (!dir_exists(APP_CONFIG_DRIVE_MOUNT_PATH)) {
        storage_create_dir(APP_CONFIG_DRIVE_MOUNT_PATH);
    }

    char c = storage_is_drive_plugged();
    if (!c) {
        return -1;
    }

    snprintf(drive, sizeof(drive), "/dev/sd%c", c);
    snprintf(path, sizeof(path), "%s1", drive);
    if (!is_drive(path)) {     // Se presente monta la prima partizione
        strcpy(path, drive);
        log_info("Non trovo una partizione; monto %s", path);
    } else {
        log_info("Trovata partizione %s", path);
    }

    if (mount(path, APP_CONFIG_DRIVE_MOUNT_PATH, "vfat", 0, NULL) < 0) {
        if (errno == EBUSY)     // Il file system era gia' montato
            return 0;

        log_error("Errore nel montare il disco %s: %s (%i)", path, strerror(errno), errno);
        return -1;
    }
    return 0;
#endif
}


void storage_unmount_drive(void) {
#ifdef BUILD_CONFIG_SIMULATOR
    return;
#endif
    if (dir_exists(APP_CONFIG_DRIVE_MOUNT_PATH)) {
        if (umount(APP_CONFIG_DRIVE_MOUNT_PATH) < 0) {
            log_warn("Umount error: %s (%i)", strerror(errno), errno);
        }
        rmdir(APP_CONFIG_DRIVE_MOUNT_PATH);
    }
}


int storage_is_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISREG(path_stat.st_mode);
}


int storage_update_temporary_firmware(char *app_path, char *temporary_path) {
    int res = 0;

#ifdef BUILD_CONFIG_SIMULATOR
    return 0;
#endif

    if (storage_is_file(app_path)) {
        if (mount("/dev/root", "/", "ext2", MS_REMOUNT, NULL) < 0) {
            log_error("Errore nel montare la root: %s (%i)", strerror(errno), errno);
            return -1;
        }

        if ((res = storage_copy_file(temporary_path, app_path)) < 0)
            log_error("Non sono riuscito ad aggiornare il firmware");

        if (mount("/dev/root", "/", "ext2", MS_REMOUNT | MS_RDONLY, NULL) < 0)
            log_warn("Errore nel rimontare la root: %s (%i)", strerror(errno), errno);

        sync();
    } else {
        return -1;
    }

    return res;
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


static int is_drive(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISBLK(path_stat.st_mode);
}


static int dir_exists(char *name) {
    DIR *dir = opendir(name);
    if (dir) {
        closedir(dir);
        return 1;
    }
    return 0;
}


void storage_create_dir(char *name) {
    remount_rw();
    DIR_CHECK(mkdir(name, 0766));
    remount_ro();
}


int storage_copy_file(const char *to, const char *from) {
    int     fd_to, fd_from;
    char    buf[4096];
    ssize_t nread;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0) {
        log_warn("Non sono riuscito ad aprire %s: %s", from, strerror(errno));
        return -1;
    }

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_to < 0) {
        log_warn("Non sono riuscito ad aprire %s: %s", to, strerror(errno));
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
