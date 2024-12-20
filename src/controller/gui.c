#include <lvgl.h>
#include "gui.h"
#include "services/timestamp.h"
#include "view/view.h"
#include "controller.h"
#include "storage/disk_op.h"


static void set_test_mode(pman_handle_t handle, uint8_t test_on);
static void test_output(pman_handle_t handle, uint16_t output_index);
static void test_output_clear(pman_handle_t handle);
static void test_pwm(pman_handle_t handle, uint8_t percentage);
static void save_configuration(pman_handle_t handle);


view_protocol_t gui_view_protocol = {
    .set_test_mode      = set_test_mode,
    .test_output        = test_output,
    .test_output_clear  = test_output_clear,
    .test_pwm           = test_pwm,
    .save_configuration = save_configuration,
};


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
