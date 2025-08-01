#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "disk_op.h"
#include "storage.h"
#include "config/app_config.h"
#include "adapters/network/network.h"
#include <esp_log.h>
#include "model/model.h"


#define MOUNT_ATTEMPTS 5
#define APP_UPDATE     "/tmp/mnt/pressa-display-rotondi"

#ifdef BUILD_CONFIG_SIMULATOR
#define TEMPORARY_APP "./newapp"
#else
#define TEMPORARY_APP "/root/newapp"
#endif


typedef struct {
    name_t *names;
    size_t  num;
} disk_op_name_list_t;


typedef struct {
    disk_op_callback_t callback;
    uint8_t            error;
    void              *data;
    void              *arg;

    uint8_t            payload;
    disk_op_response_t response;
} task_response_t;


typedef enum {
    DISK_OP_MESSAGE_TAG_LOAD_CONFIG,
    DISK_OP_MESSAGE_TAG_SAVE_CONFIG,
    DISK_OP_MESSAGE_TAG_EXPORT_CONFIG,
    DISK_OP_MESSAGE_TAG_SAVE_WIFI_CONFIG,
    DISK_OP_MESSAGE_TAG_FIRMWARE_UPDATE,
    DISK_OP_MESSAGE_TAG_FINALIZE_FIRMWARE_UPDATE,
} task_request_tag_t;


typedef struct {
    task_request_tag_t tag;
    disk_op_callback_t callback;
    void              *arg;

    union {
        struct {
            configuration_t *config;
        } save_config;
        struct {
            const char *name;
        } export_config;
        struct {
            const char *path;
        } finalize_firmware_update;
    } as;
} task_request_t;

static void disk_interaction_task(void *args);
static void simple_request(int code);


static QueueHandle_t     requestq;
static QueueHandle_t     responseq;
static SemaphoreHandle_t sem;
static int               drive_mounted = 0;
static const char       *TAG           = __FILE_NAME__;


void disk_op_init(void) {
    {
        static StaticQueue_t static_queue;
        static uint8_t       queue_buffer[sizeof(task_request_t) * 8] = {0};
        requestq = xQueueCreateStatic(sizeof(queue_buffer) / sizeof(task_request_t), sizeof(task_request_t),
                                      queue_buffer, &static_queue);
    }
    {
        static StaticQueue_t static_queue;
        static uint8_t       queue_buffer[sizeof(task_response_t) * 8] = {0};
        responseq = xQueueCreateStatic(sizeof(queue_buffer) / sizeof(task_response_t), sizeof(task_response_t),
                                       queue_buffer, &static_queue);
    }
    {
        static StaticSemaphore_t static_semaphore;
        sem = xSemaphoreCreateMutexStatic(&static_semaphore);
    }

    {
        static StackType_t task_stack[512 * 8] = {0};
#ifdef BUILD_CONFIG_SIMULATED_APP
        xTaskCreate(disk_interaction_task, TAG, sizeof(task_stack), NULL, 5, NULL);
#else
        static StaticTask_t static_task;
        xTaskCreateStatic(disk_interaction_task, TAG, sizeof(task_stack), NULL, 5, task_stack, &static_task);
#endif
    }
}


void disk_op_save_config(const configuration_t *config) {
    configuration_t *config_copy = malloc(sizeof(configuration_t));
    assert(config_copy != NULL);
    memcpy(config_copy, config, sizeof(configuration_t));
    task_request_t msg = {
        .tag = DISK_OP_MESSAGE_TAG_SAVE_CONFIG,
        .as  = {.save_config = {.config = config_copy}},
    };
    xQueueSend(requestq, (uint8_t *)&msg, pdMS_TO_TICKS(10));
}


void disk_op_export_config(const char *name) {
    const char *name_copy = strdup(name);
    assert(name_copy != NULL);

    task_request_t msg = {
        .tag = DISK_OP_MESSAGE_TAG_EXPORT_CONFIG,
        .as  = {.export_config = {.name = name_copy}},
    };
    xQueueSend(requestq, (uint8_t *)&msg, pdMS_TO_TICKS(10));
}


