#include "../view.h"
#include "lvgl.h"
#include "model/model.h"
#include "src/core/lv_obj_event.h"
#include "src/misc/lv_types.h"
#include "src/page.h"
#include <assert.h>
#include <stdlib.h>

#define NUM_CHANNELS 16
#define NUM_TIME_UNITS 25
#define MAX_LABEL_WIDTH 150
#define CHANNEL_ROW_OFFSET 50

static const lv_style_const_prop_t style_label_props[] = {
    LV_STYLE_CONST_RADIUS(5),
    LV_STYLE_CONST_BORDER_WIDTH(2),
    LV_STYLE_CONST_BG_COLOR(LV_PALETTE_BLUE),
};
LV_STYLE_CONST_INIT(style_label, style_label_props);


enum {
    FILESYSTEM_BTN_ID,
    PLAY_BTN_ID,
    PROGRAM_NAME_LABEL_ID,
    DATETIME_LABEL_ID,
    PROGRAM_BTNMATRIX_ID,
};

struct page_data {
    char *message;
    lv_obj_t *file_system_icon;
    lv_obj_t *play_icon;
    lv_obj_t *program_name_label;
    lv_obj_t *date_time_label;

    char *channel_names[NUM_CHANNELS];
    bool channels_states[NUM_CHANNELS][NUM_TIME_UNITS];
};

static void update_page(model_t *model, struct page_data *pdata);

static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->message = extra;

    return pdata;
}

static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    int32_t offsetx = 100;

    lv_obj_t *btn1 = lv_btn_create(lv_scr_act());
    lv_obj_t *lbl1 = lv_label_create(btn1);
    lv_label_set_text(lbl1, "FILES");
    lv_obj_align(btn1, LV_ALIGN_TOP_LEFT, 0, 0);
    view_register_object_default_callback(btn1, FILESYSTEM_BTN_ID);

    lv_obj_t *btn2 = lv_btn_create(lv_scr_act());
    lv_obj_t *lbl2 = lv_label_create(btn2);
    lv_label_set_text(lbl2, "PLAY");
    lv_obj_align(btn2, LV_ALIGN_TOP_LEFT, offsetx, 0);
    view_register_object_default_callback(btn1, PLAY_BTN_ID);

    lv_obj_t *program_name_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_align(program_name_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(program_name_label, LV_ALIGN_TOP_LEFT, offsetx * 2, 0);
    lv_label_set_text(program_name_label, "Program Name");
    pdata->program_name_label = program_name_label;

    lv_obj_t *date_time_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_align(date_time_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(date_time_label, LV_ALIGN_TOP_LEFT, offsetx * 3.5, 0);
    pdata->date_time_label = date_time_label;

    // programs settings
    lv_obj_t *channel_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_align(channel_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(channel_label, LV_ALIGN_TOP_LEFT, 0, CHANNEL_ROW_OFFSET);
    lv_obj_add_style(channel_label, (lv_style_t *)&style_label, LV_STATE_DEFAULT);
    lv_obj_set_width(channel_label, MAX_LABEL_WIDTH);
    lv_label_set_text(channel_label, "Tooltip CH 1");

    // array that shows the 25 buttons showing if the channel is on or off in that time unit
    

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
            lv_obj_t           *target   = lv_event_get_current_target_obj(event.as.lvgl);
            view_object_data_t *obj_data = lv_obj_get_user_data(target);

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    switch (obj_data->id) {
                        case 0: {
                            if (pdata->message == NULL) {
                                msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_main, "Secondo messaggio");
                            } else {
                                msg.stack_msg = PMAN_STACK_MSG_BACK();
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

            break;
        }

        default:
            break;
    }

    return msg;
}

static void update_page(model_t *model, struct page_data *pdata) {}

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
