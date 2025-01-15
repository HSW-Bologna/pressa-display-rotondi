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
    BTN_COMMUNICATION_ID,
    BTN_COPY_ID,
    BTN_EXPORT_ID,
    KEYBOARD_ID,
};

struct page_data {
    lv_obj_t *obj_blanket;

    lv_obj_t *label_headgap_offset_up;
    lv_obj_t *label_headgap_offset_down;
    lv_obj_t *label_operation;

    lv_obj_t *button_copy;
    lv_obj_t *button_import;
    lv_obj_t *button_export;
    lv_obj_t *buttons_programs[NUM_PROGRAMS];

    lv_obj_t *textarea;

    lv_obj_t *keyboard;

    lv_obj_t *label_status;

    lv_obj_t *led_communication;

    uint8_t                 modified;
    view_page_program_arg_t page_program_arg;

    enum {
        OPERATION_NONE = 0,
        OPERATION_COPY,
        OPERATION_IMPORT,
        OPERATION_EXPORT,
    } operation;
    int16_t program_index;
};


static void      update_page(model_t *model, struct page_data *pdata);
static lv_obj_t *param_button_create(lv_obj_t *parent, uint16_t id, int16_t number);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->operation     = OPERATION_NONE;
    pdata->program_index = -1;

    return pdata;
}

