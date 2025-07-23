#include <string.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <unistd.h>
#include "lvgl.h"
#include "controller.h"
#include "model/model.h"
#include "adapters/view/view.h"
#include "adapters/network/network.h"
#include "storage/disk_op.h"
#include "storage/storage.h"
#include "config/app_config.h"
#include "log.h"
#include "gui.h"
#include "minion.h"
#include "services/timestamp.h"


static void load_programs_callback(model_t *pmodel, void *data, void *arg);


void controller_init(mut_model_t *model) {
    (void)model;

    minion_init();
    network_init();

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
                disk_op_save_config(&model->config);
                view_show_toast(1, "Non sono riuscito a caricare una configurazione!");
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
                    model->run.minion.communication_error = 1;
                    break;
                }

                case MINION_RESPONSE_TAG_SYNC: {
                    model->run.minion.read.firmware_version_major = response.as.sync.firmware_version_major;
                    model->run.minion.read.firmware_version_minor = response.as.sync.firmware_version_minor;
                    model->run.minion.read.firmware_version_patch = response.as.sync.firmware_version_patch;
                    model->run.minion.read.inputs                 = response.as.sync.inputs;
                    model->run.minion.read.v0_10_adc              = response.as.sync.v0_10_adc;
                    model->run.minion.read.ma4_adc                = response.as.sync.ma4_adc;
                    model->run.minion.read.ma20_adc               = response.as.sync.ma20_adc;
                    model->run.minion.read.ma4_20_adc             = response.as.sync.ma4_20_adc;
                    model->run.minion.read.running                = response.as.sync.running;
                    model->run.minion.read.elapsed_milliseconds   = response.as.sync.elapsed_time_ms;


                    if (model_is_program_ready(model)) {
                        const program_t *program = model_get_current_program(model);

                        uint16_t time_unit_index =
                            model->run.minion.read.elapsed_milliseconds / (program->time_unit_decisecs * 100);
                        if (time_unit_index < PROGRAM_NUM_TIME_UNITS) {
                            size_t  sensor_threshold_index = program->sensor_channel[time_unit_index];
                            int16_t position_target        = model->config.ma4_20_offset +
                                                      (int16_t)model_position_mm_to_adc(
                                                          model, program->position_levels[sensor_threshold_index - 1]);

                            log_info("MA420: %4i, %4i", model->run.minion.read.ma4_20_adc, position_target);
                        }
                    }

                    break;
                }
            }
        }
    }

    {
        disk_op_response_t response = {0};
        if (disk_op_get_response(&response)) {
            switch (response.tag) {
                case DISK_OP_RESPONSE_TAG_ERROR:
                    view_show_toast(1, "Operazione su disco fallita!");
                    break;

                case DISK_OP_RESPONSE_TAG_CONFIGURATION_EXPORTED:
                    disk_op_update_importable_configurations(model);
                    break;

                default:
                    break;
            }
        }
    }

    {
        static timestamp_t ts = 0;
        if (timestamp_is_expired(ts, 200)) {
            controller_sync_minion(model);

            uint8_t drive_mounted = disk_op_is_drive_mounted();

            if (drive_mounted && !model->run.drive_mounted) {
                disk_op_update_importable_configurations(model);
                model->run.firmware_update_ready = disk_op_is_firmware_present();
            }
            model->run.drive_mounted = drive_mounted;

            ts = timestamp_get();
        }
    }

    {
        static timestamp_t ts = 0;
        if (timestamp_is_expired(ts, 2000)) {
            model->network.num_networks = network_read_wifi_scan(&model->network.networks);
            wifi_status_t status        = network_status(model->network.ssid);

            if (status != WIFI_SCANNING) {
                model->network.net_status = status;
            }
            int ip1 = 0, ip2 = 0;
            if (status == WIFI_CONNECTED) {
                ip1 = network_get_ip_address(APP_CONFIG_IFWIFI, model->network.wifi_ipaddr);
            } else {
                strcpy(model->network.wifi_ipaddr, "_._._._");
            }

            ip2 = network_get_ip_address(APP_CONFIG_IFETH, model->network.eth_ipaddr);

            model->run.network_connected = ip1 || ip2;

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
    if (model_is_communication_ok(model)) {
        minion_sync(model);
    }
}
