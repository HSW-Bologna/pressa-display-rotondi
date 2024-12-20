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
};

struct page_data {
    lv_obj_t *label_channels[PROGRAM_NUM_CHANNELS];

    uint16_t program_index;
};

static void update_page(model_t *model, struct page_data *pdata);

static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->program_index = (uint16_t)(uintptr_t)extra;

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
    // lv_obj_set_scroll_snap_y(bottom_container, LV_SCROLL_SNAP_START);
    lv_obj_remove_flag(bottom_container, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_scrollbar_mode(bottom_container, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_scroll_dir(bottom_container, LV_DIR_VER);
    lv_obj_set_style_bg_opa(bottom_container, LV_OPA_MAX, LV_STATE_DEFAULT | LV_PART_SCROLLBAR);

    lv_obj_t *left_panel = lv_obj_create(bottom_container);
    lv_obj_add_style(left_panel, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(left_panel, 100, LV_PCT(100));
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

    for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
        lv_obj_t *obj = lv_obj_create(left_panel);
        lv_obj_set_size(obj, 96, 20);
        lv_obj_t *label = lv_label_create(obj);
        lv_obj_center(label);
        pdata->label_channels[i] = label;
    }


    lv_obj_t *cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));

    view_common_back_button_create(lv_screen_active(), BTN_BACK_ID);

    update_page(model, pdata);
}

static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;
    (void)pdata;

    mut_model_t *model = view_get_model(handle);
    (void)model;

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

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    switch (view_get_obj_id(target)) {
                        case BTN_BACK_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
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
    // const program_t *program = model_get_program(model, pdata->program_index);
    for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
        lv_label_set_text(pdata->label_channels[i], model->config.channel_names[i]);
    }
}


static void close_page(void *state) {
    (void)state;
    lv_obj_clean(lv_scr_act());
}

const pman_page_t page_execution = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
