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
    BTN_PROGRAM_NAME_ID,
    BTN_CHANNEL_NAME_ID,
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
    BTN_DIGITAL_CHANNEL_15_ID,
    BTN_DIGITAL_CHANNEL_16_ID,
    BTN_PRESSURE_CHANNEL_ID,
    BTN_SENSOR_CHANNEL_ID,
    BTN_PARAMETER_ID,
    BTN_TIME_UNIT_MOD_ID,
    BTN_PRESSURE_LEVEL_1_MOD_ID,
    BTN_PRESSURE_LEVEL_2_MOD_ID,
    BTN_PRESSURE_LEVEL_3_MOD_ID,
    BTN_SENSOR_LEVEL_1_MOD_ID,
    BTN_SENSOR_LEVEL_2_MOD_ID,
    BTN_SENSOR_LEVEL_3_MOD_ID,
    KEYBOARD_ID,
    LEFT_PANEL_SCROLL_ID,
    RIGHT_PANEL_SCROLL_ID,
};

typedef enum {
    STATE_PROGRAM = 0,
    STATE_PARAMETERS,
    STATE_PROGRAM_NAME,
    STATE_CHANNEL_NAME,
} page_program_state_t;


struct page_data {
    lv_obj_t *label_channels[PROGRAM_NUM_PROGRAMMABLE_CHANNELS];

    lv_obj_t *obj_digital_time_bars[PROGRAM_NUM_PROGRAMMABLE_CHANNELS];
    lv_obj_t *obj_pressure_time_bar;
    lv_obj_t *obj_position_time_bar;
    lv_obj_t *obj_parameters;
    lv_obj_t *obj_blanket;

    lv_obj_t *button_parameters;

    lv_obj_t *label_program_name;
    lv_obj_t *label_parameters;
    lv_obj_t *label_time_unit;
    lv_obj_t *label_pressure_levels[PROGRAM_PRESSURE_LEVELS];
    lv_obj_t *label_position_levels[PROGRAM_SENSOR_LEVELS];

    lv_obj_t *textarea;

    lv_obj_t *keyboard;

    lv_obj_t *left_panel;
    lv_obj_t *right_panel;

    uint16_t                 channel_window_index;
    view_page_program_arg_t *arg;
    page_program_state_t     state;
    uint16_t                 channel_index;
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->arg                  = (view_page_program_arg_t *)extra;
    pdata->channel_window_index = 0;
    pdata->state                = STATE_PROGRAM;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t         *model   = view_get_model(handle);
    const program_t *program = model_get_program(model, pdata->arg->program_index);

    {     // Top bar
        lv_obj_t *obj_title = view_common_title_create(lv_screen_active(), BTN_BACK_ID, program->name);
        lv_obj_set_width(obj_title, LV_HOR_RES - 56 * 2 - 8);
        lv_obj_align(obj_title, LV_ALIGN_TOP_LEFT, 0, 0);
        pdata->label_program_name = lv_obj_get_child(obj_title, 1);

        {
            lv_obj_t *button = lv_button_create(lv_screen_active());
            lv_obj_set_size(button, 56, 56);
            lv_obj_t *label = lv_label_create(button);
            lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
            lv_label_set_text(label, LV_SYMBOL_EDIT);
            lv_obj_center(label);
            lv_obj_align(button, LV_ALIGN_TOP_RIGHT, -64, 4);
            view_register_object_default_callback(button, BTN_PROGRAM_NAME_ID);
        }
    }

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
    lv_obj_set_size(left_panel, 156, (46 + 6) * PROGRAM_NUM_PROGRAMMABLE_CHANNELS);
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

