#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED

#include "../model/model.h"

typedef struct {
    size_t num_programs;
    program_t programs[NUM_PROGRAMS];
} press_program_list_t;

int storage_load_programs(const char *path, press_program_list_t *pmodel);
int storage_update_program_index(const char *path, name_t *names, size_t len);
int storage_update_program(const char *path, program_t *p);

char  *storage_read_file(char *name);
void   storage_create_dir(char *name);
size_t storage_get_file_size(const char *path);
void   storage_clear_file(const char *path);
char   storage_is_drive_plugged(void);
int storage_mount_drive(void);
void storage_unmount_drive(void);
int    storage_is_file(const char *path);
int    storage_update_temporary_firmware(char *app_path, char *temporary_path);

#endif
