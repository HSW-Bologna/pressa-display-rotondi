#ifndef DISK_OP_H_INCLUDED
#define DISK_OP_H_INCLUDED


#include "model/model.h"


typedef void (*disk_op_callback_t)(model_t *, void *, void *);
typedef void (*disk_op_error_callback_t)(model_t *, void *);


typedef struct {
    disk_op_callback_t       callback;
    disk_op_error_callback_t error_callback;
    int                      error;
    void                    *data;
    void                    *arg;
    int                      transfer_data;
} disk_op_response_t;


void   disk_op_init(void);
void   disk_op_load_programs(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_save_program_index(model_t *pmodel, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_save_program(program_t *p, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
int    disk_op_manage_response(model_t *pmodel);

void   disk_op_save_wifi_config(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);
void   disk_op_read_file(char *name, disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);

int    disk_op_is_drive_mounted(void);

int    disk_op_is_firmware_present(void);
void   disk_op_firmware_update(disk_op_callback_t cb, disk_op_error_callback_t errcb, void *arg);

#endif
