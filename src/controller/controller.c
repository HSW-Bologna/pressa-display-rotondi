#include <linux/reboot.h>
#include <sys/reboot.h>
#include <unistd.h>
#include "lvgl.h"
#include "controller.h"
#include "model/model.h"
#include "view/view.h"
#include "storage/disk_op.h"
#include "storage/storage.h"
#include "controller/network/wifi.h"
#include "config/app_conf.h"
#include "../../lib/log/src/log.h"

view_protocol_t controller_view_protocol = {
    
};

static void load_programs_callback(model_t *pmodel, void *data, void *arg);


void controller_init(mut_model_t *model) {
  wifi_init();
  disk_op_init();

  disk_op_load_programs(load_programs_callback, NULL, NULL);
    while (disk_op_manage_response(pmodel) == 0) {
        usleep(1000);
    }

  view_change_page(&page_home);
}


void controller_manage(mut_model_t *model) {
  (void)model;
  lv_timer_handler();
}


static void load_programs_callback(model_t *pmodel, void *data, void *arg) {
    (void)arg;
    storage_program_list_t *list = data;

    memcpy(pmodel->configuration.programs, list->programs, sizeof(list->programs[0]) * list->num_programs);
    pmodel->configuration.num_programs = list->num_programs;
    log_info("Caricati %zu programmi", list->num_programs);
    for (size_t i = 0; i < model_get_num_programs(pmodel); i++) {
        program_t *p = model_get_program(pmodel, i);
        // TODO check this
        // for (size_t j = 0; j < p->num_steps; j++) {
        //     // Limit check
        //     parciclo_init(pmodel, i, j);
        // }
    }
}
