#include <stdio.h>
#include "view.h"
#include "common.h"
#include "style.h"
#include "model/model.h"


#define DATE_TIME_STRING "2021/07/09 10:47"


void view_common_set_hidden(lv_obj_t *obj, uint8_t hidden) {
    if (hidden && !lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else if (!hidden && lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}


communication_error_popup_t view_common_communication_error_popup(lv_obj_t *parent) {
    lv_obj_t *blanket = lv_obj_create(parent);
    lv_obj_add_style(blanket, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(blanket, LV_PCT(100), LV_PCT(100));

    lv_obj_t *cont = lv_obj_create(blanket);
    lv_obj_set_size(cont, LV_PCT(80), LV_PCT(80));
    lv_obj_center(cont);

    lv_obj_t *lbl_msg = lv_label_create(cont);
    lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_msg, LV_PCT(95));
    // lv_label_set_text(lbl_msg, view_intl_get_string(model, STRINGS_ERRORE_DI_COMUNICAZIONE));
    lv_obj_align(lbl_msg, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *btn = lv_button_create(cont);
    lv_obj_set_size(btn, 104, 48);
    lv_obj_t *lbl_retry = lv_label_create(btn);
    lv_obj_center(lbl_retry);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *btn_disable = lv_button_create(cont);
    lv_obj_set_size(btn_disable, 104, 48);
    lv_obj_t *lbl_disable = lv_label_create(btn_disable);
    lv_obj_center(lbl_disable);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);

    return (communication_error_popup_t){
        .blanket     = blanket,
        .btn_retry   = btn,
        .btn_disable = btn_disable,
        .lbl_msg     = lbl_msg,
        .lbl_retry   = lbl_retry,
        .lbl_disable = lbl_disable,
    };
}


lv_obj_t *view_common_create_datetime_widget(lv_obj_t *parent, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 160, 50);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, x, y);

    lv_obj_t *date_time_label = lv_label_create(cont);
    lv_label_set_text(date_time_label, DATE_TIME_STRING);
    lv_obj_set_style_text_align(date_time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(date_time_label, LV_ALIGN_TOP_MID, 0, -10);

    lv_obj_t *wifi_icon = lv_img_create(cont);
    lv_img_set_src(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_align(wifi_icon, LV_ALIGN_TOP_MID, 0, 5);

    return cont;
}


lv_obj_t *view_common_create_logo_widget(lv_obj_t *parent) {
    // placeholder for the logo
    lv_obj_t *label_logo = lv_label_create(parent);
    lv_label_set_text(label_logo, "Rotondi - Logo");
    lv_obj_set_size(label_logo, 150, 50);
    lv_obj_align(label_logo, LV_ALIGN_TOP_RIGHT, 0, 0);

    return label_logo;
}


lv_obj_t *view_common_create_folder_widget(lv_obj_t *parent, lv_align_t align, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 50, 50);
    lv_obj_align(btn, align, x, y);

    lv_obj_t *folder_icon = lv_img_create(btn);
    lv_image_set_src(folder_icon, LV_SYMBOL_DIRECTORY);
    lv_obj_center(folder_icon);

    return btn;
}


lv_obj_t *view_common_create_play_button(lv_obj_t *parent, lv_align_t align, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 50, 50);
    lv_obj_align(btn, align, x, y);

    lv_obj_t *folder_icon = lv_img_create(btn);
    lv_img_set_src(folder_icon, LV_SYMBOL_PLAY);
    lv_obj_center(folder_icon);

    return btn;
}


lv_obj_t *view_common_back_button_create(lv_obj_t *parent, uint16_t id) {
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, 56, 56);

    lv_obj_t *label = lv_label_create(button);
    lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_label_set_text(label, LV_SYMBOL_LEFT);
    lv_obj_center(label);

    lv_obj_align(button, LV_ALIGN_TOP_LEFT, 0, 0);
    view_register_object_default_callback(button, id);
    return button;
}


lv_obj_t *view_common_title_create(lv_obj_t *parent, uint16_t back_id, const char *text) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont, &style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_PCT(100), 64);

    lv_obj_t *button = view_common_back_button_create(cont, back_id);
    lv_obj_align(button, LV_ALIGN_LEFT_MID, 8, 0);
    view_register_object_default_callback(button, back_id);

    lv_obj_t *label_title = lv_label_create(cont);
    lv_obj_set_style_text_font(label_title, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_label_set_long_mode(label_title, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_title, LV_HOR_RES - 64 - 128 -8);
    lv_obj_align(label_title, LV_ALIGN_LEFT_MID, 64+8, 0);
    lv_label_set_text(label_title, text);

    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 0);

    return cont;
}


lv_obj_t *view_common_time_bar_create(lv_obj_t *parent, uint16_t id) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_add_style(cont, &style_time_bar, LV_STATE_DEFAULT);

    lv_obj_set_style_pad_column(cont, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, PROGRAM_NUM_TIME_UNITS * 47, 46);

    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    for (uint16_t i = 0; i < PROGRAM_NUM_TIME_UNITS; i++) {
        lv_obj_t *unit = lv_button_create(cont);
        lv_obj_remove_flag(unit, LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_remove_flag(unit, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_add_style(unit, &style_time_unit, LV_STATE_DEFAULT);
        lv_obj_set_style_border_post(unit, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(unit, LV_OPA_TRANSP, LV_STATE_DEFAULT);
        lv_obj_set_height(unit, 46);
        lv_obj_set_width(unit, 46);
        // lv_obj_set_flex_grow(unit, 1);
        view_register_object_default_callback_with_number(unit, id, i);

        lv_obj_t *label = lv_label_create(unit);
        lv_label_set_text_fmt(label, "%i", i + 1);
        lv_obj_center(label);
    }

    return cont;
}


lv_obj_t *view_common_gradient_background_create(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));

    lv_obj_set_style_bg_grad_color(obj, lv_color_lighten(STYLE_COLOR_PRIMARY, LV_OPA_50), 0);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);

    return obj;
}
