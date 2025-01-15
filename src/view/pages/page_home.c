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
    CONFIG_BTN_ID,
    TEST_BTN_ID,
    INFO_BTN_ID,
};

struct page_data {
    char *message;
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

    int32_t offsetx = 50;
    int32_t offsety = 30;

    lv_obj_t *obj_background = view_common_gradient_background_create(lv_screen_active());

    // create 4 buttons in a 2x2 grid
    {
        LV_IMAGE_DECLARE(img_execute);
        lv_obj_t *btn1 = lv_btn_create(obj_background);
        lv_obj_set_size(btn1, 300, 150);
        lv_obj_t *image = lv_image_create(btn1);
        lv_image_set_src(image, &img_execute);
        lv_obj_center(image);
        lv_obj_align(btn1, LV_ALIGN_TOP_LEFT, offsetx, offsety);
        view_register_object_default_callback(btn1, EXECUTE_BTN_ID);
    }

    {
        LV_IMAGE_DECLARE(img_programs);
        lv_obj_t *btn2 = lv_btn_create(obj_background);
        lv_obj_set_size(btn2, 300, 150);
        lv_obj_t *image = lv_image_create(btn2);
        lv_image_set_src(image, &img_programs);
        lv_obj_center(image);
        lv_obj_align(btn2, LV_ALIGN_TOP_RIGHT, -offsetx, offsety);
        view_register_object_default_callback(btn2, CONFIG_BTN_ID);
    }

    {
        LV_IMAGE_DECLARE(img_settings);
        lv_obj_t *btn3 = lv_btn_create(obj_background);
        lv_obj_set_size(btn3, 300, 150);
        lv_obj_t *image = lv_image_create(btn3);
        lv_image_set_src(image, &img_settings);
        lv_obj_center(image);
        lv_obj_align(btn3, LV_ALIGN_BOTTOM_LEFT, offsetx, -offsety);
        view_register_object_default_callback(btn3, TEST_BTN_ID);
    }

    {
        LV_IMAGE_DECLARE(img_info);
        lv_obj_t *btn4 = lv_btn_create(obj_background);
        lv_obj_set_size(btn4, 300, 150);
        lv_obj_t *image = lv_image_create(btn4);
        lv_image_set_src(image, &img_info);
        lv_obj_center(image);
        lv_obj_align(btn4, LV_ALIGN_BOTTOM_RIGHT, -offsetx, -offsety);
        view_register_object_default_callback(btn4, INFO_BTN_ID);
    }

    LV_IMAGE_DECLARE(img_rotondi);
    lv_obj_t *image_logo = lv_image_create(lv_screen_active());
    lv_image_set_src(image_logo, &img_rotondi);
    lv_obj_center(image_logo);

    update_page(model, pdata);
}

static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;
    mut_model_t      *model = view_get_model(handle);
    (void)pdata;
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

                        case EXECUTE_BTN_ID:
                            msg.stack_msg.tag                 = PMAN_STACK_MSG_TAG_PUSH_PAGE;
                            msg.stack_msg.as.destination.page = (void *)&page_choice;
                            break;

                        case CONFIG_BTN_ID:
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE(&page_config);
                            break;

                        case TEST_BTN_ID:
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE(&page_test);
                            break;

                        case INFO_BTN_ID:
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE(&page_info);
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

static void update_page(model_t *model, struct page_data *pdata) {}

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
