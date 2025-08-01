#include "view.h"
#include "lvgl.h"
#include "model/model.h"
#include "page_manager.h"
#include "src/page.h"
#include "watcher.h"
#include "services/timestamp.h"
#include "common.h"
#include "style.h"


static void view_watcher_cb(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg);
static void retry_communication_callback(lv_event_t *event);
static void ignore_communication_error_callback(lv_event_t *event);
static void toast_fade_out_timer_cb(lv_timer_t *timer);
static void align_toast_list(void);
static void toast_fade_out(lv_obj_t *obj_toast);
static void toast_click_callback(lv_event_t *event);


static struct {
    pman_t                      page_manager;
    view_protocol_t             view_protocol;
    watcher_t                   watcher;
    communication_error_popup_t popup_communication_error;
    uint16_t                    communication_attempts;
    timestamp_t                 last_communication_attempt;
    lv_obj_t                   *toast_list;
} state = {0};


void view_init(model_t *model, view_protocol_t protocol) {
    state.view_protocol = protocol;
    WATCHER_INIT_STD(&state.watcher, NULL);

    lv_display_t *display = lv_display_get_default();
    lv_indev_t * touch_indev = lv_indev_get_next(NULL);

    lv_display_set_theme(display, lv_theme_default_init(display, STYLE_COLOR_PRIMARY, lv_palette_main(LV_PALETTE_RED),
                                                        LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT));

    state.communication_attempts     = 0;
    state.last_communication_attempt = timestamp_get();
    state.popup_communication_error  = view_common_communication_error_popup(lv_layer_top());

    lv_obj_add_event_cb(state.popup_communication_error.btn_retry, retry_communication_callback, LV_EVENT_CLICKED,
                        &state.page_manager);
    lv_obj_add_event_cb(state.popup_communication_error.btn_disable, ignore_communication_error_callback,
                        LV_EVENT_CLICKED, &state.page_manager);
    view_common_set_hidden(state.popup_communication_error.blanket, 1);

    pman_init(&state.page_manager, (void *)model, touch_indev, NULL, NULL, NULL);
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
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_SCROLL);
    pman_register_obj_event(&state.page_manager, obj, LV_EVENT_SCROLL_BEGIN);
}


