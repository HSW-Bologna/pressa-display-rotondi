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
    BTN_PROGRAM_ID,
};

struct page_data {};

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

    view_common_title_create(lv_screen_active(), BTN_BACK_ID, "Selezionare programma");

    lv_obj_t *cont = lv_obj_create(lv_screen_active());
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_HOR_RES - 16, LV_VER_RES - 64 - 16);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_pad_column(cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
        lv_obj_t *button = lv_button_create(cont);
        lv_obj_set_size(button, 240, 48);
        lv_obj_t *label  = lv_label_create(button);
        lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(label, LV_PCT(100));
        lv_label_set_text(label, model->config.programs[i].name);
        view_register_object_default_callback_with_number(button, BTN_PROGRAM_ID, i);
    }

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

                        case BTN_PROGRAM_ID:
                            model_set_current_program(model, view_get_obj_number(target));
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE(&page_execution);
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
    (void)model;
    (void)pdata;
}


static void close_page(void *state) {
    (void)state;
    lv_obj_clean(lv_scr_act());
}

const pman_page_t page_choice = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
