#include "view.h"
#include "lvgl.h"
#include "model/model.h"
#include "page_manager.h"
#include "src/page.h"

static pman_t          page_manager  = {0};
static view_protocol_t view_protocol = {0};

void view_init(model_t *model, view_protocol_t protocol) {
    view_protocol = protocol;
    lv_init();

#ifdef BUILD_CONFIG_SIMULATOR
    lv_sdl_window_create(BUILD_CONFIG_DISPLAY_HORIZONTAL_RESOLUTION, BUILD_CONFIG_DISPLAY_VERTICAL_RESOLUTION);
    lv_indev_t *touch_indev = lv_sdl_mouse_create();
#endif

    pman_init(&page_manager, (void *)model, touch_indev, NULL, NULL);
}

void view_change_page(const pman_page_t *page) {
    pman_change_page(&page_manager, *page);
}

mut_model_t *view_get_model(void *handle) {
    return pman_get_user_data(handle);
}

void view_register_object_default_callback(lv_obj_t *obj, int id) {
    view_register_object_default_callback_with_number(obj, id, 0);
}


view_protocol_t *view_get_protocol(pman_handle_t handle) {
    (void)handle;
    return &view_protocol;
}


void view_event(view_event_t event) {
    pman_event(&page_manager, (pman_event_t){.tag = PMAN_EVENT_TAG_USER, .as = {.user = &event}});
}


void view_register_object_default_callback_with_number(lv_obj_t *obj, int id, int number) {
    view_object_data_t *data = malloc(sizeof(view_object_data_t));
    data->id                 = id;
    data->number             = number;
    lv_obj_set_user_data(obj, data);

    pman_unregister_obj_event(obj);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_CLICKED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_VALUE_CHANGED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_RELEASED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_PRESSED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_PRESSING);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_LONG_PRESSED);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_LONG_PRESSED_REPEAT);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_CANCEL);
    pman_register_obj_event(&page_manager, obj, LV_EVENT_READY);
    pman_set_obj_self_destruct(obj);
}