static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    lv_obj_t *tabview = lv_tabview_create(lv_screen_active());
    lv_obj_set_style_text_font(tabview, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview);
    lv_obj_set_style_pad_left(tab_bar, 64, LV_STATE_DEFAULT);

    lv_obj_t *button = view_common_back_button_create(lv_screen_active(), BTN_BACK_ID);
    lv_obj_set_size(button, 64, 64);
    lv_obj_align(button, LV_ALIGN_TOP_LEFT, 0, 0);
    view_register_object_default_callback(button, BTN_BACK_ID);

    {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, "Programmi");
        lv_obj_set_style_text_font(tab, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_ver(tab, 10, LV_STATE_DEFAULT);

        {
            lv_obj_t *cont = lv_obj_create(tab);

            lv_obj_set_style_pad_ver(cont, 4, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_hor(cont, 4, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_column(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(cont, 6, LV_STATE_DEFAULT);
            lv_obj_set_size(cont, LV_PCT(66), LV_PCT(100));
            lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_align(cont, LV_ALIGN_RIGHT_MID, 0, 0);

            for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
                lv_obj_t *button = lv_button_create(cont);
                lv_obj_set_size(button, 152, 48);
                lv_obj_add_flag(button, LV_OBJ_FLAG_CHECKABLE);
                lv_obj_t *label = lv_label_create(button);
                lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
                lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_width(label, LV_PCT(100));
                lv_obj_center(label);
                view_register_object_default_callback_with_number(button, BTN_PROGRAM_ID, i);

                pdata->buttons_programs[i] = button;
            }
        }

        {     // Left button panel
            lv_obj_t *cont = lv_obj_create(tab);
            lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_size(cont, LV_PCT(32), LV_PCT(100));
            lv_obj_align(cont, LV_ALIGN_LEFT_MID, 0, 0);

            lv_obj_t *button_cont = lv_obj_create(cont);
            lv_obj_add_style(button_cont, &style_transparent_cont, LV_STATE_DEFAULT);
            lv_obj_set_layout(button_cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(button_cont, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_flex_align(button_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_all(button_cont, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_column(button_cont, 8, LV_STATE_DEFAULT);
            lv_obj_set_width(button_cont, LV_PCT(100));
            lv_obj_align(button_cont, LV_ALIGN_TOP_MID, 0, 0);

            {     // Copy button
                lv_obj_t *button = lv_button_create(button_cont);
                lv_obj_add_flag(button, LV_OBJ_FLAG_CHECKABLE);
                lv_obj_set_size(button, 56, 56);
                lv_obj_t *label = lv_label_create(button);
                lv_obj_center(label);
                lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
                lv_label_set_text(label, LV_SYMBOL_COPY);
                view_register_object_default_callback(button, BTN_COPY_ID);
                pdata->button_copy = button;
            }

            {     // Import button
                lv_obj_t *button = lv_button_create(button_cont);
                lv_obj_add_flag(button, LV_OBJ_FLAG_CHECKABLE);
                lv_obj_set_size(button, 56, 56);
                lv_obj_t *label = lv_label_create(button);
                lv_obj_center(label);
                lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
                view_register_object_default_callback(button, BTN_EXPORT_ID);
                lv_label_set_text(label, LV_SYMBOL_DOWNLOAD);
                pdata->button_export = button;
            }

            {     // Export button
                lv_obj_t *button = lv_button_create(button_cont);
                lv_obj_add_flag(button, LV_OBJ_FLAG_CHECKABLE);
                lv_obj_set_size(button, 56, 56);
                lv_obj_t *label = lv_label_create(button);
                lv_obj_center(label);
                lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
                lv_label_set_text(label, LV_SYMBOL_EJECT);
                pdata->button_import = button;
            }

            lv_obj_t *label = lv_label_create(cont);
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(label, LV_PCT(100));
            pdata->label_status = label;
        }
    }

    {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, "Parametri");
        lv_obj_set_style_text_font(tab, STYLE_FONT_SMALL, LV_STATE_DEFAULT);

        {     // Headgap offset
            lv_obj_t *headgap_offset_cont = lv_obj_create(tab);
            lv_obj_set_style_pad_hor(headgap_offset_cont, 8, LV_STATE_DEFAULT);
            lv_obj_set_layout(headgap_offset_cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(headgap_offset_cont, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(headgap_offset_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_set_size(headgap_offset_cont, 390, 256);
            lv_obj_align(headgap_offset_cont, LV_ALIGN_TOP_LEFT, 0, 0);
            lv_obj_remove_flag(headgap_offset_cont, LV_OBJ_FLAG_SCROLLABLE);

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

        {     // Communication options
            lv_obj_t *cont_com = lv_obj_create(tab);
            lv_obj_set_layout(cont_com, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cont_com, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(cont_com, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_size(cont_com, 350, 200);
            lv_obj_align(cont_com, LV_ALIGN_TOP_RIGHT, 0, 0);

            {
                lv_obj_t *button = lv_button_create(cont_com);
                lv_obj_set_layout(button, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(button, LV_FLEX_FLOW_ROW);
                lv_obj_set_flex_align(button, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                lv_obj_set_size(button, 300, 80);
                view_register_object_default_callback(button, BTN_COMMUNICATION_ID);

                lv_obj_t *label = lv_label_create(button);
                lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
                lv_label_set_text(label, "Comunicazione");

                lv_obj_t *led = lv_led_create(button);
                lv_led_set_color(led, STYLE_COLOR_GREEN);
                lv_obj_set_size(led, 48, 48);
                pdata->led_communication = led;
            }
        }
    }

    {     // Export name input
        lv_obj_t *blanket = lv_obj_create(lv_screen_active());
        lv_obj_remove_flag(blanket, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(blanket, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(blanket, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(blanket, LV_OPA_50, LV_STATE_DEFAULT);
        lv_obj_set_size(blanket, LV_PCT(100), LV_PCT(100));
        lv_obj_center(blanket);
        pdata->obj_blanket = blanket;

        lv_obj_t *textarea = lv_textarea_create(blanket);
        lv_obj_set_style_text_font(textarea, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_obj_set_size(textarea, 480, 64);
        lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 80);
        pdata->textarea = textarea;

        lv_obj_t *keyboard = lv_keyboard_create(blanket);
        lv_keyboard_set_textarea(keyboard, textarea);
        lv_obj_set_size(keyboard, LV_PCT(100), 300);
        lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        view_register_object_default_callback(keyboard, KEYBOARD_ID);
        pdata->keyboard = keyboard;
    }

    VIEW_ADD_WATCHED_VARIABLE(&model->run.drive_mounted, 0);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.num_importable_configurations, 0);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.importable_configurations, 0);

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
                case VIEW_EVENT_TAG_PAGE_WATCHER:
                    update_page(model, pdata);
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

                        case BTN_PROGRAM_ID: {
                            uint16_t program_index = view_get_obj_number(target);

                            if (pdata->operation == OPERATION_COPY) {
                                // No source program was selected yet
                                if (pdata->program_index < 0) {
                                    pdata->program_index = program_index;
                                }
                                // Selecting destination program
                                else {
                                    model_copy_program(model, pdata->program_index, program_index);
                                    pdata->operation = OPERATION_NONE;
                                }

                                update_page(model, pdata);
                            } else {
                                pdata->page_program_arg.modified      = &pdata->modified;
                                pdata->page_program_arg.program_index = program_index;

                                msg.stack_msg =
                                    PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_program, (void *)&pdata->page_program_arg);
                            }
                            break;
                        }

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

                        case BTN_COMMUNICATION_ID:
                            model->run.minion.communication_enabled = !model->run.minion.communication_enabled;
                            update_page(model, pdata);
                            break;

                        case BTN_COPY_ID:
                            pdata->operation = OPERATION_COPY;
                            update_page(model, pdata);
                            break;

                        case BTN_EXPORT_ID:
                            pdata->operation = OPERATION_EXPORT;
                            lv_textarea_set_text(pdata->textarea, "configurazione");
                            update_page(model, pdata);
                            break;

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_READY: {
                    switch (view_get_obj_id(target)) {
                        case KEYBOARD_ID: {
                            view_get_protocol(handle)->export_configuration(handle,
                                                                            lv_textarea_get_text(pdata->textarea));
                            pdata->operation = OPERATION_NONE;
                            update_page(model, pdata);
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_CANCEL: {
                    switch (view_get_obj_id(target)) {
                        case KEYBOARD_ID: {
                            pdata->operation = OPERATION_NONE;
                            update_page(model, pdata);
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

    if (model->run.minion.communication_enabled) {
        lv_led_on(pdata->led_communication);
    } else {
        lv_led_off(pdata->led_communication);
    }

    for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
        lv_label_set_text(lv_obj_get_child(pdata->buttons_programs[i], 0), model->config.programs[i].name);
    }

    switch (pdata->operation) {
        case OPERATION_COPY:
            if (pdata->program_index < 0) {
                lv_label_set_text(pdata->label_status, "Selezionare un programma da copiare");
            } else {
                lv_label_set_text(pdata->label_status, "Selezionare la posizione da sovrascrivere");
            }

            for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
                if (i == pdata->program_index) {
                    lv_obj_add_state(pdata->buttons_programs[i], LV_STATE_CHECKED);
                } else {
                    lv_obj_remove_state(pdata->buttons_programs[i], LV_STATE_CHECKED);
                }
            }

            view_common_set_hidden(pdata->label_status, 0);
            lv_obj_add_state(pdata->button_copy, LV_STATE_CHECKED);
            lv_obj_remove_state(pdata->button_import, LV_STATE_CHECKED);
            lv_obj_remove_state(pdata->button_export, LV_STATE_CHECKED);
            view_common_set_hidden(pdata->obj_blanket, 1);
            break;

        case OPERATION_EXPORT: {
            view_common_set_hidden(pdata->obj_blanket, 0);
            break;
        }

        default:
            for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
                lv_obj_remove_state(pdata->buttons_programs[i], LV_STATE_CHECKED);
            }

            view_common_set_hidden(pdata->label_status, 1);
            view_common_set_hidden(pdata->obj_blanket, 1);
            lv_obj_remove_state(pdata->button_copy, LV_STATE_CHECKED);
            lv_obj_remove_state(pdata->button_import, LV_STATE_CHECKED);
            lv_obj_remove_state(pdata->button_export, LV_STATE_CHECKED);
            break;
    }

    if (model->run.drive_mounted) {
        lv_obj_remove_state(pdata->button_export, LV_STATE_DISABLED);

        if (model->run.num_importable_configurations == 0) {
            lv_obj_add_state(pdata->button_import, LV_STATE_DISABLED);
        } else {
            lv_obj_remove_state(pdata->button_import, LV_STATE_DISABLED);
        }
    } else {
        lv_obj_add_state(pdata->button_import, LV_STATE_DISABLED);
        lv_obj_add_state(pdata->button_export, LV_STATE_DISABLED);
        lv_obj_remove_state(pdata->button_import, LV_STATE_CHECKED);
        lv_obj_remove_state(pdata->button_export, LV_STATE_CHECKED);
    }
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
