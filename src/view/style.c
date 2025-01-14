#include "lvgl.h"
#include "style.h"


static const lv_style_const_prop_t style_transparent_cont_props[] = {
    LV_STYLE_CONST_PAD_BOTTOM(0), LV_STYLE_CONST_PAD_TOP(0),      LV_STYLE_CONST_PAD_LEFT(0),
    LV_STYLE_CONST_PAD_RIGHT(0),  LV_STYLE_CONST_BORDER_WIDTH(0), LV_STYLE_CONST_BG_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_PROPS_END,
};
LV_STYLE_CONST_INIT(style_transparent_cont, (void *)style_transparent_cont_props);

static const lv_style_const_prop_t style_padless_cont_props[] = {
    LV_STYLE_CONST_PAD_BOTTOM(0), LV_STYLE_CONST_PAD_TOP(0), LV_STYLE_CONST_PAD_LEFT(0),
    LV_STYLE_CONST_PAD_RIGHT(0),  LV_STYLE_CONST_PROPS_END,
};
LV_STYLE_CONST_INIT(style_padless_cont, (void *)style_padless_cont_props);

static const lv_style_const_prop_t style_time_bar_props[] = {
    LV_STYLE_CONST_PAD_BOTTOM(0),         LV_STYLE_CONST_PAD_TOP(0),      LV_STYLE_CONST_PAD_LEFT(0),
    LV_STYLE_CONST_PAD_RIGHT(0),          LV_STYLE_CONST_BORDER_WIDTH(0), LV_STYLE_CONST_BG_COLOR(STYLE_COLOR_BLUE),
    LV_STYLE_CONST_BG_OPA(LV_OPA_70), LV_STYLE_CONST_RADIUS(0),       LV_STYLE_CONST_PROPS_END,
};
LV_STYLE_CONST_INIT(style_time_bar, (void *)style_time_bar_props);

static const lv_style_const_prop_t style_time_unit_props[] = {
    LV_STYLE_CONST_PAD_BOTTOM(0),
    LV_STYLE_CONST_PAD_TOP(0),
    LV_STYLE_CONST_PAD_LEFT(0),
    LV_STYLE_CONST_PAD_RIGHT(0),
    LV_STYLE_CONST_BORDER_WIDTH(0),
    LV_STYLE_CONST_BORDER_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_BG_COLOR(STYLE_COLOR_CYAN),
    LV_STYLE_CONST_RADIUS(0),
    LV_STYLE_CONST_PROPS_END,
};
LV_STYLE_CONST_INIT(style_time_unit, (void *)style_time_unit_props);
