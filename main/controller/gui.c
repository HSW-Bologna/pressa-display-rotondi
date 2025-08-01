#include <lvgl.h>
#include "gui.h"
#include "services/timestamp.h"
#include "adapters/network/network.h"
#include "controller.h"
#include "storage/disk_op.h"
#include "bsp/system.h"
#include <esp_log.h>
#include "minion.h"


static void set_test_mode(pman_handle_t handle, uint8_t test_on);
static void test_output(pman_handle_t handle, uint16_t output_index);
static void test_output_clear(pman_handle_t handle);
static void test_pwm(pman_handle_t handle, uint8_t percentage);
static void save_configuration(pman_handle_t handle);
static void retry_communication(pman_handle_t handle);
static void export_configuration(pman_handle_t handle, const char *name);
static void ota_update(pman_handle_t handle);
static void finalize_ota_update(pman_handle_t handle);
static void wifi_scan(pman_handle_t handle);
static void connect_to_wifi(pman_handle_t handle, char *ssid, char *psk);


view_protocol_t gui_view_protocol = {
    .set_test_mode        = set_test_mode,
    .test_output          = test_output,
    .test_output_clear    = test_output_clear,
    .test_pwm             = test_pwm,
    .save_configuration   = save_configuration,
    .export_configuration = export_configuration,
    .retry_communication  = retry_communication,
    .ota_update           = ota_update,
    .finalize_ota_update  = finalize_ota_update,
    .wifi_scan            = wifi_scan,
    .connect_to_wifi      = connect_to_wifi,
};

static const char *TAG = __FILE_NAME__;


void controller_gui_manage(mut_model_t *model) {
#ifndef BUILD_CONFIG_SIMULATOR
    static timestamp_t last_invoked = 0;

    if (last_invoked != timestamp_get()) {
        if (last_invoked > 0) {
            lv_tick_inc(timestamp_interval(last_invoked, timestamp_get()));
        }
        last_invoked = timestamp_get();
    }
#endif

    view_manage(model);
    lv_timer_handler();
}


static void set_test_mode(pman_handle_t handle, uint8_t test_on) {
    mut_model_t *model              = view_get_model(handle);
    model->run.minion.write.test_on = test_on;
    if (!test_on) {
        model_clear_test_outputs(model);
    }
    controller_sync_minion(model);
}


static void test_output_clear(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    controller_sync_minion(model);
}


static void test_output(pman_handle_t handle, uint16_t output_index) {
    mut_model_t *model = view_get_model(handle);

    if (((1 << output_index) & model->run.minion.write.outputs) > 0) {
        model_clear_test_outputs(model);
    } else {
        model_set_test_output(model, output_index);
    }
    controller_sync_minion(model);
}


static void test_pwm(pman_handle_t handle, uint8_t percentage) {
    mut_model_t *model = view_get_model(handle);

    model->run.minion.write.pwm = percentage;
    controller_sync_minion(model);
}


static void save_configuration(pman_handle_t handle) {
    model_t *model = view_get_model(handle);
    disk_op_save_config(&model->config);
}


static void export_configuration(pman_handle_t handle, const char *name) {
    (void)handle;
    disk_op_export_config(name);
}


static void retry_communication(pman_handle_t handle) {
    ESP_LOGI(TAG, "Requesting com retry");
    minion_retry_communication();

    mut_model_t *model                    = view_get_model(handle);
    model->run.minion.communication_error = 0;
}


static void ota_done_cb(uint8_t error, void *data, void *arg) {
    (void)data;
    (void)arg;
    if (!error) {
        ESP_LOGI(TAG, "Ota successful, restarting");
        bsp_system_reset();
    } else {
        ESP_LOGW(TAG, "Ota failed!");
    }
}


static void ota_update(pman_handle_t handle) {
    (void)handle;
    ESP_LOGI(TAG, "Ota update");
    disk_op_firmware_update(ota_done_cb);
}


static void finalize_ota_done_cb(uint8_t error, void *data, void *arg) {
    (void)data;
    (void)arg;
    if (!error) {
        ESP_LOGI(TAG, "Ota completely successful, restarting");
        exit(0);
    } else {
        ESP_LOGW(TAG, "Ota failed!");
    }
}


static void finalize_ota_update(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    ESP_LOGI(TAG, "Finalizing ota update");
    disk_op_finalize_firmware_update(model->args.new_firmware_path, finalize_ota_done_cb);
}


static void wifi_scan(pman_handle_t handle) {
    (void)handle;
    network_scan();
}


static void connect_to_wifi(pman_handle_t handle, char *ssid, char *psk) {
    (void)handle;
    network_connect(ssid, psk);
    disk_op_save_wifi_config();
}
