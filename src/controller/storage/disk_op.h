#ifndef DISK_OP_H_INCLUDED
#define DISK_OP_H_INCLUDED


#include "model/model.h"


typedef void (*disk_op_callback_t)(uint8_t error, void *data, void *arg);


typedef enum {
    DISK_OP_RESPONSE_TAG_ERROR,
    DISK_OP_RESPONSE_TAG_CONFIGURATION_LOADED,
    DISK_OP_RESPONSE_TAG_CONFIGURATION_EXPORTED,
} disk_op_response_tag_t;


typedef struct {
    disk_op_response_tag_t tag;
    union {
        struct {
            configuration_t *config;
        } configuration_loaded;
    } as;
} disk_op_response_t;


void    disk_op_init(void);
void    disk_op_load_config(void);
void    disk_op_save_config(const configuration_t *config);
uint8_t disk_op_get_response(disk_op_response_t *response);
void    disk_op_save_wifi_config(void);
void    disk_op_read_file(void);
int     disk_op_is_drive_mounted(void);
int     disk_op_is_firmware_present(void);
void    disk_op_firmware_update(void);
void    disk_op_export_config(const char *name);
void    disk_op_update_importable_configurations(mut_model_t *model);

#endif
