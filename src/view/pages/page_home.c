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
    EXECUTE_BTN_ID,
    PROGRAMS_BTN_ID,
    SETTINGS_BTN_ID,
    INFO_BTN_ID,
};

struct page_data {
    char *message;
    lv_obj_t * btn_e;
    lv_obj_t * btn_p;
    lv_obj_t * btn_s;
    lv_obj_t * btn_i;

    lv_obj_t *date_time_label;
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
    int32_t offsety = 100;

    // create 4 buttons in a 2x2 grid
    lv_obj_t *btn1 = lv_btn_create(lv_scr_act());
    lv_obj_t *lbl1 = lv_label_create(btn1);
    lv_label_set_text(lbl1, "Exe");
    lv_obj_center(lbl1);
    lv_obj_align(btn1, LV_ALIGN_TOP_LEFT, offsetx, offsety);
    view_register_object_default_callback(btn1, EXECUTE_BTN_ID);
    pdata->btn_e = btn1;

    lv_obj_t *btn2 = lv_btn_create(lv_scr_act());
    lv_obj_t *lbl2 = lv_label_create(btn2);
    lv_label_set_text(lbl2, "Program");
    lv_obj_center(lbl2);
    lv_obj_align(btn2, LV_ALIGN_TOP_RIGHT, -offsetx, offsety);
    view_register_object_default_callback(btn2, PROGRAMS_BTN_ID);
    pdata->btn_p = btn2;

    lv_obj_t *btn3 = lv_btn_create(lv_scr_act());
    lv_obj_t *lbl3 = lv_label_create(btn3);
    lv_label_set_text(lbl3, "Settings");
    lv_obj_center(lbl3);
    lv_obj_align(btn3, LV_ALIGN_BOTTOM_LEFT, offsetx, -offsety);
    view_register_object_default_callback(btn3, SETTINGS_BTN_ID);
    pdata->btn_s = btn3;

    lv_obj_t *btn4 = lv_btn_create(lv_scr_act());
    lv_obj_t *lbl4 = lv_label_create(btn4);
    lv_label_set_text(lbl4, "Info");
    lv_obj_center(lbl4);
    lv_obj_align(btn4, LV_ALIGN_BOTTOM_RIGHT, -offsetx, -offsety);
    view_register_object_default_callback(btn4, INFO_BTN_ID);
    pdata->btn_i = btn4;

    lv_obj_t* datetime_widget = view_common_create_datetime_widget(lv_scr_act(), 0, 0);
    lv_obj_t* logo_widget = view_common_create_logo_widget(lv_scr_act());

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

                        case EXECUTE_BTN_ID:
                            msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_PUSH_PAGE;
                            msg.stack_msg.as.destination.page = (void *)&page_execution_home;
                        break;

                        case PROGRAMS_BTN_ID:
                            msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_PUSH_PAGE;
                            msg.stack_msg.as.destination.page = (void *)&page_programs_home;
                        break;

                        case SETTINGS_BTN_ID:
                            msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_PUSH_PAGE;
                            msg.stack_msg.as.destination.page = (void *)&page_settings_home;
                        break;

                        case INFO_BTN_ID:
                            msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_PUSH_PAGE;
                            msg.stack_msg.as.destination.page = (void *)&page_info;
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

}

static void close_page(void *state) {
    (void)state;
    lv_obj_clean(lv_scr_act());
}

const pman_page_t page_home = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
