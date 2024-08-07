#include "lvgl.h"
#include "style.h"


static const lv_style_const_prop_t style_transparent_container_props[] = {
    LV_STYLE_CONST_RADIUS(0),
    LV_STYLE_CONST_BORDER_WIDTH(0),
    LV_STYLE_CONST_BG_OPA(LV_OPA_TRANSP),
};
LV_STYLE_CONST_INIT(style_transparent_container, style_transparent_container_props);