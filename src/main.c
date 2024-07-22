#include "lvgl.h"
#include "src/widgets/button/lv_button.h"
#include <unistd.h>
#include <stdio.h>
#include "model/model.h"
#include "view/view.h"
#include "controller/controller.h"


int main(int argc, char *argv[]) {
    mut_model_t model = {0};

    view_init(&model, controller_view_protocol);
    controller_init(&model);

    for (;;) {
        controller_manage(&model);

        view_event((view_event_t){.tag = VIEW_EVENT_TAG_STORAGE_OPERATION_COMPLETED});

        usleep(5 * 1000);
    }
    return 0;
}
