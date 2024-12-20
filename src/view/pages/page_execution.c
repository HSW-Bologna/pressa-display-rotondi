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


#define CHANNEL_ROW_PADDING 2
#define CHANNEL_ROW_HEIGHT  23
#define TIME_UNIT_WIDTH     40


enum {
    BTN_BACK_ID,
};

struct page_data {
    lv_obj_t *time_bar;

    uint16_t program_index;
};


static void      update_page(model_t *model, struct page_data *pdata);
static void      configuration_from_digital_channel(uint8_t                           *channel_configuration,
                                                    program_digital_channel_schedule_t digital_channel_schedule);
static void      configuration_from_dac_channel(uint8_t                     *channel_configuration,
                                                program_dac_channel_state_t *dac_channel_states);
static void      configuration_from_sensor_channel(uint8_t                            *channel_configuration,
                                                   program_sensor_channel_threshold_t *dac_channel_thresholds);
static lv_obj_t *channel_schedule_create(lv_obj_t *parent, uint8_t *channel_configuration, uint8_t digital);


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
    lv_obj_set_style_border_width(bottom_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_ver(bottom_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(bottom_container, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(bottom_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(bottom_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(bottom_container, LV_PCT(100), LV_VER_RES - 64);
    lv_obj_align(bottom_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_remove_flag(bottom_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(bottom_container, LV_OPA_MAX, LV_STATE_DEFAULT | LV_PART_SCROLLBAR);
    lv_obj_set_layout(bottom_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *left_panel = lv_obj_create(bottom_container);
    lv_obj_add_style(left_panel, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(left_panel, 100, LV_PCT(100));
    lv_obj_align(left_panel, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_remove_flag(left_panel, LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_pad_all(left_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(left_panel, CHANNEL_ROW_PADDING, LV_STATE_DEFAULT);
    lv_obj_set_layout(left_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
        lv_obj_t *obj = lv_obj_create(left_panel);
        lv_obj_set_style_radius(obj, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_TOP,
                                     LV_STATE_DEFAULT);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(obj, LV_PCT(100), CHANNEL_ROW_HEIGHT);
        lv_obj_t *label = lv_label_create(obj);
        lv_obj_center(label);
        lv_label_set_text(label, model->config.channel_names[i]);
    }

    lv_obj_t *right_panel = lv_obj_create(bottom_container);
    lv_obj_set_height(right_panel, LV_PCT(100));
    lv_obj_set_flex_grow(right_panel, 1);
    lv_obj_remove_flag(right_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(right_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(right_panel, CHANNEL_ROW_PADDING, LV_STATE_DEFAULT);
    lv_obj_set_layout(right_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    uint8_t channel_configuration[PROGRAM_NUM_TIME_UNITS] = {0};
    for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
        // Channel 8 should be the DAC channel
        if (i == PROGRAM_DAC_CHANNEL_INDEX) {
            configuration_from_dac_channel(channel_configuration, (program_dac_channel_state_t *)program->dac_channel);
            channel_schedule_create(right_panel, channel_configuration, 0);
        }
        // Channel 12 should be the 4-20ma channel
        else if (i == PROGRAM_SENSOR_CHANNEL_INDEX) {
            configuration_from_sensor_channel(channel_configuration,
                                              (program_sensor_channel_threshold_t *)program->sensor_channel);
            channel_schedule_create(right_panel, channel_configuration, 0);
        }
        // Every other is a digital channel
        else {
            uint16_t digital_index = i;
            if (i > PROGRAM_SENSOR_CHANNEL_INDEX) {
                digital_index -= 2;
            } else if (i > PROGRAM_DAC_CHANNEL_INDEX) {
                digital_index -= 1;
            }

            configuration_from_digital_channel(channel_configuration, program->digital_channels[digital_index]);
            channel_schedule_create(right_panel, channel_configuration, 1);
        }
    }

    lv_obj_t *time_bar = lv_obj_create(right_panel);
    lv_obj_add_flag(time_bar, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_style_radius(time_bar, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(time_bar, 8, LV_PCT(100));
    lv_obj_set_style_bg_color(time_bar, STYLE_COLOR_CYAN, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(time_bar, lv_color_darken(STYLE_COLOR_CYAN, LV_OPA_10), LV_STATE_DEFAULT);
    lv_obj_set_style_opa(time_bar, LV_OPA_70, LV_STATE_DEFAULT);
    pdata->time_bar = time_bar;

    update_page(model, pdata);

    log_info("Open");
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
    const program_t *program = model_get_program(model, pdata->program_index);
    uint16_t         elapsed_permillage =
        (model->run.minion.read.elapsed_milliseconds * 1000) / program_get_duration_milliseconds(program);

    lv_obj_set_x(pdata->time_bar, (TIME_UNIT_WIDTH * PROGRAM_NUM_TIME_UNITS * elapsed_permillage) / 1000);
}


static void close_page(void *state) {
    (void)state;
    lv_obj_clean(lv_scr_act());
}


static uint16_t get_channel_activity_duration(uint8_t *channel_configuration, uint16_t starting_index) {
    uint16_t duration = 1;

    for (uint16_t i = starting_index + 1; i < PROGRAM_NUM_TIME_UNITS; i++) {
        if (channel_configuration[i] == channel_configuration[starting_index]) {
            duration++;
        } else {
            break;
        }
    }

    return duration;
}


static lv_obj_t *channel_schedule_create(lv_obj_t *parent, uint8_t *channel_configuration, uint8_t digital) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_style_pad_all(cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, PROGRAM_NUM_TIME_UNITS * TIME_UNIT_WIDTH, CHANNEL_ROW_HEIGHT);
    lv_obj_add_style(cont, &style_transparent_cont, LV_STATE_DEFAULT);

    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *obj_line = lv_obj_create(cont);
    lv_obj_add_flag(obj_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(obj_line, LV_PCT(100), 6);
    lv_obj_set_style_bg_color(obj_line, lv_obj_get_style_border_color(obj_line, LV_PART_MAIN), LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj_line, 0, LV_STATE_DEFAULT);
    lv_obj_center(obj_line);

    uint16_t i = 0;
    while (i < PROGRAM_NUM_TIME_UNITS) {
        lv_obj_t *unit = lv_obj_create(cont);
        lv_obj_remove_flag(unit, LV_OBJ_FLAG_SCROLLABLE);
        uint16_t duration = get_channel_activity_duration(channel_configuration, i);
        assert(duration > 0);
        lv_obj_set_size(unit, TIME_UNIT_WIDTH * duration, CHANNEL_ROW_HEIGHT);

        if (channel_configuration[i] > 0) {
            const lv_color_t colors[3] = {STYLE_COLOR_GREEN, STYLE_COLOR_YELLOW, STYLE_COLOR_RED};
            lv_color_t       color     = digital ? STYLE_COLOR_BLUE : colors[channel_configuration[i] - 1];

            lv_obj_set_style_bg_color(unit, color, LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(unit, 4, LV_STATE_DEFAULT);
            lv_obj_set_style_radius(unit, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(unit, lv_color_darken(color, LV_OPA_10), LV_STATE_DEFAULT);
        } else {
            lv_obj_add_style(unit, &style_transparent_cont, LV_STATE_DEFAULT);
        }
        i += duration;
    }

    return cont;
}


static void configuration_from_digital_channel(uint8_t                           *channel_configuration,
                                               program_digital_channel_schedule_t digital_channel_schedule) {
    for (uint16_t i = 0; i < PROGRAM_NUM_TIME_UNITS; i++) {
        channel_configuration[i] = (digital_channel_schedule & (1 << i)) > 0;
    }
}


static void configuration_from_dac_channel(uint8_t                     *channel_configuration,
                                           program_dac_channel_state_t *dac_channel_states) {
    for (uint16_t i = 0; i < PROGRAM_NUM_TIME_UNITS; i++) {
        channel_configuration[i] = dac_channel_states[i];
    }
}


static void configuration_from_sensor_channel(uint8_t                            *channel_configuration,
                                              program_sensor_channel_threshold_t *dac_channel_thresholds) {
    for (uint16_t i = 0; i < PROGRAM_NUM_TIME_UNITS; i++) {
        channel_configuration[i] = dac_channel_thresholds[i];
    }
}


const pman_page_t page_execution = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