void view_manage(model_t *model) {
    // Reset the attempt counter every minute
    if (state.communication_attempts > 0 && timestamp_is_expired(state.last_communication_attempt, 60000UL)) {
        state.communication_attempts = 0;
    }

    if (model->run.minion.communication_error && model->run.minion.communication_enabled) {
        // After 5 attempts allow the popup to disappear without retrying
        if (state.communication_attempts >= 5) {
            view_common_set_hidden(state.popup_communication_error.btn_disable, 0);
            lv_obj_align(state.popup_communication_error.btn_disable, LV_ALIGN_BOTTOM_MID, -128, 0);
            lv_obj_align(state.popup_communication_error.btn_retry, LV_ALIGN_BOTTOM_MID, 128, 0);
        } else {
            view_common_set_hidden(state.popup_communication_error.btn_disable, 1);
            lv_obj_align(state.popup_communication_error.btn_retry, LV_ALIGN_BOTTOM_MID, 0, 0);
        }

        lv_label_set_text(state.popup_communication_error.lbl_msg, "Errore di comunicazione!");
        lv_label_set_text(state.popup_communication_error.lbl_retry, "Riprova");
        lv_label_set_text(state.popup_communication_error.lbl_disable, "Disabilita");

        view_common_set_hidden(state.popup_communication_error.blanket, 0);
    } else {
        view_common_set_hidden(state.popup_communication_error.blanket, 1);
    }

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


void view_show_toast(uint8_t error, const char *fmt, ...) {
    lv_color_t color = error ? STYLE_COLOR_RED : STYLE_COLOR_PRIMARY;

    lv_obj_t *obj_toast = lv_obj_create(lv_layer_top());
    lv_obj_set_size(obj_toast, LV_PCT(80), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(obj_toast, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj_toast, lv_color_lighten(color, LV_OPA_70), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj_toast, color, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj_toast, 8, LV_STATE_DEFAULT);
    lv_obj_align(obj_toast, LV_ALIGN_BOTTOM_MID, 0, -32);

    lv_obj_t *label = lv_label_create(obj_toast);
    lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    char toast_string[256] = {0};

    /* Declare a va_list type variable */
    va_list args;
    /* Initialise the va_list variable with the ... after fmt */
    va_start(args, fmt);
    vsnprintf(toast_string, sizeof(toast_string), fmt, args);
    va_end(args);

    lv_label_set_text(label, toast_string);
    lv_obj_center(label);

    lv_timer_t *timer = lv_timer_create(toast_fade_out_timer_cb, 5000, obj_toast);
    lv_timer_set_auto_delete(timer, 1);
    lv_timer_set_repeat_count(timer, 1);
    lv_timer_resume(timer);

    lv_obj_add_event_cb(obj_toast, toast_click_callback, LV_EVENT_CLICKED, timer);

    lv_obj_set_user_data(obj_toast, state.toast_list);
    state.toast_list = obj_toast;

    lv_obj_move_foreground(state.popup_communication_error.blanket);

    align_toast_list();
}


static void align_toast_list(void) {
    if (state.toast_list != NULL) {
        lv_obj_align(state.toast_list, LV_ALIGN_BOTTOM_MID, 0, -8);
        lv_obj_t *prev_toast = state.toast_list;

        while (prev_toast) {
            lv_obj_t *next_toast = lv_obj_get_user_data(prev_toast);
            if (next_toast != NULL) {
                lv_obj_align_to(next_toast, prev_toast, LV_ALIGN_OUT_TOP_MID, 0, -8);
            }
            prev_toast = next_toast;
        }
    }
}


static void fade_anim_cb(void *obj, int32_t v) {
    lv_obj_set_style_opa(obj, v, 0);
}


static void delete_var_anim_cb(lv_anim_t *a) {
    if (a->var == state.toast_list) {
        state.toast_list = lv_obj_get_user_data(a->var);
    } else {
        lv_obj_t *prev_toast = state.toast_list;
        lv_obj_t *next_toast = lv_obj_get_user_data(state.toast_list);

        // The object must be removed from the list before being deleted
        while (next_toast != NULL) {
            // Object found
            if (next_toast == a->var) {
                // Remove object from list
                lv_obj_set_user_data(prev_toast, lv_obj_get_user_data(next_toast));
                break;
            } else {
                prev_toast = next_toast;
                next_toast = lv_obj_get_user_data(next_toast);
            }
        }
    }
    lv_obj_delete(a->var);
    align_toast_list();
}


static void toast_fade_out(lv_obj_t *obj_toast) {
    lv_obj_remove_event_cb(obj_toast, toast_click_callback);
    lv_anim_delete(obj_toast, fade_anim_cb);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj_toast);
    lv_anim_set_values(&a, lv_obj_get_style_opa(obj_toast, 0), LV_OPA_TRANSP);
    lv_anim_set_exec_cb(&a, fade_anim_cb);
    lv_anim_set_completed_cb(&a, delete_var_anim_cb);
    lv_anim_set_duration(&a, 500);
    lv_anim_start(&a);
}


static void toast_fade_out_timer_cb(lv_timer_t *timer) {
    toast_fade_out(lv_timer_get_user_data(timer));
}


static void view_watcher_cb(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg) {
    (void)old_value;
    (void)memory;
    (void)size;
    (void)user_ptr;
    view_event((view_event_t){.tag = VIEW_EVENT_TAG_PAGE_WATCHER, .as.page_watcher.code = (uintptr_t)arg});
}


static void retry_communication_callback(lv_event_t *event) {
    state.communication_attempts++;
    state.last_communication_attempt = timestamp_get();
    state.view_protocol.retry_communication(lv_event_get_user_data(event));
    view_common_set_hidden(state.popup_communication_error.blanket, 1);
}


static void ignore_communication_error_callback(lv_event_t *event) {
    mut_model_t *model                      = view_get_model(lv_event_get_user_data(event));
    model->run.minion.communication_enabled = 0;
    view_common_set_hidden(state.popup_communication_error.blanket, 1);
}


static void toast_click_callback(lv_event_t *event) {
    lv_timer_delete(lv_event_get_user_data(event));
    toast_fade_out(lv_event_get_target(event));
}
