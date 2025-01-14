#ifndef STYLE_H_INCLUDED
#define STYLE_H_INCLUDED


#include "lvgl.h"


#define STYLE_FONT_SMALL  (&lv_font_montserrat_16)
#define STYLE_FONT_MEDIUM (&lv_font_montserrat_24)
#define STYLE_FONT_BIG    (&lv_font_montserrat_32)
#define STYLE_FONT_HUGE   (&font_montserrat_32)

#define STYLE_COLOR_PRIMARY ((lv_color_t)LV_COLOR_MAKE(0x41, 0x63, 0x89))
#define STYLE_COLOR_RED     ((lv_color_t)LV_COLOR_MAKE(0xFF, 0x00, 0x0))
#define STYLE_COLOR_GREEN   ((lv_color_t)LV_COLOR_MAKE(0x0, 0xFF, 0x0))
#define STYLE_COLOR_BLUE    ((lv_color_t)LV_COLOR_MAKE(0x0, 0x0, 0xFF))
#define STYLE_COLOR_YELLOW  ((lv_color_t)LV_COLOR_MAKE(0xFF, 0xFF, 0x00))
#define STYLE_COLOR_CYAN    ((lv_color_t)LV_COLOR_MAKE(0x0, 0xBB, 0xFF))
#define STYLE_COLOR_BLACK   ((lv_color_t)LV_COLOR_MAKE(0x0, 0x0, 0x0))


extern const lv_style_t style_transparent_cont;
extern const lv_style_t style_padless_cont;
extern const lv_style_t style_time_bar;
extern const lv_style_t style_time_unit;


#endif
