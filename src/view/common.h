#ifndef VIEW_COMMON_H_INCLUDED
#define VIEW_COMMON_H_INCLUDED


#include "lvgl.h"
#include "model/model.h"

lv_obj_t* view_common_create_datetime_widget(lv_obj_t* parent, lv_coord_t x, lv_coord_t y);

lv_obj_t* view_common_create_logo_widget(lv_obj_t* parent);

lv_obj_t* view_common_create_folder_widget(lv_obj_t* parent, lv_align_t align, lv_coord_t x, lv_coord_t y);

#endif