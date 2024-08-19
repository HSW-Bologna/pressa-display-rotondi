#include "../view.h"
#include "lvgl.h"
#include "model/model.h"
#include "src/core/lv_obj_event.h"
#include "src/misc/lv_types.h"
#include "src/page.h"
#include <assert.h>
#include <stdlib.h>
#include "../common.h"

#define NUM_CHANNELS       16
#define NUM_TIME_UNITS     25
#define MAX_LABEL_WIDTH    120
#define CHANNEL_ROW_OFFSET 55
#define BTN_SIZE           20

enum {
    FILESYSTEM_BTN_ID,
    PLAY_BTN_ID,
    PROGRAM_NAME_LABEL_ID,
    DATETIME_LABEL_ID,
    PROGRAM_BTNMATRIX_ID,
};

struct page_data {
    char     *message;
    lv_obj_t *file_system_icon;
    lv_obj_t *play_icon;
    lv_obj_t *program_name_label;
    lv_obj_t *date_time_label;

    char *channel_names[NUM_CHANNELS];
    bool  channels_states[NUM_CHANNELS][NUM_TIME_UNITS];
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

    int32_t offset_play_btn = 50 + 10;

    lv_obj_t *folder_widget = view_common_create_folder_widget(lv_scr_act(), LV_ALIGN_TOP_LEFT, 5, 0);

    lv_obj_t *play_button = view_common_create_play_button(lv_scr_act(), LV_ALIGN_TOP_LEFT, offset_play_btn, 0);

    lv_obj_t *program_name_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_align(program_name_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(program_name_label, LV_ALIGN_TOP_LEFT, 2 * offset_play_btn + 10, 0);
    lv_label_set_text(program_name_label, "Program Name");
    pdata->program_name_label = program_name_label;

    lv_obj_t *datetime_widget = view_common_create_datetime_widget(lv_scr_act(), 0, 0);
    lv_obj_t *logo_widget     = view_common_create_logo_widget(lv_scr_act());


    // channel name label
    lv_obj_t *channel_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_align(channel_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(channel_label, LV_ALIGN_TOP_LEFT, 0, CHANNEL_ROW_OFFSET);
    lv_obj_set_width(channel_label, MAX_LABEL_WIDTH);
    lv_label_set_text(channel_label, "Tooltip CH 1");

    // array that shows the 25 buttons showing if the channel is on or off in that time unit
    lv_obj_t *btn_matrix = lv_btnmatrix_create(lv_scr_act());


    static lv_style_t style_bg;
    lv_style_init(&style_bg);
    lv_style_set_pad_all(&style_bg, 0);
    lv_style_set_pad_gap(&style_bg, 0);
    lv_style_set_clip_corner(&style_bg, true);
    lv_style_set_radius(&style_bg, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&style_bg, 0);

    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 0);
    lv_style_set_border_width(&style_btn, 1);
    lv_style_set_border_opa(&style_btn, LV_OPA_50);
    lv_style_set_border_color(&style_btn, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_border_side(&style_btn, LV_BORDER_SIDE_INTERNAL);

    lv_obj_add_style(btn_matrix, &style_bg, LV_PART_MAIN);
    lv_obj_add_style(btn_matrix, &style_btn, LV_PART_ITEMS);

    lv_obj_set_size(btn_matrix, BTN_SIZE * NUM_TIME_UNITS, BTN_SIZE);
    lv_obj_align(btn_matrix, LV_ALIGN_TOP_LEFT, MAX_LABEL_WIDTH + 5, CHANNEL_ROW_OFFSET);
    static const char *map[] = {"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
                                "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", ""};

    lv_btnmatrix_set_map(btn_matrix, map);

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

const pman_page_t page_exec_program = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
