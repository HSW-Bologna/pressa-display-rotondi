#include "lvgl.h"
#include "src/widgets/button/lv_button.h"
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "model/model.h"
#include "view/view.h"
#include "controller/controller.h"
#include "controller/gui.h"
#include "services/system_time.h"
#include "config/app_config.h"
#include "log.h"
#include "bsp/rs232.h"


static void            log_file_init(void);
static void            log_lock(void *arg, int op);
static pthread_mutex_t lock;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    mut_model_t model;

    bsp_rs232_init();

    log_set_level(APP_CONFIG_LOG_LEVEL);
    log_file_init();

    log_info("App version %s, %s", SOFTWARE_VERSION, SOFTWARE_BUILD_DATE);

    model_init(&model);

    view_init(&model, gui_view_protocol);
    controller_init(&model);

    for (;;) {
        controller_manage(&model);

        //     view_event((view_event_t){.tag = VIEW_EVENT_TAG_STORAGE_OPERATION_COMPLETED});

        usleep(5 * 1000);
    }
    return 0;
}

static void log_file_init(void) {
    assert(pthread_mutex_init(&lock, NULL) == 0);
    log_set_lock(log_lock);
    log_set_udata(&lock);

    log_set_fp(fopen(APP_CONFIG_LOGFILE, "a+"));
    log_set_fileinfo(0);
}


static void log_lock(void *arg, int op) {
    op ? pthread_mutex_lock(arg) : pthread_mutex_unlock(arg);
}
