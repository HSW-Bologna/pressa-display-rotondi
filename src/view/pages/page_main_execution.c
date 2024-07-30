#include "../view.h"
#include "lvgl.h"
#include "model/model.h"
#include "src/core/lv_obj_event.h"
#include "src/misc/lv_types.h"
#include "src/page.h"
#include <assert.h>
#include <stdlib.h>


enum {
    BTN_ID,
    PROGRAM_BTN_ID,
    PROGRAM_BTNMATRIX_ID,
};

struct page_data {
    int num_prog;
    lv_obj_t *btnmx;
};

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

    static const char * map[] = {"1", "6", "\n",
    "2", "7", "\n",
    "3", "8", "\n",
    "4", "9", "\n",
    "5", "10", "",};

    lv_obj_t *btnmatrix = lv_btnmatrix_create(lv_scr_act());
    lv_btnmatrix_set_map(btnmatrix, map);
    lv_obj_set_size(btnmatrix, 600, 400);
    lv_obj_align(btnmatrix, LV_ALIGN_CENTER, -16, 0);
    view_register_object_default_callback(btnmatrix, PROGRAM_BTNMATRIX_ID);

    pdata->btnmx = btnmatrix;

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
                        case PROGRAM_BTNMATRIX_ID: 
                            int num = lv_btnmatrix_get_selected_btn(target);
                            pdata->num_prog = num;
                            printf("num prog %d\n", pdata->num_prog);
                            size_t *extra                      = malloc(sizeof(size_t) * 1);
                            extra[0]                           = pdata->num_prog;
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_program, extra);
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

const pman_page_t page_main_execution = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
