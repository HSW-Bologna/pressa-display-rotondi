#include "view.h"
#include "lvgl.h"
#include "model/model.h"
#include "page_manager.h"
#include "src/page.h"
#include "watcher.h"
#include "services/timestamp.h"


static void view_watcher_cb(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg);


static struct {
    pman_t          page_manager;
    view_protocol_t view_protocol;
    watcher_t       watcher;
} state = {0};


void view_init(model_t *model, view_protocol_t protocol) {
    state.view_protocol = protocol;
    WATCHER_INIT_STD(&state.watcher, NULL);
    lv_init();

#ifdef BUILD_CONFIG_SIMULATOR
    lv_sdl_window_create(BUILD_CONFIG_DISPLAY_HORIZONTAL_RESOLUTION, BUILD_CONFIG_DISPLAY_VERTICAL_RESOLUTION);
    lv_indev_t *touch_indev = lv_sdl_mouse_create();
#else
    lv_display_t *display = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(display, "/dev/fb0");

    lv_indev_t *touch_indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event0");
#endif

    pman_init(&state.page_manager, (void *)model, touch_indev, NULL, NULL);
}

void view_change_page(const pman_page_t *page) {
    pman_change_page(&state.page_manager, *page);
}

mut_model_t *view_get_model(void *handle) {
    return pman_get_user_data(handle);
}

void view_register_object_default_callback(lv_obj_t *obj, int id) {
    view_register_object_default_callback_with_number(obj, id, 0);
}


view_protocol_t *view_get_protocol(pman_handle_t handle) {
    (void)handle;
    return &state.view_protocol;
}


void view_event(view_event_t event) {
    pman_event(&state.page_manager, (pman_event_t){.tag = PMAN_EVENT_TAG_USER, .as = {.user = &event}});
}


void view_register_object_default_callback_with_number(lv_obj_t *obj, int id, int number) {
    lv_obj_set_user_data(obj, (void *)(uintptr_t)((id & 0xFFFF) | ((number & 0xFFFF) << 16)));

    pman_unregister_obj_event(obj);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_CLICKED);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_VALUE_CHANGED);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_RELEASED);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_PRESSED);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_PRESSING);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_LONG_PRESSED);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_LONG_PRESSED_REPEAT);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_CANCEL);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_READY);
}


void view_manage(model_t *model) {
    (void)model;
    // view_common_set_hidden(state.popup_communication_error.blanket, !model->run.minion.communication_error);
    // lv_label_set_text(state.popup_communication_error.lbl_msg, view_intl_get_string(model,
    // STRINGS_ERRORE_DI_COMUNICAZIONE)); lv_label_set_text(state.popup_communication_error.lbl_retry,
    // view_intl_get_string(model, STRINGS_RIPROVA));

    watcher_watch(&state.watcher, timestamp_get());
}


void view_add_watched_variable(void *ptr, size_t size, int code) {
    watcher_add_entry(&state.watcher, ptr, size, view_watcher_cb, (void *)(uintptr_t)code);
}


uint16_t view_get_obj_id(lv_obj_t *obj) {
    return (((uint32_t)(uintptr_t)lv_obj_get_user_data(obj)) & 0xFFFF);
}


uint16_t view_get_obj_number(lv_obj_t *obj) {
    return ((((uint32_t)(uintptr_t)lv_obj_get_user_data(obj)) >> 16) & 0xFFFF);
}


static void view_watcher_cb(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg) {
    (void)old_value;
    (void)memory;
    (void)size;
    (void)user_ptr;
    view_event((view_event_t){.tag = VIEW_EVENT_TAG_PAGE_WATCHER, .as.page_watcher.code = (uintptr_t)arg});
}
