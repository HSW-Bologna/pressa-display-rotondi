#include "../view.h"
#include "lvgl.h"
#include "model/model.h"
#include "src/core/lv_obj_event.h"
#include "src/misc/lv_types.h"
#include "src/page.h"
#include "../style.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include "../common.h"


enum {
    BTN_BACK_ID,
    BTN_OUTPUT_ID,
    SLIDER_PWM_ID,
    TABVIEW_ID,
};


struct page_data {
    lv_obj_t *led_outputs[NUM_OUTPUTS];
    lv_obj_t *led_input[NUM_INPUTS];

    lv_obj_t *label_4_20ma;
    lv_obj_t *label_pwm;

    lv_obj_t *slider_4_20ma;
    lv_obj_t *slider_pwm;
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    return pdata;
}

static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    lv_obj_t *tabview = lv_tabview_create(lv_screen_active());
    view_register_object_default_callback(tabview, TABVIEW_ID);

    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview);
    lv_obj_set_style_pad_left(tab_bar, 64, LV_STATE_DEFAULT);

    {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, "Digitale");

        {
            lv_obj_t *cont = lv_obj_create(tab);
            lv_obj_set_style_pad_column(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_size(cont, LV_PCT(100), LV_PCT(50));
            lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
            lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 0);

            for (size_t i = 0; i < NUM_OUTPUTS; i++) {
                lv_obj_t *btn = lv_btn_create(cont);
                lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_radius(btn, 4, LV_STATE_DEFAULT);
                lv_obj_set_size(btn, 52, 52);
                view_register_object_default_callback_with_number(btn, BTN_OUTPUT_ID, i);

                lv_obj_t *led = lv_led_create(btn);
                lv_obj_add_flag(led, LV_OBJ_FLAG_EVENT_BUBBLE);
                lv_led_set_color(led, lv_color_darken(STYLE_COLOR_GREEN, LV_OPA_20));
                lv_obj_set_size(led, 32, 32);
                lv_obj_center(led);

                lv_obj_t *lbl = lv_label_create(btn);
                lv_obj_set_style_text_color(lbl, STYLE_COLOR_BLACK, LV_STATE_DEFAULT);
                lv_obj_add_flag(lbl, LV_OBJ_FLAG_EVENT_BUBBLE);
                lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_white(), LV_STATE_DEFAULT);
                lv_label_set_text_fmt(lbl, "%02zu", i + 1);
                lv_obj_center(lbl);

                pdata->led_outputs[i] = led;
            }
        }

        {
            lv_obj_t *cont = lv_obj_create(tab);
            lv_obj_set_style_pad_column(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_size(cont, LV_PCT(100), LV_PCT(50));
            lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
            lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

            for (size_t i = 0; i < NUM_INPUTS; i++) {
                lv_obj_t *obj = lv_obj_create(cont);
                lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_radius(obj, 4, LV_STATE_DEFAULT);
                lv_obj_set_size(obj, 52, 52);

                lv_obj_t *led = lv_led_create(obj);
                lv_led_set_color(led, lv_color_darken(STYLE_COLOR_GREEN, LV_OPA_20));
                lv_obj_set_size(led, 32, 32);
                lv_obj_center(led);

                lv_obj_t *lbl = lv_label_create(obj);
                lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(lbl, lv_color_white(), LV_STATE_DEFAULT);
                lv_label_set_text_fmt(lbl, "%02zu", i + 1);
                lv_obj_center(lbl);

                pdata->led_input[i] = led;
            }
        }
    }

    {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, "Analogico");

        {
            lv_obj_t *slider = lv_slider_create(tab);
            lv_slider_set_range(slider, 0, 100);
            lv_obj_set_style_pad_hor(slider, 32, LV_STATE_DEFAULT | LV_PART_MAIN);
            lv_obj_set_style_pad_all(slider, -4, LV_STATE_DEFAULT | LV_PART_KNOB);
            lv_obj_set_size(slider, 640, 64);
            lv_obj_align(slider, LV_ALIGN_CENTER, 0, -128);
            lv_obj_remove_flag(slider, LV_OBJ_FLAG_CLICKABLE);
            view_register_object_default_callback(slider, SLIDER_PWM_ID);
            pdata->slider_4_20ma = slider;

            lv_obj_t *label = lv_label_create(tab);
            lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
            lv_obj_align_to(label, slider, LV_ALIGN_BOTTOM_MID, 0, 32);
            pdata->label_4_20ma = label;
        }

        {
            lv_obj_t *slider = lv_slider_create(tab);
            lv_slider_set_range(slider, 0, 100);
            lv_obj_set_style_pad_hor(slider, 32, LV_STATE_DEFAULT | LV_PART_MAIN);
            lv_obj_set_style_pad_hor(slider, -4, LV_STATE_DEFAULT | LV_PART_KNOB);
            lv_obj_set_size(slider, 640, 64);
            lv_obj_align(slider, LV_ALIGN_CENTER, 0, 96);
            view_register_object_default_callback(slider, SLIDER_PWM_ID);
            pdata->slider_pwm = slider;

            lv_obj_t *label = lv_label_create(tab);
            lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
            lv_obj_align_to(label, slider, LV_ALIGN_BOTTOM_MID, 0, 32);
            pdata->label_pwm = label;
        }
    }

    lv_obj_t *button = lv_button_create(lv_screen_active());
    lv_obj_set_size(button, 64, 64);
    lv_obj_align(button, LV_ALIGN_TOP_LEFT, 0, 0);
    view_register_object_default_callback(button, BTN_BACK_ID);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, LV_SYMBOL_LEFT);
    lv_obj_center(label);

    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read, 0);

    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;

    mut_model_t *model = view_get_model(handle);

    switch (event.tag) {
        case PMAN_EVENT_TAG_OPEN: {
            view_get_protocol(handle)->set_test_mode(handle, 1);
            break;
        }

        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_STORAGE_OPERATION_COMPLETED:
                    break;
                case VIEW_EVENT_TAG_PAGE_WATCHER:
                    update_page(model, pdata);
                    break;
                default:
                    break;
            }
            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t *target     = lv_event_get_current_target_obj(event.as.lvgl);
            uint16_t  obj_id     = view_get_obj_id(target);
            uint16_t  obj_number = view_get_obj_number(target);

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    switch (obj_id) {
                        case BTN_BACK_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            view_get_protocol(handle)->set_test_mode(handle, 0);
                            break;

                        case BTN_OUTPUT_ID:
                            view_get_protocol(handle)->test_output(handle, obj_number);
                            update_page(model, pdata);
                            break;

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_VALUE_CHANGED: {
                    switch (obj_id) {
                        case TABVIEW_ID:
                            view_get_protocol(handle)->test_output_clear(handle);
                            view_get_protocol(handle)->test_pwm(handle, 0);
                            break;

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_RELEASED: {
                    switch (obj_id) {
                        case SLIDER_PWM_ID:
                            view_get_protocol(handle)->test_pwm(handle, lv_slider_get_value(pdata->slider_pwm));
                            update_page(model, pdata);
                            break;

                        default:
                            break;
                    }
                    break;
                }

                default:
                    break;
            }

            break;
        }

        default:
            break;
    }

    return msg;
}


