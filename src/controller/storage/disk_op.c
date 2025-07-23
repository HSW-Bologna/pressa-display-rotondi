#include <dirent.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "services/socketq.h"
#include "disk_op.h"
#include "storage.h"
#include "config/app_config.h"
#include "adapters/network/network.h"
#include "log.h"
#include "model/model.h"


#define REQUEST_SOCKET_PATH  "/tmp/.application_disk_request_socket"
#define RESPONSE_SOCKET_PATH "/tmp/.application_disk_response_socket"
#define MOUNT_ATTEMPTS       5
#define APP_UPDATE           "/tmp/mnt/pressa-display-rotondi"

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

static void *disk_interaction_task(void *args);
static void  simple_request(int code);


static socketq_t       requestq;
static socketq_t       responseq;
static pthread_mutex_t sem;
static int             drive_mounted = 0;


void disk_op_init(void) {
    int res1 = socketq_init(&requestq, REQUEST_SOCKET_PATH, sizeof(task_request_t));
    int res2 = socketq_init(&responseq, RESPONSE_SOCKET_PATH, sizeof(task_response_t));
    assert(res1 == 0 && res2 == 0);

    assert(pthread_mutex_init(&sem, NULL) == 0);

    pthread_t id;
    pthread_create(&id, NULL, disk_interaction_task, NULL);
    pthread_detach(id);
}


void disk_op_save_config(const configuration_t *config) {
    configuration_t *config_copy = malloc(sizeof(configuration_t));
    assert(config_copy != NULL);
    memcpy(config_copy, config, sizeof(configuration_t));
    task_request_t msg = {
        .tag = DISK_OP_MESSAGE_TAG_SAVE_CONFIG,
        .as  = {.save_config = {.config = config_copy}},
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


void disk_op_export_config(const char *name) {
    const char *name_copy = strdup(name);
    assert(name_copy != NULL);

    task_request_t msg = {
        .tag = DISK_OP_MESSAGE_TAG_EXPORT_CONFIG,
        .as  = {.export_config = {.name = name_copy}},
    };
    socketq_send(&requestq, (uint8_t *)&msg);
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
    socketq_send(&requestq, (uint8_t *)&msg);
}


void disk_op_firmware_update(disk_op_callback_t callback) {
    task_request_t msg = {
        .tag      = DISK_OP_MESSAGE_TAG_FIRMWARE_UPDATE,
        .callback = callback,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


int disk_op_is_drive_mounted(void) {
    pthread_mutex_lock(&sem);
    int res = drive_mounted;
    pthread_mutex_unlock(&sem);
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

    if (socketq_receive_nonblock(&responseq, (uint8_t *)&task_response, 0)) {
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


static void *disk_interaction_task(void *args) {
    (void)args;
    unsigned int mount_attempts = 0;

    storage_create_dir(APP_CONFIG_DATA_PATH);

    for (;;) {
        task_request_t msg;
        if (socketq_receive_nonblock(&requestq, (uint8_t *)&msg, 500)) {
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
                    socketq_send(&responseq, (uint8_t *)&response);
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

                    log_info("Exporting " APP_CONFIG_CONFIGURATION_PATH " to %s", path);
                    response.error = storage_copy_file(path, APP_CONFIG_CONFIGURATION_PATH);
                    free((void *)msg.as.export_config.name);
                    free(path);

                    socketq_send(&responseq, (uint8_t *)&response);
                    break;
                }

                case DISK_OP_MESSAGE_TAG_LOAD_CONFIG:
                    response.payload                                 = 1;
                    response.response.tag                            = DISK_OP_RESPONSE_TAG_CONFIGURATION_LOADED;
                    response.response.as.configuration_loaded.config = malloc(sizeof(configuration_t));
                    assert(response.response.as.configuration_loaded.config);

                    response.error = storage_load_configuration(APP_CONFIG_CONFIGURATION_PATH,
                                                                response.response.as.configuration_loaded.config);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_TAG_SAVE_WIFI_CONFIG:
                    response.error = 0;
                    network_save_config();
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_TAG_FIRMWARE_UPDATE:
                    response.error = storage_update_temporary_firmware(APP_UPDATE, TEMPORARY_APP);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_TAG_FINALIZE_FIRMWARE_UPDATE:
                    response.error = storage_update_final_firmware((char *)(msg.as.finalize_firmware_update.path
                                                                                ? msg.as.finalize_firmware_update.path
                                                                                : "root/app"));
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;
            }
        }

        if (storage_get_file_size(APP_CONFIG_LOGFILE) > MAX_LOGFILE_SIZE) {
            storage_clear_file(APP_CONFIG_LOGFILE);
        }

        int drive_already_mounted = disk_op_is_drive_mounted();
        int drive_plugged         = storage_is_drive_plugged();

        if (drive_already_mounted && !drive_plugged) {
            pthread_mutex_lock(&sem);
            drive_mounted = 0;
            pthread_mutex_unlock(&sem);
            storage_unmount_drive();
            log_info("Chiavetta rimossa");
            mount_attempts = 0;
        } else if (!drive_already_mounted && drive_plugged) {
            if (mount_attempts < MOUNT_ATTEMPTS) {
                mount_attempts++;
                log_info("Rilevata una chiavetta");
                if (storage_mount_drive() == 0) {
                    pthread_mutex_lock(&sem);
                    drive_mounted = 1;
                    // drive_machines_num = storage_list_saved_machines(DRIVE_MOUNT_PATH, &drive_machines);
                    pthread_mutex_unlock(&sem);
                    // model->system.f_update_ready  = is_firmware_present();
                    log_info("Chiavetta montata con successo");
                    mount_attempts = 0;
                } else {
                    pthread_mutex_lock(&sem);
                    drive_mounted = 0;
                    pthread_mutex_unlock(&sem);
                    log_warn("Non sono riuscito a montare la chiavetta!");
                }
            }
        }
    }

    pthread_exit(NULL);
    return NULL;
}


static void simple_request(int code) {
    task_request_t msg = {
        .tag = code,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}