    for (uint16_t i = 0; i < PROGRAM_NUM_PROGRAMMABLE_CHANNELS; i++) {
        lv_obj_t *button = lv_button_create(left_panel);
        lv_obj_add_flag(button, LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_remove_flag(button, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_size(button, 152, 46);
        view_register_object_default_callback_with_number(button, BTN_CHANNEL_NAME_ID, i);
        lv_obj_t *label = lv_label_create(button);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(label, LV_PCT(100));
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_obj_center(label);
        pdata->label_channels[i] = label;
    }

    view_register_object_default_callback(left_panel, LEFT_PANEL_SCROLL_ID);
    pdata->left_panel = left_panel;

    lv_obj_t *right_panel = lv_obj_create(bottom_container);
    lv_obj_remove_flag(right_panel, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_style(right_panel, &style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(right_panel, LV_HOR_RES - 156, (46 + 6) * PROGRAM_NUM_PROGRAMMABLE_CHANNELS);
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

    for (uint16_t i = 0; i < PROGRAM_NUM_TIME_UNITS; i++) {
        lv_obj_t *line = lv_obj_create(right_panel);
        lv_obj_add_flag(line, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_set_size(line, 1, LV_PCT(100));

        lv_obj_set_style_border_color(line, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(line, 2, LV_STATE_DEFAULT);
        lv_obj_add_flag(line, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_align(line, LV_ALIGN_LEFT_MID, 46 * (i + 1) + i, 0);
    }

    for (uint16_t i = 0; i < PROGRAM_NUM_PROGRAMMABLE_CHANNELS; i++) {
        // Channel 8 should be the PRESSURE channel
        if (i == PROGRAM_PRESSURE_CHANNEL_INDEX) {
            lv_obj_t *time_bar = view_common_time_bar_create(right_panel, BTN_PRESSURE_CHANNEL_ID);
            lv_obj_remove_flag(time_bar, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
            lv_obj_remove_flag(time_bar, LV_OBJ_FLAG_SNAPPABLE);
            pdata->obj_pressure_time_bar    = time_bar;
            pdata->obj_digital_time_bars[i] = NULL;
        }
        // Channel 12 should be the 4-20ma channel
        else if (i == PROGRAM_SENSOR_CHANNEL_INDEX) {
            lv_obj_t *time_bar = view_common_time_bar_create(right_panel, BTN_SENSOR_CHANNEL_ID);
            lv_obj_remove_flag(time_bar, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
            lv_obj_remove_flag(time_bar, LV_OBJ_FLAG_SNAPPABLE);
            pdata->obj_position_time_bar    = time_bar;
            pdata->obj_digital_time_bars[i] = NULL;
        } else {
            lv_obj_t *time_bar = view_common_time_bar_create(right_panel, BTN_DIGITAL_CHANNEL_1_ID + i);
            lv_obj_remove_flag(time_bar, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
            lv_obj_remove_flag(time_bar, LV_OBJ_FLAG_SNAPPABLE);
            pdata->obj_digital_time_bars[i] = time_bar;
        }
    }

    {     // Parameters and names
        lv_obj_t *blanket = lv_obj_create(lv_screen_active());
        lv_obj_remove_flag(blanket, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(blanket, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(blanket, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(blanket, LV_OPA_50, LV_STATE_DEFAULT);
        lv_obj_set_size(blanket, LV_PCT(100), LV_PCT(100));
        lv_obj_center(blanket);
        pdata->obj_blanket = blanket;

        {
            lv_obj_t *button = lv_button_create(lv_screen_active());
            lv_obj_set_size(button, 56, 56);
            lv_obj_t *label = lv_label_create(button);
            lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
            lv_label_set_text(label, LV_SYMBOL_LEFT LV_SYMBOL_SETTINGS);
            lv_obj_center(label);
            pdata->label_parameters = label;
            view_register_object_default_callback(button, BTN_PARAMETER_ID);
            pdata->button_parameters = button;
        }

        {     // Name keyboard
            lv_obj_t *textarea = lv_textarea_create(blanket);
            lv_textarea_set_max_length(textarea, PROGRAM_NAME_LENGTH);
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

        lv_obj_t *cont = lv_obj_create(blanket);
        lv_obj_set_style_pad_all(cont, 0, LV_STATE_DEFAULT);
        lv_obj_set_size(cont, 600, LV_PCT(100));
        pdata->obj_parameters = cont;

        lv_obj_t *tabview = lv_tabview_create(cont);
        lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(100));
        lv_tabview_set_tab_bar_position(tabview, LV_DIR_RIGHT);

        {
            lv_obj_t *tab           = lv_tabview_add_tab(tabview, "Tempo");
            lv_obj_t *obj_time_unit = lv_obj_create(tab);
            lv_obj_set_size(obj_time_unit, 160, 64);
            lv_obj_remove_flag(obj_time_unit, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(obj_time_unit, LV_ALIGN_TOP_MID, 0, 32);
            lv_obj_t *label_time_unit = lv_label_create(obj_time_unit);
            lv_obj_set_style_text_font(label_time_unit, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
            pdata->label_time_unit = label_time_unit;

            {
                lv_obj_t *button = lv_button_create(tab);
                lv_obj_set_size(button, 96, 64);
                lv_obj_t *label = lv_label_create(button);
                lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
                lv_label_set_text(label, LV_SYMBOL_MINUS);
                lv_obj_center(label);
                lv_obj_align_to(button, obj_time_unit, LV_ALIGN_OUT_LEFT_MID, -16, 0);
                view_register_object_default_callback_with_number(button, BTN_TIME_UNIT_MOD_ID, -1);
            }

            {
                lv_obj_t *button = lv_button_create(tab);
                lv_obj_set_size(button, 96, 64);
                lv_obj_t *label = lv_label_create(button);
                lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
                lv_label_set_text(label, LV_SYMBOL_PLUS);
                lv_obj_center(label);
                lv_obj_align_to(button, obj_time_unit, LV_ALIGN_OUT_RIGHT_MID, 16, 0);
                view_register_object_default_callback_with_number(button, BTN_TIME_UNIT_MOD_ID, +1);
            }
        }

        {
            lv_obj_t *tab = lv_tabview_add_tab(tabview, "Pressione");

            lv_obj_t *obj_pressure_levels = lv_obj_create(tab);
            lv_obj_set_size(obj_pressure_levels, LV_PCT(100), LV_PCT(100));
            lv_obj_align(obj_pressure_levels, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_style(obj_pressure_levels, &style_transparent_cont, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj_pressure_levels, 4, LV_STATE_DEFAULT);
            lv_obj_set_layout(obj_pressure_levels, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(obj_pressure_levels, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(obj_pressure_levels, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);

            for (uint16_t i = 0; i < PROGRAM_PRESSURE_LEVELS; i++) {
                lv_obj_t *obj = lv_obj_create(obj_pressure_levels);
                lv_obj_set_size(obj, LV_PCT(100), 90);
                lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_add_style(obj, &style_transparent_cont, LV_STATE_DEFAULT);

                lv_obj_t *obj_label = lv_obj_create(obj);
                lv_obj_remove_flag(obj_label, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_size(obj_label, 160, 64);
                lv_obj_center(obj_label);
                lv_obj_t *label = lv_label_create(obj_label);
                lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
                pdata->label_pressure_levels[i] = label;

                {
                    lv_obj_t *button = lv_button_create(obj);
                    lv_obj_set_size(button, 96, 64);
                    lv_obj_t *label = lv_label_create(button);
                    lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
                    lv_label_set_text(label, LV_SYMBOL_MINUS);
                    lv_obj_center(label);
                    lv_obj_align_to(button, obj_label, LV_ALIGN_OUT_LEFT_MID, -16, 0);
                    view_register_object_default_callback_with_number(button, BTN_PRESSURE_LEVEL_1_MOD_ID + i, -1);
                }

                {
                    lv_obj_t *button = lv_button_create(obj);
                    lv_obj_set_size(button, 96, 64);
                    lv_obj_t *label = lv_label_create(button);
                    lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
                    lv_label_set_text(label, LV_SYMBOL_PLUS);
                    lv_obj_center(label);
                    lv_obj_align_to(button, obj_label, LV_ALIGN_OUT_RIGHT_MID, 16, 0);
                    view_register_object_default_callback_with_number(button, BTN_PRESSURE_LEVEL_1_MOD_ID + i, +1);
                }
            }
        }

        {
            lv_obj_t *tab = lv_tabview_add_tab(tabview, "Livello");

            lv_obj_t *obj_position_levels = lv_obj_create(tab);
            lv_obj_set_size(obj_position_levels, LV_PCT(100), LV_PCT(100));
            lv_obj_align(obj_position_levels, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_style(obj_position_levels, &style_transparent_cont, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj_position_levels, 4, LV_STATE_DEFAULT);
            lv_obj_set_layout(obj_position_levels, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(obj_position_levels, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(obj_position_levels, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);

            for (uint16_t i = 0; i < PROGRAM_SENSOR_LEVELS; i++) {
                lv_obj_t *obj = lv_obj_create(obj_position_levels);
                lv_obj_set_size(obj, LV_PCT(100), 90);
                lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_add_style(obj, &style_transparent_cont, LV_STATE_DEFAULT);

                lv_obj_t *obj_label = lv_obj_create(obj);
                lv_obj_remove_flag(obj_label, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_size(obj_label, 160, 64);
                lv_obj_center(obj_label);
                lv_obj_t *label = lv_label_create(obj_label);
                lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
                pdata->label_position_levels[i] = label;

                {
                    lv_obj_t *button = lv_button_create(obj);
                    lv_obj_set_size(button, 96, 64);
                    lv_obj_t *label = lv_label_create(button);
                    lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
                    lv_label_set_text(label, LV_SYMBOL_MINUS);
                    lv_obj_center(label);
                    lv_obj_align_to(button, obj_label, LV_ALIGN_OUT_LEFT_MID, -16, 0);
                    view_register_object_default_callback_with_number(button, BTN_SENSOR_LEVEL_1_MOD_ID + i, -1);
                }

                {
                    lv_obj_t *button = lv_button_create(obj);
                    lv_obj_set_size(button, 96, 64);
                    lv_obj_t *label = lv_label_create(button);
                    lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
                    lv_label_set_text(label, LV_SYMBOL_PLUS);
                    lv_obj_center(label);
                    lv_obj_align_to(button, obj_label, LV_ALIGN_OUT_RIGHT_MID, 16, 0);
                    view_register_object_default_callback_with_number(button, BTN_SENSOR_LEVEL_1_MOD_ID + i, +1);
                }
            }
        }
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
            lv_obj_t *target     = lv_event_get_current_target_obj(event.as.lvgl);
            uint16_t  obj_id     = view_get_obj_id(target);
            int16_t   obj_number = view_get_obj_number(target);

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    switch (obj_id) {
                        case BTN_BACK_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_PROGRAM_NAME_ID: {
                            const program_t *program = model_get_program(model, pdata->arg->program_index);
                            pdata->state             = STATE_PROGRAM_NAME;
                            lv_obj_send_event(pdata->textarea, LV_EVENT_FOCUSED, NULL);
                            lv_textarea_set_text(pdata->textarea, program->name);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_CHANNEL_NAME_ID: {
                            pdata->state = STATE_CHANNEL_NAME;
                            lv_obj_send_event(pdata->textarea, LV_EVENT_FOCUSED, NULL);
                            pdata->channel_index = view_get_obj_number(target);
                            lv_textarea_set_text(pdata->textarea, model->config.channel_names[pdata->channel_index]);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_PARAMETER_ID:
                            switch (pdata->state) {
                                case STATE_PROGRAM:
                                    pdata->state = STATE_PARAMETERS;
                                    break;
                                default:
                                    pdata->state = STATE_PROGRAM;
                                    break;
                            }
                            update_page(model, pdata);
                            break;

                        case BTN_TIME_UNIT_MOD_ID: {
                            program_t *program = model_get_program_mut(model, pdata->arg->program_index);
                            program->time_unit_decisecs += obj_number;
                            program_check_parameters(program, model->config.position_sensor_scale_mm);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_PRESSURE_LEVEL_1_MOD_ID:
                        case BTN_PRESSURE_LEVEL_2_MOD_ID:
                        case BTN_PRESSURE_LEVEL_3_MOD_ID: {
                            uint16_t   pressure_level_index = obj_id - BTN_PRESSURE_LEVEL_1_MOD_ID;
                            program_t *program              = model_get_program_mut(model, pdata->arg->program_index);
                            program->pressure_levels[pressure_level_index] += obj_number;
                            program_check_parameters(program, model->config.position_sensor_scale_mm);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_SENSOR_LEVEL_1_MOD_ID:
                        case BTN_SENSOR_LEVEL_2_MOD_ID:
                        case BTN_SENSOR_LEVEL_3_MOD_ID: {
                            uint16_t   position_level_index = obj_id - BTN_SENSOR_LEVEL_1_MOD_ID;
                            program_t *program              = model_get_program_mut(model, pdata->arg->program_index);
                            program->position_levels[position_level_index] += obj_number * 1;
                            program_check_parameters(program, model->config.position_sensor_scale_mm);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_DIGITAL_CHANNEL_1_ID ... BTN_DIGITAL_CHANNEL_16_ID: {
                            *pdata->arg->modified    = 1;
                            program_t *program       = model_get_program_mut(model, pdata->arg->program_index);
                            uint16_t   channel_index = obj_id - BTN_DIGITAL_CHANNEL_1_ID;
                            uint16_t   unit_index    = view_get_obj_number(target);

                            program_flip_digital_channel_state_at(program, channel_index, unit_index);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_PRESSURE_CHANNEL_ID: {
                            *pdata->arg->modified = 1;
                            program_t *program    = model_get_program_mut(model, pdata->arg->program_index);
                            uint16_t   unit_index = view_get_obj_number(target);

                            program_increase_pressure_channel_state_at(program, unit_index);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_SENSOR_CHANNEL_ID: {
                            *pdata->arg->modified = 1;
                            program_t *program    = model_get_program_mut(model, pdata->arg->program_index);
                            uint16_t   unit_index = view_get_obj_number(target);

                            program_increase_sensor_channel_threshold_at(program, unit_index);
                            update_page(model, pdata);
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_LONG_PRESSED_REPEAT: {
                    switch (obj_id) {
                        case BTN_TIME_UNIT_MOD_ID: {
                            program_t *program = model_get_program_mut(model, pdata->arg->program_index);
                            program->time_unit_decisecs += obj_number;
                            program_check_parameters(program, model->config.position_sensor_scale_mm);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_PRESSURE_LEVEL_1_MOD_ID:
                        case BTN_PRESSURE_LEVEL_2_MOD_ID:
                        case BTN_PRESSURE_LEVEL_3_MOD_ID: {
                            uint16_t   pressure_level_index = obj_id - BTN_PRESSURE_LEVEL_1_MOD_ID;
                            program_t *program              = model_get_program_mut(model, pdata->arg->program_index);
                            program->pressure_levels[pressure_level_index] += obj_number * 10;
                            program->pressure_levels[pressure_level_index] -=
                                program->pressure_levels[pressure_level_index] % 10;
                            program_check_parameters(program, model->config.position_sensor_scale_mm);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_SENSOR_LEVEL_1_MOD_ID:
                        case BTN_SENSOR_LEVEL_2_MOD_ID:
                        case BTN_SENSOR_LEVEL_3_MOD_ID: {
                            uint16_t   position_level_index = obj_id - BTN_SENSOR_LEVEL_1_MOD_ID;
                            program_t *program              = model_get_program_mut(model, pdata->arg->program_index);
                            program->position_levels[position_level_index] += obj_number * 10;
                            program->position_levels[position_level_index] -=
                                program->position_levels[position_level_index] % 10;
                            program_check_parameters(program, model->config.position_sensor_scale_mm);
                            update_page(model, pdata);
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_READY: {
                    switch (obj_id) {
                        case KEYBOARD_ID: {
                            switch (pdata->state) {
                                case STATE_PROGRAM_NAME: {
                                    program_t *program = model_get_program_mut(model, pdata->arg->program_index);
                                    snprintf(program->name, sizeof(program->name), "%s",
                                             lv_textarea_get_text(pdata->textarea));
                                    break;
                                }

                                case STATE_CHANNEL_NAME: {
                                    snprintf(model->config.channel_names[pdata->channel_index],
                                             sizeof(model->config.channel_names[pdata->channel_index]), "%s",
                                             lv_textarea_get_text(pdata->textarea));
                                    break;
                                }

                                default:
                                    break;
                            }

                            pdata->state = STATE_PROGRAM;
                            update_page(model, pdata);
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_CANCEL: {
                    switch (obj_id) {
                        case KEYBOARD_ID: {
                            pdata->state = STATE_PROGRAM;
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
    const program_t *program = model_get_program(model, pdata->arg->program_index);

    for (uint16_t i = 0; i < PROGRAM_NUM_PROGRAMMABLE_CHANNELS; i++) {
        lv_label_set_text(pdata->label_channels[i], model->config.channel_names[i]);
    }

    for (uint16_t i = 0; i < PROGRAM_NUM_PROGRAMMABLE_CHANNELS; i++) {
        for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
            if (pdata->obj_digital_time_bars[i] != NULL) {
                lv_obj_t *unit = lv_obj_get_child(pdata->obj_digital_time_bars[i], j);
                if (program_get_digital_channel_state_at(program, i, j)) {
                    lv_obj_set_style_bg_opa(unit, LV_OPA_100, LV_STATE_DEFAULT);
                } else {
                    lv_obj_set_style_bg_opa(unit, LV_OPA_0, LV_STATE_DEFAULT);
                }
            }
        }
    }

    const lv_color_t colors[4] = {STYLE_COLOR_BLUE, STYLE_COLOR_GREEN, STYLE_COLOR_YELLOW, STYLE_COLOR_RED};
    for (uint16_t j = 0; j < PROGRAM_NUM_TIME_UNITS; j++) {
        lv_obj_t                        *pressure_unit          = lv_obj_get_child(pdata->obj_pressure_time_bar, j);
        program_pressure_channel_state_t pressure_channel_state = program_get_pressure_channel_state_at(program, j);

        if (pressure_channel_state != PROGRAM_PRESSURE_CHANNEL_STATE_OFF) {
            lv_obj_set_style_bg_opa(pressure_unit, LV_OPA_100, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(pressure_unit, colors[pressure_channel_state], LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_bg_opa(pressure_unit, LV_OPA_0, LV_STATE_DEFAULT);
        }

        lv_obj_t                          *position_unit = lv_obj_get_child(pdata->obj_position_time_bar, j);
        program_sensor_channel_threshold_t position_channel_threshold = program_get_sensor_channel_state_at(program, j);

        if (position_channel_threshold != PROGRAM_SENSOR_CHANNEL_THRESHOLD_NONE) {
            lv_obj_set_style_bg_opa(position_unit, LV_OPA_100, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(position_unit, colors[position_channel_threshold], LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_bg_opa(position_unit, LV_OPA_0, LV_STATE_DEFAULT);
        }
    }

    switch (pdata->state) {
        case STATE_PROGRAM:
            view_common_set_hidden(pdata->obj_blanket, 1);
            lv_obj_align(pdata->obj_parameters, LV_ALIGN_RIGHT_MID, 600, 0);
            lv_obj_align_to(pdata->button_parameters, pdata->obj_parameters, LV_ALIGN_OUT_LEFT_TOP, 0, 4);
            lv_label_set_text(pdata->label_parameters, LV_SYMBOL_LEFT LV_SYMBOL_SETTINGS);
            lv_label_set_text(pdata->label_program_name, program->name);
            break;

        case STATE_PARAMETERS:
            view_common_set_hidden(pdata->obj_blanket, 0);
            view_common_set_hidden(pdata->textarea, 1);
            view_common_set_hidden(pdata->keyboard, 1);
            lv_obj_align(pdata->obj_parameters, LV_ALIGN_RIGHT_MID, 0, 0);
            lv_obj_align_to(pdata->button_parameters, pdata->obj_parameters, LV_ALIGN_OUT_LEFT_TOP, 0, 4);
            lv_label_set_text(pdata->label_parameters, LV_SYMBOL_SETTINGS LV_SYMBOL_RIGHT);

            lv_label_set_text_fmt(pdata->label_time_unit, "%.1f s", ((float)program->time_unit_decisecs) / 10.);
            for (uint16_t i = 0; i < PROGRAM_PRESSURE_LEVELS; i++) {
                lv_label_set_text_fmt(pdata->label_pressure_levels[i], "%.1f bar",
                                      ((float)program->pressure_levels[i]) / 10.);
            }
            for (uint16_t i = 0; i < PROGRAM_SENSOR_LEVELS; i++) {
                lv_label_set_text_fmt(pdata->label_position_levels[i], "%i mm", program->position_levels[i]);
            }
            break;

        case STATE_PROGRAM_NAME:
        case STATE_CHANNEL_NAME:
            view_common_set_hidden(pdata->obj_blanket, 0);
            view_common_set_hidden(pdata->textarea, 0);
            view_common_set_hidden(pdata->keyboard, 0);
            lv_obj_align(pdata->obj_parameters, LV_ALIGN_RIGHT_MID, 600, 0);
            lv_obj_align_to(pdata->button_parameters, pdata->obj_parameters, LV_ALIGN_OUT_LEFT_TOP, 0, 4);
            lv_label_set_text(pdata->label_parameters, LV_SYMBOL_LEFT LV_SYMBOL_SETTINGS);
            break;
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