void disk_op_load_config(void) {
    simple_request(DISK_OP_MESSAGE_TAG_LOAD_CONFIG);
}


void disk_op_save_wifi_config(void) {
    simple_request(DISK_OP_MESSAGE_TAG_SAVE_WIFI_CONFIG);
}


void disk_op_finalize_firmware_update(const char *path, disk_op_callback_t callback) {
    task_request_t msg = {
        .tag                              = DISK_OP_MESSAGE_TAG_FINALIZE_FIRMWARE_UPDATE,
        .as.finalize_firmware_update.path = path,
        .callback                         = callback,
    };
    xQueueSend(requestq, (uint8_t *)&msg, pdMS_TO_TICKS(10));
}


void disk_op_firmware_update(disk_op_callback_t callback) {
    task_request_t msg = {
        .tag      = DISK_OP_MESSAGE_TAG_FIRMWARE_UPDATE,
        .callback = callback,
    };
    xQueueSend(requestq, (uint8_t *)&msg, pdMS_TO_TICKS(10));
}


int disk_op_is_drive_mounted(void) {
    xSemaphoreTake(sem, portMAX_DELAY);
    int res = drive_mounted;
    xSemaphoreGive(sem);
    return res;
}


void disk_op_update_importable_configurations(mut_model_t *model) {
    free(model->run.importable_configurations);
    model->run.importable_configurations = 0;
    DIR           *d;
    struct dirent *dir;
    d = opendir(APP_CONFIG_DRIVE_MOUNT_PATH);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                char *extension = strstr(dir->d_name, APP_CONFIG_CONFIGURATION_EXTENSION);
                if (extension && strlen(extension) == strlen(APP_CONFIG_CONFIGURATION_EXTENSION)) {
                    uint16_t i = model->run.num_importable_configurations;

                    model->run.num_importable_configurations++;
                    model->run.importable_configurations =
                        realloc(model->run.importable_configurations,
                                sizeof(char *) * model->run.num_importable_configurations);
                    assert(model->run.importable_configurations);

                    model->run.importable_configurations[i] = strdup(dir->d_name);
                }
            }
        }
        closedir(d);
    }
}


