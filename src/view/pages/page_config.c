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
    BTN_PARMAC_ID,
    BTN_MOD_HEADGAP_OFFSET_UP_ID,
    BTN_MOD_HEADGAP_OFFSET_DOWN_ID,
};

struct page_data {
    lv_obj_t *label_headgap_offset_up;
    lv_obj_t *label_headgap_offset_down;

    uint8_t                 modified;
    view_page_program_arg_t page_program_arg;
};


static void      update_page(model_t *model, struct page_data *pdata);
static lv_obj_t *param_button_create(lv_obj_t *parent, uint16_t id, int16_t number);


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
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview);
    lv_obj_set_style_pad_left(tab_bar, 64, LV_STATE_DEFAULT);

    lv_obj_t *button = view_common_back_button_create(lv_screen_active(), BTN_BACK_ID);
    lv_obj_set_size(button, 64, 64);
    lv_obj_align(button, LV_ALIGN_TOP_LEFT, 0, 0);
    view_register_object_default_callback(button, BTN_BACK_ID);

    {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, "Programmi");
        {
            lv_obj_t *cont = lv_obj_create(tab);

            lv_obj_set_style_pad_column(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_size(cont, LV_PCT(68), LV_PCT(100));
            lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN_WRAP);
            lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_align(cont, LV_ALIGN_RIGHT_MID, 0, 0);

            for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
                lv_obj_t *button = lv_button_create(cont);
                lv_obj_t *label  = lv_label_create(button);
                lv_label_set_text(label, model->config.programs[i].name);
                view_register_object_default_callback_with_number(button, BTN_PROGRAM_ID, i);
            }
        }

        {
            lv_obj_t *cont = lv_obj_create(tab);
            lv_obj_set_size(cont, LV_PCT(28), LV_PCT(100));
            lv_obj_align(cont, LV_ALIGN_LEFT_MID, 0, 0);
        }
    }

    {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, "Parametri");

        lv_obj_t *parameter_cont = lv_obj_create(tab);
        lv_obj_set_size(parameter_cont, LV_PCT(100), LV_PCT(100));

        lv_obj_t *headgap_offset_cont = lv_obj_create(parameter_cont);
        lv_obj_set_layout(headgap_offset_cont, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(headgap_offset_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(headgap_offset_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_style(headgap_offset_cont, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_size(headgap_offset_cont, 400, 256);
        lv_obj_align(headgap_offset_cont, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *label = lv_label_create(headgap_offset_cont);
        lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_label_set_text(label, "Offset traferro");

        {
            lv_obj_t *obj = lv_obj_create(headgap_offset_cont);
            lv_obj_set_size(obj, LV_PCT(100), 90);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_style(obj, &style_transparent_cont, LV_STATE_DEFAULT);

            lv_obj_t *obj_label = lv_obj_create(obj);
            lv_obj_remove_flag(obj_label, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_size(obj_label, 128, 64);
            lv_obj_center(obj_label);
            lv_obj_t *label                = lv_label_create(obj_label);
            pdata->label_headgap_offset_up = label;

            {
                lv_obj_t *button = param_button_create(obj, BTN_MOD_HEADGAP_OFFSET_UP_ID, -1);
                lv_obj_align_to(button, obj_label, LV_ALIGN_OUT_LEFT_MID, -16, 0);
            }

            {
                lv_obj_t *button = param_button_create(obj, BTN_MOD_HEADGAP_OFFSET_UP_ID, +1);
                lv_obj_align_to(button, obj_label, LV_ALIGN_OUT_RIGHT_MID, 16, 0);
            }
        }

        {
            lv_obj_t *obj = lv_obj_create(headgap_offset_cont);
            lv_obj_set_size(obj, LV_PCT(100), 90);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_style(obj, &style_transparent_cont, LV_STATE_DEFAULT);

            lv_obj_t *obj_label = lv_obj_create(obj);
            lv_obj_remove_flag(obj_label, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_size(obj_label, 128, 64);
            lv_obj_center(obj_label);
            lv_obj_t *label                  = lv_label_create(obj_label);
            pdata->label_headgap_offset_down = label;

            {
                lv_obj_t *button = param_button_create(obj, BTN_MOD_HEADGAP_OFFSET_UP_ID, -1);
                lv_obj_align_to(button, obj_label, LV_ALIGN_OUT_LEFT_MID, -16, 0);
            }

            {
                lv_obj_t *button = param_button_create(obj, BTN_MOD_HEADGAP_OFFSET_UP_ID, +1);
                lv_obj_align_to(button, obj_label, LV_ALIGN_OUT_RIGHT_MID, 16, 0);
            }
        }
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
                            if (pdata->modified) {
                                view_get_protocol(handle)->save_configuration(handle);
                            }
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_PROGRAM_ID:
                            pdata->page_program_arg.modified      = &pdata->modified;
                            pdata->page_program_arg.program_index = view_get_obj_number(target);

                            msg.stack_msg =
                                PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_program, (void *)&pdata->page_program_arg);
                            break;

                        case BTN_MOD_HEADGAP_OFFSET_UP_ID:
                            pdata->modified = 1;
                            model->config.headgap_offset_up += view_get_obj_number(target);
                            model_check_parameters(model);
                            update_page(model, pdata);
                            break;

                        case BTN_MOD_HEADGAP_OFFSET_DOWN_ID:
                            pdata->modified = 1;
                            model->config.headgap_offset_down += view_get_obj_number(target);
                            model_check_parameters(model);
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
    lv_label_set_text_fmt(pdata->label_headgap_offset_up, "Sopra: %imm", model->config.headgap_offset_up);
    lv_label_set_text_fmt(pdata->label_headgap_offset_down, "Sotto: %imm", model->config.headgap_offset_down);
}


static void close_page(void *state) {
    (void)state;
    lv_obj_clean(lv_scr_act());
}


static lv_obj_t *param_button_create(lv_obj_t *parent, uint16_t id, int16_t number) {
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, 96, 64);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, number >= 0 ? LV_SYMBOL_PLUS : LV_SYMBOL_MINUS);
    lv_obj_center(label);
    view_register_object_default_callback_with_number(button, id, number);
    return button;
}


const pman_page_t page_config = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
