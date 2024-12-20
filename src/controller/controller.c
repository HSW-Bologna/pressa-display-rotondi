#include <string.h>
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
#include "config/app_config.h"
#include "log.h"
#include "gui.h"
#include "minion.h"
#include "services/timestamp.h"


static void load_programs_callback(model_t *pmodel, void *data, void *arg);


void controller_init(mut_model_t *model) {
    (void)model;

    wifi_init();
    minion_init();

    disk_op_init();
    disk_op_load_config();
    for (;;) {
        disk_op_response_t response = {0};
        if (disk_op_get_response(&response)) {
            if (response.tag == DISK_OP_RESPONSE_TAG_CONFIGURATION_LOADED) {
                memcpy(&model->config, response.as.configuration_loaded.config, sizeof(configuration_t));
                model_check_parameters(model);
                log_info("Configuration loaded!");
            } else {
                log_warn("Could not load configuration (%i)", response.tag);
            }
            break;
        } else {
            usleep(1000);
        }
    }

    view_change_page(&page_home);
}


void controller_manage(mut_model_t *model) {
    controller_gui_manage(model);

    {
        minion_response_t response = {0};
        if (minion_get_response(&response)) {
            switch (response.tag) {
                case MINION_RESPONSE_TAG_ERROR: {
                    log_error("Error");
                    break;
                }

                case MINION_RESPONSE_TAG_SYNC: {
                    model->run.minion.read.firmware_version_major = response.as.sync.firmware_version_major;
                    model->run.minion.read.firmware_version_minor = response.as.sync.firmware_version_minor;
                    model->run.minion.read.firmware_version_patch = response.as.sync.firmware_version_patch;
                    model->run.minion.read.inputs                 = response.as.sync.inputs;
                    model->run.minion.read.v0_10_adc              = response.as.sync.v0_10_adc;
                    model->run.minion.read.ma4_20_adc             = response.as.sync.ma4_20_adc;
                    break;
                }
            }
        }
    }

    {
        static timestamp_t ts = 0;
        if (timestamp_is_expired(ts, 400)) {
            // controller_sync_minion(model);
            ts = timestamp_get();
        }
    }
}


#if 0
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
#endif


void controller_sync_minion(model_t *model) {
    minion_sync(model);
}