static void update_page(model_t *model, struct page_data *pdata) {
    for (int16_t i = 0; i < NUM_OUTPUTS; i++) {
        if ((model->run.minion.write.outputs & (1 << i)) > 0) {
            lv_led_on(pdata->led_outputs[i]);
        } else {
            lv_led_off(pdata->led_outputs[i]);
        }
    }

    for (size_t i = 0; i < NUM_INPUTS; i++) {
        if (model->run.minion.read.inputs & (1 << i)) {
            lv_led_on(pdata->led_input[i]);
        } else {
            lv_led_off(pdata->led_input[i]);
        }
    }

    lv_slider_set_value(pdata->slider_4_20ma, model->run.minion.read.ma4_20_adc, LV_ANIM_OFF);
    lv_label_set_text_fmt(pdata->label_4_20ma, "%4i", model->run.minion.read.ma4_20_adc);
    lv_slider_set_value(pdata->slider_pwm, model->run.minion.write.pwm, LV_ANIM_OFF);
    lv_label_set_text_fmt(pdata->label_pwm, "%3i%% (%4i)", model->run.minion.write.pwm,
                          model->run.minion.read.v0_10_adc);
}


static void close_page(void *state) {
    (void)state;
    lv_obj_clean(lv_screen_active());
}


const pman_page_t page_test = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
