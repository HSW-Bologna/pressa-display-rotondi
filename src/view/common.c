#include <stdio.h>
#include "view.h"
#include "common.h"

#define DATE_TIME_STRING "2021/07/09 10:47"

lv_obj_t* view_common_create_datetime_widget(lv_obj_t* parent, lv_coord_t x, lv_coord_t y) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 160, 50);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, x, y);

    lv_obj_t *date_time_label = lv_label_create(cont);
    lv_label_set_text(date_time_label, DATE_TIME_STRING);
    lv_obj_set_style_text_align(date_time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(date_time_label, LV_ALIGN_TOP_MID, 0, -10);

    lv_obj_t* wifi_icon = lv_img_create(cont);
    lv_img_set_src(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_align(wifi_icon, LV_ALIGN_TOP_MID, 0, 5);

    return cont;
}

lv_obj_t* view_common_create_logo_widget(lv_obj_t* parent) {
    // placeholder for the logo
    lv_obj_t* label_logo = lv_label_create(parent);
    lv_label_set_text(label_logo, "Rotondi - Logo");
    lv_obj_set_size(label_logo, 150, 50);
    lv_obj_align(label_logo, LV_ALIGN_TOP_RIGHT, 0, 0);

    return label_logo;
}

lv_obj_t* view_common_create_folder_widget(lv_obj_t* parent, lv_align_t align, lv_coord_t x, lv_coord_t y) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, 50, 50);
    lv_obj_align(btn, align, x, y);

    lv_obj_t* folder_icon = lv_img_create(btn);
    lv_img_set_src(folder_icon, LV_SYMBOL_DIRECTORY);
    lv_obj_center(folder_icon);

    return btn;
}
