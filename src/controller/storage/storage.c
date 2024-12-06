#if 0
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
#include "config/app_conf.h"

#define DIR_CHECK(x)                                                                                                   \
    {                                                                                                                  \
        int res = x;                                                                                                   \
        if (res < 0)                                                                                                   \
            log_error("Errore nel maneggiare una cartella: %s", strerror(errno));                                      \
    }

#ifdef TARGET_DEBUG
#define remount_ro()
#define remount_rw()
#else
static inline void remount_ro() {
    if (mount("/dev/mmcblk0p3", DEFAULT_BASE_PATH, "ext2", MS_REMOUNT | MS_RDONLY, NULL) < 0)
        log_error("Errore nel montare la partizione %s: %s (%i)", DEFAULT_BASE_PATH, strerror(errno), errno);
}

static inline void remount_rw() {
    if (mount("/dev/mmcblk0p3", DEFAULT_BASE_PATH, "ext2", MS_REMOUNT, NULL) < 0)
        log_error("Errore nel montare la partizione %s: %s (%i)", DEFAULT_BASE_PATH, strerror(errno), errno);
}
#endif


int storage_load_programs(const char *path, press_program_list_t *pmodel) {
    char file_path[128];
    int count = 0;

    for (int i = 0; i < NUM_PROGRAMS; i++) {
        sprintf(file_path, "%s/program_%d.bin", path, i + 1);

        if (storage_is_file(file_path)) {
            log_info("Found program %s", file_path);
            FILE *fp = fopen(file_path, "rb");

            if (!fp) {
                log_error("Failed to open file %s: %s", file_path, strerror(errno));
                continue;
            }

            size_t read = fread(&pmodel->programs[count], sizeof(program_t), 1, fp);

            if (read != 1) {
                log_error("Failed to read file %s: %s", file_path, strerror(errno));
            } else {
                snprintf(pmodel->programs[count].name, sizeof(pmodel->programs[count].name), "Program %d", count + 1);
                log_info("Loaded program %d", count + 1);
                count++;
            }

            fclose(fp);
        }
    }

    pmodel->num_programs = count;
    return 0;
}


int storage_update_program(const char *path, program_t *p) {
    char filename[128];
    int res = 0;

    remount_rw();

    snprintf(filename, sizeof(filename), "%s/%s.bin", path, p->name);
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        log_error("Unable to open %s: %s", filename, strerror(errno));
        return 1;
    }

    size_t written = fwrite(p, sizeof(program_t), 1, fp);
    if (written != 1) {
        res = 1;
        log_error("Failed to write file %s: %s", filename, strerror(errno));
    } else {
        log_info("Saved program %s", p->name);
    }

    fclose(fp);
    remount_ro();
    return res;
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
#ifdef TARGET_DEBUG
    return is_dir(DRIVE_MOUNT_PATH);
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
#ifdef TARGET_DEBUG
    return 0;
#else
    char path[80];
    char drive[64];

    if (!dir_exists(DRIVE_MOUNT_PATH)) {
        storage_create_dir(DRIVE_MOUNT_PATH);
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

    if (mount(path, DRIVE_MOUNT_PATH, "vfat", 0, NULL) < 0) {
        if (errno == EBUSY)     // Il file system era gia' montato
            return 0;

        log_error("Errore nel montare il disco %s: %s (%i)", path, strerror(errno), errno);
        return -1;
    }
    return 0;
#endif
}


void storage_unmount_drive(void) {
#ifdef TARGET_DEBUG
    return;
#endif
    if (dir_exists(DRIVE_MOUNT_PATH)) {
        if (umount(DRIVE_MOUNT_PATH) < 0) {
            log_warn("Umount error: %s (%i)", strerror(errno), errno);
        }
        rmdir(DRIVE_MOUNT_PATH);
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

#ifdef TARGET_DEBUG
    return 0;
#endif

    if (storage_is_file(app_path)) {
        if (mount("/dev/root", "/", "ext2", MS_REMOUNT, NULL) < 0) {
            log_error("Errore nel montare la root: %s (%i)", strerror(errno), errno);
            return -1;
        }

        if ((res = copy_file(temporary_path, app_path)) < 0)
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


static int copy_file(const char *to, const char *from) {
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
#endif
