#if 0
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
#include "../network/wifi.h"
#include "log.h"
#include "model/model.h"


#define REQUEST_SOCKET_PATH  "/tmp/.application_disk_request_socket"
#define RESPONSE_SOCKET_PATH "/tmp/.application_disk_response_socket"
#define MOUNT_ATTEMPTS       5
#define APP_UPDATE           "/tmp/mnt/pressa-display-rotondi.bin"

#ifdef TARGET_DEBUG
#define TEMPORARY_APP "./newapp"
#else
#define TEMPORARY_APP "/root/newapp"
#endif


typedef struct {
    name_t *names;
    size_t  num;
} disk_op_name_list_t;


typedef enum {
    DISK_OP_MESSAGE_CODE_LOAD_PROGRAMS,
    DISK_OP_MESSAGE_CODE_SAVE_PROGRAM,
    DISK_OP_MESSAGE_CODE_SAVE_WIFI_CONFIG,
    DISK_OP_MESSAGE_CODE_FIRMWARE_UPDATE,
} disk_op_message_code_t;


typedef struct {
    disk_op_message_code_t   code;
    disk_op_callback_t       callback;
    disk_op_error_callback_t error_callback;
    void                    *data;
    void                    *arg;
} disk_op_message_t;

static void *disk_interaction_task(void *args);
static void  simple_request(int code, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);


static socketq_t       requestq;
static socketq_t       responseq;
static pthread_mutex_t sem;
static int             drive_mounted = 0;


void disk_op_init(void) {
    int res1 = socketq_init(&requestq, REQUEST_SOCKET_PATH, sizeof(disk_op_message_t));
    int res2 = socketq_init(&responseq, RESPONSE_SOCKET_PATH, sizeof(disk_op_response_t));
    assert(res1 == 0 && res2 == 0);

    assert(pthread_mutex_init(&sem, NULL) == 0);

    pthread_t id;
    pthread_create(&id, NULL, disk_interaction_task, NULL);
    pthread_detach(id);
}


void disk_op_save_program(program_t *p, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    name_t *program_copy = malloc(sizeof(program_t));
    memcpy(program_copy, p, sizeof(program_t));
    assert(program_copy != NULL);
    disk_op_message_t msg = {
        .code           = DISK_OP_MESSAGE_CODE_SAVE_PROGRAM,
        .data           = program_copy,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}


void disk_op_load_programs(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    simple_request(DISK_OP_MESSAGE_CODE_LOAD_PROGRAMS, cb, errcb, arg);
}


void disk_op_save_wifi_config(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    simple_request(DISK_OP_MESSAGE_CODE_SAVE_WIFI_CONFIG, cb, errcb, arg);
}


void disk_op_firmware_update(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    simple_request(DISK_OP_MESSAGE_CODE_FIRMWARE_UPDATE, cb, errcb, arg);
}


int disk_op_is_drive_mounted(void) {
    pthread_mutex_lock(&sem);
    int res = drive_mounted;
    pthread_mutex_unlock(&sem);
    return res;
}


int disk_op_manage_response(model_t *pmodel) {
    disk_op_response_t response = {0};

    if (socketq_receive_nonblock(&responseq, (uint8_t *)&response, 0)) {
        if (response.error) {
            if (response.error_callback != NULL) {
                response.error_callback(pmodel, response.arg);
            }
        } else {
            response.callback(pmodel, response.data, response.arg);
            if (!response.transfer_data) {
                free(response.data);
            }
        }
        return 1;
    } else {
        return 0;
    }
}


int disk_op_is_firmware_present(void) {
    return storage_is_file(APP_UPDATE);
}


static void *disk_interaction_task(void *args) {
    unsigned int mount_attempts = 0;

    storage_create_dir(APP_CONFIG_DATA_PATH);

    for (;;) {
        disk_op_message_t msg;
        if (socketq_receive_nonblock(&requestq, (uint8_t *)&msg, 500)) {
            disk_op_response_t response = {
                .callback       = msg.callback,
                .error_callback = msg.error_callback,
                .data           = NULL,
                .arg            = msg.arg,
                .transfer_data  = 0,
            };

            switch (msg.code) {
                case DISK_OP_MESSAGE_CODE_SAVE_PROGRAM:
                    response.error = storage_update_program(APP_CONFIG_DATA_PATH, msg.data);
                    free(msg.data);
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_LOAD_PROGRAMS:
                    response.data = malloc(sizeof(storage_program_list_t));
                    if (response.data == NULL) {
                        response.error = 1;
                    } else {
                        response.error = storage_load_programs(APP_CONFIG_DATA_PATH, response.data);
                    }
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_SAVE_WIFI_CONFIG:
                    response.error = wifi_save_config();
                    socketq_send(&responseq, (uint8_t *)&response);
                    break;

                case DISK_OP_MESSAGE_CODE_FIRMWARE_UPDATE:
                    response.error = storage_update_temporary_firmware(APP_UPDATE, TEMPORARY_APP);
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


static void simple_request(int code, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg) {
    disk_op_message_t msg = {
        .code           = code,
        .data           = NULL,
        .callback       = cb,
        .error_callback = errcb,
        .arg            = arg,
    };
    socketq_send(&requestq, (uint8_t *)&msg);
}
#endif
