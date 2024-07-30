#ifndef VIEW_H_INCLUDED
#define VIEW_H_INCLUDED

#include "page_manager.h"
#include "model/model.h"


typedef struct {
    int id;
    int number;
} view_object_data_t;


typedef struct {
    void (*hello)(void);
} view_protocol_t;


typedef enum {
    VIEW_EVENT_TAG_STORAGE_OPERATION_COMPLETED,
} view_event_tag_t;


typedef struct {
    view_event_tag_t tag;
    union {
        struct {
            int code;
        } storage_operation;
    } as;
} view_event_t;


void             view_init(model_t *model, view_protocol_t protocol);
view_protocol_t *view_get_protocol(pman_handle_t handle);
void             view_change_page(const pman_page_t *page);
mut_model_t     *view_get_model(void *handle);
void             view_register_object_default_callback(lv_obj_t *obj, int id);
void             view_register_object_default_callback_with_number(lv_obj_t *obj, int id, int number);
void             view_event(view_event_t event);

extern const pman_page_t page_main, page_main_execution, page_main_settings, page_info, page_program, page_main_programs;

#endif
