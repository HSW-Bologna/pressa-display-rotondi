#ifndef VIEW_H_INCLUDED
#define VIEW_H_INCLUDED

#include "page_manager.h"
#include "model/model.h"


#define VIEW_ADD_WATCHED_VARIABLE(ptr, code)     view_add_watched_variable((void *)ptr, sizeof(*ptr), code)
#define VIEW_ADD_WATCHED_ARRAY(ptr, items, code) view_add_watched_variable((void *)ptr, sizeof(*ptr) * items, code)


typedef struct {
    void (*set_test_mode)(pman_handle_t handle, uint8_t test_mode);
    void (*test_output)(pman_handle_t handle, uint16_t output_index);
    void (*test_output_clear)(pman_handle_t handle);
    void (*test_pwm)(pman_handle_t handle, uint8_t percentage);
    void (*save_configuration)(pman_handle_t handle);
    void (*retry_communication)(pman_handle_t handle);
} view_protocol_t;

typedef enum {
    VIEW_EVENT_TAG_STORAGE_OPERATION_COMPLETED,
    VIEW_EVENT_TAG_PAGE_WATCHER,
} view_event_tag_t;

typedef struct {
    view_event_tag_t tag;
    union {
        struct {
            int code;
        } storage_operation;
        struct {
            int code;
        } page_watcher;
    } as;
} view_event_t;

typedef struct {
    uint8_t *modified;
    uint16_t program_index;
} view_page_program_arg_t;


void             view_init(model_t *model, view_protocol_t protocol);
view_protocol_t *view_get_protocol(pman_handle_t handle);
void             view_change_page(const pman_page_t *page);
mut_model_t     *view_get_model(void *handle);
void             view_register_object_default_callback(lv_obj_t *obj, int id);
void             view_register_object_default_callback_with_number(lv_obj_t *obj, int id, int number);
void             view_event(view_event_t event);
void             view_add_watched_variable(void *ptr, size_t size, int code);
void             view_manage(model_t *model);
uint16_t         view_get_obj_id(lv_obj_t *obj);
uint16_t         view_get_obj_number(lv_obj_t *obj);


extern const pman_page_t page_home, page_info, page_settings_home, page_programs_home, page_execution_home,
    page_execution_programs, page_programs_setup, page_test, page_config, page_program, page_choice, page_execution;


#endif
