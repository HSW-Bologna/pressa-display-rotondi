#include <esp_log.h>
#include "lvgl.h"
#include "src/widgets/button/lv_button.h"
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "model/model.h"
#include "adapters/view/view.h"
#include "controller/controller.h"
#include "controller/gui.h"
#include "services/system_time.h"
#include "config/app_config.h"
#include "bsp/rs232.h"
#include "bsp/lcd.h"


static void loop(lv_timer_t *timer);


static const char *TAG = __FILE_NAME__;


void app_main(void) {
    bsp_rs232_init();
    bsp_lcd_init();
    lv_timer_create(loop, 1, NULL);

    ESP_LOGI(TAG, "App version %s, %s", SOFTWARE_VERSION, SOFTWARE_BUILD_DATE);
}


static void loop(lv_timer_t *timer) {
    static mut_model_t model = {0};

    static bool first_run = true;
    if (first_run) {
        model_init(&model);
        view_init(&model, gui_view_protocol);
        controller_init(&model);
        first_run = false;
    } else {
        controller_manage(&model);
    }
}
