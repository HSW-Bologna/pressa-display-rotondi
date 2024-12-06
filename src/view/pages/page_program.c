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
#include "log.h"


enum {
    BTN_BACK_ID,
    BTN_DIGITAL_CHANNEL_1_ID,
    BTN_DIGITAL_CHANNEL_2_ID,
    BTN_DIGITAL_CHANNEL_3_ID,
    BTN_DIGITAL_CHANNEL_4_ID,
    BTN_DIGITAL_CHANNEL_5_ID,
    BTN_DIGITAL_CHANNEL_6_ID,
    BTN_DIGITAL_CHANNEL_7_ID,
    BTN_DIGITAL_CHANNEL_8_ID,
    BTN_DIGITAL_CHANNEL_9_ID,
    BTN_DIGITAL_CHANNEL_10_ID,
    BTN_DIGITAL_CHANNEL_11_ID,
    BTN_DIGITAL_CHANNEL_12_ID,
    BTN_DIGITAL_CHANNEL_13_ID,
    BTN_DIGITAL_CHANNEL_14_ID,
    LEFT_PANEL_SCROLL_ID,
    RIGHT_PANEL_SCROLL_ID,
};


struct page_data {
    lv_obj_t *label_digital_channels[PROGRAM_NUM_DIGITAL_CHANNELS];

    lv_obj_t *obj_digital_time_bars[PROGRAM_NUM_DIGITAL_CHANNELS];

    lv_obj_t *left_panel;
    lv_obj_t *right_panel;

    uint16_t channel_window_index;
    uint16_t program_index;
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->program_index        = (uint16_t)(uintptr_t)extra;
    pdata->channel_window_index = 0;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t         *model   = view_get_model(handle);
    const program_t *program = model_get_program(model, pdata->program_index);

    view_common_title_create(lv_screen_active(), BTN_BACK_ID, program->name);

    lv_obj_t *bottom_container = lv_obj_create(lv_screen_active());
    lv_obj_add_style(bottom_container, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(bottom_container, LV_PCT(100), LV_VER_RES - 64);
    lv_obj_align(bottom_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    //lv_obj_set_scroll_snap_y(bottom_container, LV_SCROLL_SNAP_START);
    lv_obj_remove_flag(bottom_container, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_scrollbar_mode(bottom_container, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_scroll_dir(bottom_container, LV_DIR_VER);
    lv_obj_set_style_bg_opa(bottom_container, LV_OPA_MAX, LV_STATE_DEFAULT | LV_PART_SCROLLBAR);

    lv_obj_t *left_panel = lv_obj_create(bottom_container);
    lv_obj_add_style(left_panel, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(left_panel, 100, (40 + 6) * PROGRAM_NUM_DIGITAL_CHANNELS);
    lv_obj_align(left_panel, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_remove_flag(left_panel, LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_pad_left(left_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(left_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(left_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(left_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(left_panel, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(left_panel, 6, LV_STATE_DEFAULT);
    lv_obj_set_layout(left_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (uint16_t i = 0; i < PROGRAM_NUM_DIGITAL_CHANNELS; i++) {
        lv_obj_t *button = lv_button_create(left_panel);
        lv_obj_add_flag(button, LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_remove_flag(button, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_size(button, 96, 40);
        lv_obj_t *label = lv_label_create(button);
        lv_obj_center(label);
        pdata->label_digital_channels[i] = label;
    }

    view_register_object_default_callback(left_panel, LEFT_PANEL_SCROLL_ID);
    pdata->left_panel = left_panel;

    lv_obj_t *right_panel = lv_obj_create(bottom_container);
    lv_obj_remove_flag(right_panel, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_style(right_panel, &style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(right_panel, LV_HOR_RES - 100, (40 + 6) * PROGRAM_NUM_DIGITAL_CHANNELS);
    lv_obj_align(right_panel, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_scrollbar_mode(right_panel, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_scroll_dir(right_panel, LV_DIR_HOR);
    lv_obj_set_style_bg_opa(right_panel, LV_OPA_MAX, LV_STATE_DEFAULT | LV_PART_SCROLLBAR);
    // lv_obj_set_scroll_snap_x(right_panel, LV_SCROLL_SNAP_START);
    lv_obj_remove_flag(right_panel, LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_pad_left(right_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(right_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(right_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(right_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(right_panel, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(right_panel, 6, LV_STATE_DEFAULT);
    lv_obj_set_layout(right_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    for (uint16_t i = 0; i < PROGRAM_NUM_DIGITAL_CHANNELS; i++) {
        lv_obj_t *time_bar = view_common_time_bar_create(right_panel, BTN_DIGITAL_CHANNEL_1_ID + i);
        lv_obj_remove_flag(time_bar, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_remove_flag(time_bar, LV_OBJ_FLAG_SNAPPABLE);
        pdata->obj_digital_time_bars[i] = time_bar;
    }

    view_register_object_default_callback(right_panel, RIGHT_PANEL_SCROLL_ID);
    pdata->right_panel = right_panel;

    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;

    mut_model_t *model = view_get_model(handle);

    switch (event.tag) {
        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_STORAGE_OPERATION_COMPLETED:
                    break;
                default:
                    break;
            }
            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t *target = lv_event_get_current_target_obj(event.as.lvgl);
            uint16_t  obj_id = view_get_obj_id(target);

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    switch (obj_id) {
                        case BTN_BACK_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_DIGITAL_CHANNEL_1_ID ... BTN_DIGITAL_CHANNEL_14_ID: {
                            program_t *program       = model_get_program_mut(model, pdata->program_index);
                            uint16_t   channel_index = obj_id - BTN_DIGITAL_CHANNEL_1_ID;
                            uint16_t   unit_index    = view_get_obj_number(target);

                            program_flip_digital_channel_state_at(program, channel_index, unit_index);
                            update_page(model, pdata);
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_SCROLL: {
                    switch (obj_id) {
                        case LEFT_PANEL_SCROLL_ID:
                            // lv_obj_scroll_to_y(pdata->right_panel, lv_obj_get_scroll_y(pdata->left_panel),
                            // LV_ANIM_OFF);
                            break;
                        case RIGHT_PANEL_SCROLL_ID:
                            // lv_obj_scroll_to_y(pdata->left_panel, lv_obj_get_scroll_y(pdata->right_panel),
                            // LV_ANIM_OFF);
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
    const program_t *program = model_get_program(model, pdata->program_index);

    for (uint16_t i = 0; i < PROGRAM_NUM_DIGITAL_CHANNELS; i++) {
        lv_label_set_text(pdata->label_digital_channels[i], model->config.channel_names[i]);

        for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
            lv_obj_t *unit = lv_obj_get_child(pdata->obj_digital_time_bars[i], j);
            if (program_get_digital_channel_state_at(program, i, j)) {
                lv_obj_set_style_bg_opa(unit, LV_OPA_100, LV_STATE_DEFAULT);
            } else {
                lv_obj_set_style_bg_opa(unit, LV_OPA_0, LV_STATE_DEFAULT);
            }
        }
    }
}


static void close_page(void *state) {
    (void)state;
    lv_obj_clean(lv_scr_act());
}


const pman_page_t page_program = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