uint8_t disk_op_get_response(disk_op_response_t *response) {
    task_response_t task_response = {0};

    if (xQueueReceive(responseq, (uint8_t *)&task_response, 0)) {
        if (task_response.callback) {
            task_response.callback(task_response.error, task_response.data, task_response.arg);
        }
        if (task_response.error) {
            response->tag = DISK_OP_RESPONSE_TAG_ERROR;
            return 1;
        } else if (task_response.payload) {
            *response = task_response.response;
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}


int disk_op_is_firmware_present(void) {
    return storage_is_file(APP_UPDATE);
}


static void disk_interaction_task(void *args) {
    (void)args;
    unsigned int mount_attempts = 0;

    storage_create_dir(APP_CONFIG_DATA_PATH);

    for (;;) {
        task_request_t msg;
        if (xQueueReceive(requestq, (uint8_t *)&msg, pdMS_TO_TICKS(500))) {
            task_response_t response = {
                .callback = msg.callback,
                .data     = NULL,
                .arg      = msg.arg,
            };

            switch (msg.tag) {
                case DISK_OP_MESSAGE_TAG_SAVE_CONFIG:
                    response.error =
                        storage_save_configuration(APP_CONFIG_CONFIGURATION_PATH, msg.as.save_config.config);
                    free(msg.as.save_config.config);
                    xQueueSend(responseq, (uint8_t *)&response, portMAX_DELAY);
                    break;

                case DISK_OP_MESSAGE_TAG_EXPORT_CONFIG: {
                    response.payload      = 1;
                    response.response.tag = DISK_OP_RESPONSE_TAG_CONFIGURATION_EXPORTED;
                    size_t len            = strlen(APP_CONFIG_DRIVE_MOUNT_PATH) + strlen(msg.as.export_config.name) +
                                 strlen(APP_CONFIG_CONFIGURATION_EXTENSION) + 5;

                    char *path = malloc(len);
                    assert(path);

                    snprintf(path, len, "%s/%s%s", APP_CONFIG_DRIVE_MOUNT_PATH, msg.as.export_config.name,
                             APP_CONFIG_CONFIGURATION_EXTENSION);

                    ESP_LOGI(TAG, "Exporting " APP_CONFIG_CONFIGURATION_PATH " to %s", path);
                    response.error = storage_copy_file(path, APP_CONFIG_CONFIGURATION_PATH);
                    free((void *)msg.as.export_config.name);
                    free(path);

                    xQueueSend(responseq, (uint8_t *)&response, portMAX_DELAY);
                    break;
                }

                case DISK_OP_MESSAGE_TAG_LOAD_CONFIG:
                    response.payload                                 = 1;
                    response.response.tag                            = DISK_OP_RESPONSE_TAG_CONFIGURATION_LOADED;
                    response.response.as.configuration_loaded.config = malloc(sizeof(configuration_t));
                    assert(response.response.as.configuration_loaded.config);

                    response.error = storage_load_configuration(APP_CONFIG_CONFIGURATION_PATH,
                                                                response.response.as.configuration_loaded.config);
                    xQueueSend(responseq, (uint8_t *)&response, portMAX_DELAY);
                    break;

                case DISK_OP_MESSAGE_TAG_SAVE_WIFI_CONFIG:
                    response.error = 0;
                    network_save_config();
                    xQueueSend(responseq, (uint8_t *)&response, portMAX_DELAY);
                    break;

                case DISK_OP_MESSAGE_TAG_FIRMWARE_UPDATE:
                    response.error = storage_update_temporary_firmware(APP_UPDATE, TEMPORARY_APP);
                    xQueueSend(responseq, (uint8_t *)&response, portMAX_DELAY);
                    break;

                case DISK_OP_MESSAGE_TAG_FINALIZE_FIRMWARE_UPDATE:
                    response.error = storage_update_final_firmware((char *)(msg.as.finalize_firmware_update.path
                                                                                ? msg.as.finalize_firmware_update.path
                                                                                : "root/app"));
                    xQueueSend(responseq, (uint8_t *)&response, portMAX_DELAY);
                    break;
            }
        }

        if (storage_get_file_size(APP_CONFIG_LOGFILE) > MAX_LOGFILE_SIZE) {
            storage_clear_file(APP_CONFIG_LOGFILE);
        }

        int drive_already_mounted = disk_op_is_drive_mounted();
        int drive_plugged         = storage_is_drive_plugged();

        if (drive_already_mounted && !drive_plugged) {
            xSemaphoreTake(sem, portMAX_DELAY);
            drive_mounted = 0;
            xSemaphoreGive(sem);
            storage_unmount_drive();
            ESP_LOGI(TAG, "Chiavetta rimossa");
            mount_attempts = 0;
        } else if (!drive_already_mounted && drive_plugged) {
            if (mount_attempts < MOUNT_ATTEMPTS) {
                mount_attempts++;
                ESP_LOGI(TAG, "Rilevata una chiavetta");
                if (storage_mount_drive() == 0) {
                    xSemaphoreTake(sem, portMAX_DELAY);
                    drive_mounted = 1;
                    // drive_machines_num = storage_list_saved_machines(DRIVE_MOUNT_PATH, &drive_machines);
                    xSemaphoreGive(sem);
                    // model->system.f_update_ready  = is_firmware_present();
                    ESP_LOGI(TAG, "Chiavetta montata con successo");
                    mount_attempts = 0;
                } else {
                    xSemaphoreTake(sem, portMAX_DELAY);
                    drive_mounted = 0;
                    xSemaphoreGive(sem);
                    ESP_LOGW(TAG, "Non sono riuscito a montare la chiavetta!");
                }
            }
        }
    }

    vTaskDelete(NULL);
}


static void simple_request(int code) {
    task_request_t msg = {
        .tag = code,
    };
    xQueueSend(requestq, (uint8_t *)&msg, portMAX_DELAY);
}
