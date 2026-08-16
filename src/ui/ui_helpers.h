#ifndef UI_HELPERS_H
#define UI_HELPERS_H

#include <lvgl.h>
#include "../ui/theme.h"

lv_obj_t *create_wifi_bars(lv_obj_t *parent, lv_color_t color, int strength);
lv_obj_t *create_divider(lv_obj_t *parent);
lv_obj_t *create_label(lv_obj_t *parent, const char *txt, const lv_font_t *font, lv_color_t color);
lv_obj_t *create_clean_container(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, lv_flex_flow_t flow);
lv_obj_t *create_tab_btn(lv_obj_t *parent, const char *text, const lv_font_t *font);

#endif