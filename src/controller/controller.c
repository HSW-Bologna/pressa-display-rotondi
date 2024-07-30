#include "lvgl.h"
#include "model/model.h"
#include "view/view.h"

view_protocol_t controller_view_protocol = {
    
};


void controller_init(mut_model_t *model) {
    view_change_page(&page_main);
}


void controller_manage(mut_model_t *model) {
  (void)model;
  lv_timer_handler();
}

