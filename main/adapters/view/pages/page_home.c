#include <string.h>
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
#include "config/app_config.h"


enum {
    EXECUTE_BTN_ID,
    CONFIG_BTN_ID,
    TEST_BTN_ID,
    INFO_BTN_ID,
    OTA_BTN_ID,
    CONFIRM_BTN_ID,
    REJECT_BTN_ID,
    POPUP_PASSWORD_ID,
};

struct page_data {
    lv_obj_t *button_ota;
    lv_obj_t *button_popup_no;

    lv_obj_t *label_popup;

    lv_obj_t *obj_blanket;

    struct view_common_password_popup password_popup;

    enum { POPUP_STATE_NONE, POPUP_STATE_OTA } popup_state;
    enum {
        PASSWORD_NAVIGATION_NONE = 0,
        PASSWORD_NAVIGATION_DIAGNOSIS,
        PASSWORD_NAVIGATION_CONFIG,
    } password_navigation;
};

static void update_page(model_t *model, struct page_data *pdata);

static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->popup_state         = POPUP_STATE_NONE;
    pdata->password_navigation = PASSWORD_NAVIGATION_NONE;

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
        lv_obj_t *btn1 = lv_button_create(obj_background);
        lv_obj_set_size(btn1, 300, 150);
        lv_obj_t *image = lv_image_create(btn1);
        lv_image_set_src(image, &img_execute);
        lv_obj_center(image);
        lv_obj_align(btn1, LV_ALIGN_TOP_LEFT, offsetx, offsety);
        view_register_object_default_callback(btn1, EXECUTE_BTN_ID);
    }

    {
        LV_IMAGE_DECLARE(img_programs);
        lv_obj_t *btn2 = lv_button_create(obj_background);
        lv_obj_set_size(btn2, 300, 150);
        lv_obj_t *image = lv_image_create(btn2);
        lv_image_set_src(image, &img_programs);
        lv_obj_center(image);
        lv_obj_align(btn2, LV_ALIGN_TOP_RIGHT, -offsetx, offsety);
        view_register_object_default_callback(btn2, CONFIG_BTN_ID);
    }

    {
        LV_IMAGE_DECLARE(img_settings);
        lv_obj_t *btn3 = lv_button_create(obj_background);
        lv_obj_set_size(btn3, 300, 150);
        lv_obj_t *image = lv_image_create(btn3);
        lv_image_set_src(image, &img_settings);
        lv_obj_center(image);
        lv_obj_align(btn3, LV_ALIGN_BOTTOM_LEFT, offsetx, -offsety);
        view_register_object_default_callback(btn3, TEST_BTN_ID);
    }

    {
        LV_IMAGE_DECLARE(img_info);
        lv_obj_t *btn4 = lv_button_create(obj_background);
        lv_obj_set_size(btn4, 300, 150);
        lv_obj_t *image = lv_image_create(btn4);
        lv_image_set_src(image, &img_info);
        lv_obj_center(image);
        lv_obj_align(btn4, LV_ALIGN_BOTTOM_RIGHT, -offsetx, -offsety);
        view_register_object_default_callback(btn4, INFO_BTN_ID);
    }

    {
        lv_obj_t *btn = lv_button_create(obj_background);
        lv_obj_set_size(btn, 64, 64);
        lv_obj_t *label = lv_label_create(btn);
        lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
        lv_label_set_text(label, LV_SYMBOL_DRIVE);
        lv_obj_center(label);
        lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -offsetx, 0);
        view_register_object_default_callback(btn, OTA_BTN_ID);
        pdata->button_ota = btn;
    }

    LV_IMAGE_DECLARE(img_rotondi);
    lv_obj_t *image_logo = lv_image_create(lv_screen_active());
    lv_image_set_src(image_logo, &img_rotondi);
    lv_obj_center(image_logo);

    pdata->password_popup = view_common_password_popup(lv_screen_active(), POPUP_PASSWORD_ID);

    {
        lv_obj_t *obj = lv_obj_create(lv_screen_active());
        lv_obj_add_style(obj, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));

        lv_obj_t *cont = lv_obj_create(obj);
        lv_obj_set_size(cont, LV_PCT(70), LV_PCT(70));
        lv_obj_center(cont);

        lv_obj_t *lbl_msg = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl_msg, STYLE_FONT_BIG, LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(lbl_msg, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl_msg, LV_PCT(95));
        lv_obj_align(lbl_msg, LV_ALIGN_CENTER, 0, -48);
        pdata->label_popup = lbl_msg;

        lv_obj_t *btn_yes = lv_button_create(cont);
        lv_obj_set_size(btn_yes, 128, 56);
        lv_obj_t *lbl_yes = lv_label_create(btn_yes);
        lv_obj_set_style_text_font(lbl_yes, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_label_set_text(lbl_yes, "Si");
        lv_obj_center(lbl_yes);
        lv_obj_align(btn_yes, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        view_register_object_default_callback(btn_yes, CONFIRM_BTN_ID);

        lv_obj_t *btn_no = lv_button_create(cont);
        lv_obj_set_size(btn_no, 128, 56);
        lv_obj_t *lbl_no = lv_label_create(btn_no);
        lv_obj_set_style_text_font(lbl_no, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_label_set_text(lbl_no, "No");
        lv_obj_center(lbl_no);
        lv_obj_align(btn_no, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        view_register_object_default_callback(btn_no, REJECT_BTN_ID);
        pdata->button_popup_no = btn_no;

        pdata->obj_blanket = obj;
    }

    VIEW_ADD_WATCHED_VARIABLE(&model->run.firmware_update_ready, 0);

    update_page(model, pdata);
}

static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;
    mut_model_t      *model = view_get_model(handle);
    (void)pdata;
    (void)model;

    switch (event.tag) {
        case PMAN_EVENT_TAG_OPEN: {
            break;
        }

        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_PAGE_WATCHER:
                    update_page(model, pdata);
                    break;

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
                            pdata->password_navigation = PASSWORD_NAVIGATION_CONFIG;
                            lv_textarea_set_text(pdata->password_popup.textarea, "");
                            update_page(model, pdata);
                            break;

                        case TEST_BTN_ID:
                            pdata->password_navigation = PASSWORD_NAVIGATION_DIAGNOSIS;
                            lv_textarea_set_text(pdata->password_popup.textarea, "");
                            update_page(model, pdata);
                            break;

                        case INFO_BTN_ID:
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE(&page_info);
                            break;

                        case OTA_BTN_ID: {
                            pdata->popup_state = POPUP_STATE_OTA;
                            update_page(model, pdata);
                            break;
                        }

                        case CONFIRM_BTN_ID: {
                            if (model->args.flag_firmware_update) {
                                model->args.flag_firmware_update = 0;
                                view_get_protocol(handle)->finalize_ota_update(handle);
                            } else {
                                pdata->popup_state = POPUP_STATE_NONE;
                                view_get_protocol(handle)->ota_update(handle);
                            }
                            update_page(model, pdata);
                            break;
                        }

                        case REJECT_BTN_ID: {
                            pdata->popup_state = POPUP_STATE_NONE;
                            update_page(model, pdata);
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_READY: {
                    switch (view_get_obj_id(target)) {
                        case POPUP_PASSWORD_ID: {
                            if (pdata->password_navigation == PASSWORD_NAVIGATION_DIAGNOSIS &&
                                strcmp(lv_textarea_get_text(pdata->password_popup.textarea), "1790") == 0) {
                                pdata->password_navigation = PASSWORD_NAVIGATION_NONE;
                                msg.stack_msg              = PMAN_STACK_MSG_PUSH_PAGE(&page_settings);
                            } else if (pdata->password_navigation == PASSWORD_NAVIGATION_CONFIG &&
                                       strcmp(lv_textarea_get_text(pdata->password_popup.textarea), "1958") == 0) {
                                pdata->password_navigation = PASSWORD_NAVIGATION_NONE;
                                msg.stack_msg              = PMAN_STACK_MSG_PUSH_PAGE(&page_config);
                            } else {
                                view_show_toast(1, "Password errata");
                            }
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_CANCEL: {
                    switch (view_get_obj_id(target)) {
                        case POPUP_PASSWORD_ID: {
                            pdata->password_navigation = PASSWORD_NAVIGATION_NONE;
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
    view_common_set_hidden(pdata->button_ota, !model->run.firmware_update_ready);
    if (model->args.flag_firmware_update) {
        view_common_set_hidden(pdata->obj_blanket, 0);
        lv_label_set_text_fmt(pdata->label_popup, "Rilevato aggiornamento alla versione %s. Confermare?",
                              SOFTWARE_VERSION);
        view_common_set_hidden(pdata->button_popup_no, 1);
    } else if (pdata->popup_state == POPUP_STATE_OTA) {
        view_common_set_hidden(pdata->obj_blanket, 0);
        lv_label_set_text(pdata->label_popup, "Procedere con l'aggiornamento dell'applicazione?");
        view_common_set_hidden(pdata->button_popup_no, 0);
    } else {
        view_common_set_hidden(pdata->obj_blanket, 1);
    }

    if (pdata->password_navigation == PASSWORD_NAVIGATION_NONE) {
        view_common_set_hidden(pdata->password_popup.blanket, 1);
    } else {
        view_common_set_hidden(pdata->password_popup.blanket, 0);
    }
}


static void close_page(void *state) {
    (void)state;
    lv_obj_clean(lv_screen_active());
}


const pman_page_t page_home = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
