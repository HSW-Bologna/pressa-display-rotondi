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
    BTN_WIFI_ID,
    SLIDER_PWM_ID,
    KEYBOARD_ID,
    TIMER_WIFI_ID,
    UPDATE_MINION_ID,
    UPDATE_NETWORK_ID,
};


struct page_data {
    lv_obj_t *led_outputs[NUM_OUTPUTS];
    lv_obj_t *led_input[NUM_INPUTS];

    lv_obj_t *obj_blanket;

    lv_obj_t *textarea;

    lv_obj_t *label_4_20ma;
    lv_obj_t *label_pwm;
    lv_obj_t *label_wifi;
    lv_obj_t *label_state;
    lv_obj_t *label_ethernet;

    lv_obj_t *list_networks;

    lv_obj_t *slider_pwm;

    pman_timer_t *timer_wifi;

    uint8_t password;
    char    selected_network[33];
};


static const void *get_wifi_icon(int signal);
static void        update_network_list(struct page_data *pdata, wifi_network_t *networks, int num);
static void        update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->timer_wifi = PMAN_REGISTER_TIMER_ID(handle, 10000, TIMER_WIFI_ID);
    pdata->password   = 0;

    return pdata;
}

static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    pman_timer_resume(pdata->timer_wifi);

    lv_obj_t *tabview = lv_tabview_create(lv_screen_active());
    lv_obj_set_style_text_font(tabview, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview);
    lv_obj_set_style_pad_left(tab_bar, 64, LV_STATE_DEFAULT);

    lv_obj_t *button = view_common_back_button_create(lv_screen_active(), BTN_BACK_ID);
    lv_obj_set_size(button, 64, 64);
    lv_obj_align(button, LV_ALIGN_TOP_LEFT, 0, 0);
    view_register_object_default_callback(button, BTN_BACK_ID);

    {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, "Diagnosi");
        lv_obj_set_style_text_font(tab, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_ver(tab, 10, LV_STATE_DEFAULT);

        lv_obj_t *base_cont = tab;

        lv_obj_set_style_pad_column(base_cont, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(base_cont, 16, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_hor(base_cont, 4, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_ver(base_cont, 4, LV_STATE_DEFAULT);
        lv_obj_set_size(base_cont, LV_PCT(100), LV_VER_RES - 64);
        lv_obj_set_layout(base_cont, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(base_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(base_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_align(base_cont, LV_ALIGN_BOTTOM_MID, 0, 0);

        {
            lv_obj_t *cont = lv_obj_create(base_cont);
            lv_obj_add_style(cont, &style_transparent_cont, LV_STATE_DEFAULT);
            lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_column(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_width(cont, LV_PCT(100));
            lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            for (size_t i = 0; i < NUM_OUTPUTS; i++) {
                lv_obj_t *btn = lv_button_create(cont);
                lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_radius(btn, 4, LV_STATE_DEFAULT);
                lv_obj_set_size(btn, 48, 48);
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
            lv_obj_t *cont = lv_obj_create(base_cont);
            lv_obj_add_style(cont, &style_transparent_cont, LV_STATE_DEFAULT);
            lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_column(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_size(cont, LV_PCT(100), 60);
            lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            for (size_t i = 0; i < NUM_INPUTS; i++) {
                lv_obj_t *obj = lv_obj_create(cont);
                lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_radius(obj, 4, LV_STATE_DEFAULT);
                lv_obj_set_size(obj, 44, 44);

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

        {
            lv_obj_t *label = lv_label_create(base_cont);
            lv_obj_set_width(label, 64);
            lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
            pdata->label_4_20ma = label;
        }

        {
            lv_obj_t *slider = lv_slider_create(base_cont);
            lv_slider_set_range(slider, 0, 100);
            lv_obj_set_style_pad_hor(slider, 32, LV_STATE_DEFAULT | LV_PART_MAIN);
            lv_obj_set_style_pad_hor(slider, -4, LV_STATE_DEFAULT | LV_PART_KNOB);
            lv_obj_set_size(slider, 640, 48);
            lv_obj_align(slider, LV_ALIGN_CENTER, 0, 96);
            view_register_object_default_callback(slider, SLIDER_PWM_ID);
            pdata->slider_pwm = slider;

            lv_obj_t *label = lv_label_create(base_cont);
            lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
            lv_obj_align_to(label, slider, LV_ALIGN_BOTTOM_MID, 0, 32);
            pdata->label_pwm = label;
        }
    }

    {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, "Rete");
        lv_obj_set_style_text_font(tab, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_ver(tab, 10, LV_STATE_DEFAULT);

        pdata->label_wifi = lv_label_create(tab);
        lv_obj_set_style_text_font(pdata->label_wifi, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_obj_align(pdata->label_wifi, LV_ALIGN_TOP_LEFT, 8, 8);

        pdata->label_state = lv_label_create(tab);
        lv_obj_set_style_text_font(pdata->label_state, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_obj_align_to(pdata->label_state, pdata->label_wifi, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

        pdata->label_ethernet = lv_label_create(tab);
        lv_obj_set_style_text_font(pdata->label_ethernet, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_obj_align_to(pdata->label_ethernet, pdata->label_wifi, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 40);

        pdata->list_networks = lv_list_create(tab);
        lv_obj_set_size(pdata->list_networks, 480, 380);
        lv_obj_align(pdata->list_networks, LV_ALIGN_TOP_RIGHT, -8, 8);
        update_network_list(pdata, model->network.networks, model->network.num_networks);
    }

    {
        lv_obj_t *blanket = lv_obj_create(lv_screen_active());
        lv_obj_set_size(blanket, LV_PCT(100), LV_PCT(100));
        lv_obj_add_style(blanket, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_t *kb = lv_keyboard_create(blanket);
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

        lv_obj_t *ta = lv_textarea_create(blanket);
        lv_obj_set_style_text_font(ta, STYLE_FONT_BIG, LV_STATE_DEFAULT);
        lv_obj_set_width(ta, 420);
        lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 120);
        lv_textarea_set_text(ta, "");
        lv_textarea_set_password_mode(ta, 1);
        lv_textarea_set_placeholder_text(ta, "Password");
        lv_textarea_set_max_length(ta, 32);
        lv_textarea_set_one_line(ta, 1);
        lv_keyboard_set_textarea(kb, ta);
        view_register_object_default_callback(kb, KEYBOARD_ID);
        pdata->textarea = ta;

        pdata->obj_blanket = blanket;
    }

    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read, UPDATE_MINION_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->network, UPDATE_NETWORK_ID);

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

        case PMAN_EVENT_TAG_TIMER: {
            view_get_protocol(handle)->wifi_scan(handle);
            break;
        }

        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_STORAGE_OPERATION_COMPLETED:
                    break;
                case VIEW_EVENT_TAG_PAGE_WATCHER: {
                    switch (view_event->as.page_watcher.code) {
                        case UPDATE_NETWORK_ID:
                            update_network_list(pdata, model->network.networks, model->network.num_networks);
                            break;
                        default:
                            break;
                    }
                    update_page(model, pdata);
                    break;
                }
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

                        case BTN_WIFI_ID:
                            snprintf(pdata->selected_network, sizeof(pdata->selected_network), "%s",
                                     model->network.networks[obj_id].ssid);
                            pdata->password = 1;
                            update_page(model, pdata);
                            break;

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_CANCEL: {
                    switch (obj_id) {
                        case KEYBOARD_ID:
                            pdata->password = 0;
                            update_page(model, pdata);
                            break;

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_READY: {
                    switch (obj_id) {
                        case KEYBOARD_ID:
                            pdata->password = 0;
                            view_get_protocol(handle)->connect_to_wifi(handle, pdata->selected_network,
                                                                       (char *)lv_textarea_get_text(pdata->textarea));
                            update_page(model, pdata);
                            break;

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_VALUE_CHANGED: {
                    switch (obj_id) {
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

    lv_label_set_text_fmt(pdata->label_4_20ma, "%4i", model->run.minion.read.ma4_20_adc);
    lv_slider_set_value(pdata->slider_pwm, model->run.minion.write.pwm, LV_ANIM_OFF);
    lv_label_set_text_fmt(pdata->label_pwm, "%3i%% (%4i)", model->run.minion.write.pwm,
                          model->run.minion.read.v0_10_adc);

    view_common_set_hidden(pdata->obj_blanket, !pdata->password);

    lv_label_set_text_fmt(pdata->label_wifi, "WiFi - %s", model->network.wifi_ipaddr);
    lv_label_set_text_fmt(pdata->label_ethernet, "Ethernet - %s", model->network.eth_ipaddr);
    switch (model->network.net_status) {
        case WIFI_INACTIVE:
            lv_label_set_text(pdata->label_state, "Non connesso");
            break;
        case WIFI_CONNECTED:
            lv_label_set_text_fmt(pdata->label_state, "Connesso a %s", model->network.ssid);
            break;
        case WIFI_CONNECTING:
            lv_label_set_text(pdata->label_state, "Connessione in corso...");
            break;

        default:
            break;
    }
}


static void close_page(void *state) {
    struct page_data *pdata = state;
    lv_obj_clean(lv_screen_active());
    pman_timer_pause(pdata->timer_wifi);
}


static void destroy_page(void *state, void *extra) {
    struct page_data *pdata = state;
    (void)extra;
    pman_timer_delete(pdata->timer_wifi);
    lv_free(state);
}


static void update_network_list(struct page_data *pdata, wifi_network_t *networks, int num) {
    lv_obj_clean(pdata->list_networks);

    for (int i = 0; i < num; i++) {
        char str[128];
        snprintf(str, sizeof(str), "     %s", networks[i].ssid);
        lv_obj_t *btn = lv_list_add_btn(pdata->list_networks, get_wifi_icon(networks[i].signal), str);
        view_register_object_default_callback(btn, BTN_WIFI_ID);
    }
}


static const void *get_wifi_icon(int signal) {
    LV_IMG_DECLARE(img_wifi_signal_1);
    LV_IMG_DECLARE(img_wifi_signal_2);
    LV_IMG_DECLARE(img_wifi_signal_3);
    LV_IMG_DECLARE(img_wifi_signal_max);
    if (signal > -65) {
        return &img_wifi_signal_max;
    } else if (signal > -70) {
        return &img_wifi_signal_3;
    } else if (signal > -75) {
        return &img_wifi_signal_2;
    } else {
        return &img_wifi_signal_1;
    }
}


const pman_page_t page_settings = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
