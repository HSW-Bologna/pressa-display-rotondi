#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED


#include "model/model.h"


typedef struct {
    size_t    num_programs;
    program_t programs[NUM_PROGRAMS];
} press_program_list_t;


char  *storage_read_file(char *name);
void   storage_create_dir(char *name);
size_t storage_get_file_size(const char *path);
void   storage_clear_file(const char *path);
char   storage_is_drive_plugged(void);
int    storage_mount_drive(void);
void   storage_unmount_drive(void);
int    storage_is_file(const char *path);
int    storage_update_temporary_firmware(char *app_path, char *temporary_path);
int    storage_load_configuration(const char *path, configuration_t *config);
int    storage_save_configuration(const char *path, const configuration_t *config);


#endif
