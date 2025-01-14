#ifndef VIEW_COMMON_H_INCLUDED
#define VIEW_COMMON_H_INCLUDED


#include <stdio.h>
#include "lvgl.h"
#include "model/model.h"


typedef struct {
    lv_obj_t *blanket;

    lv_obj_t *lbl_msg;
    lv_obj_t *lbl_retry;
    lv_obj_t *lbl_disable;
    lv_obj_t *btn_retry;
    lv_obj_t *btn_disable;
} communication_error_popup_t;


lv_obj_t *view_common_create_datetime_widget(lv_obj_t *parent, lv_coord_t x, lv_coord_t y);
lv_obj_t *view_common_create_logo_widget(lv_obj_t *parent);
lv_obj_t *view_common_create_folder_widget(lv_obj_t *parent, lv_align_t align, lv_coord_t x, lv_coord_t y);
lv_obj_t *view_common_create_play_button(lv_obj_t *parent, lv_align_t align, lv_coord_t x, lv_coord_t y);
lv_obj_t *view_common_title_create(lv_obj_t *parent, uint16_t back_id, const char *text);
lv_obj_t *view_common_time_bar_create(lv_obj_t *parent, uint16_t id);
void      view_common_set_hidden(lv_obj_t *obj, uint8_t hidden);
lv_obj_t *view_common_back_button_create(lv_obj_t *parent, uint16_t id);
lv_obj_t *view_common_gradient_background_create(lv_obj_t *parent);

communication_error_popup_t view_common_communication_error_popup(lv_obj_t *parent);


#endif
