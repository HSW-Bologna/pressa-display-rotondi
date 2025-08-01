#include "FreeRTOS.h"
#include "task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "model/model.h"
#include "adapters/view/view.h"
#include "controller/controller.h"
#include "controller/gui.h"
#include "bsp/rs232.h"


static const char *TAG = "Main";


void app_main(void *arg) {
    (void)arg;

    mut_model_t model = {0};

    bsp_rs232_init();

    lv_init();
    lv_sdl_window_create(1280, 800);
    lv_sdl_mouse_create();

    model_init(&model);
    view_init(&model, gui_view_protocol);
    controller_init(&model);
    ESP_LOGI(TAG, "controller");

    ESP_LOGI(TAG, "Begin main loop");
    for (;;) {
        controller_manage(&model);

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    vTaskDelete(NULL);
}
