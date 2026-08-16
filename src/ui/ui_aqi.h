#ifndef UI_AQI_H
#define UI_AQI_H

#include <lvgl.h>

extern lv_obj_t *aqi_score_label;
extern lv_obj_t *aqi_status_label;

void create_aqi_widget(lv_obj_t *parent);
void ui_set_aqi_impl(uint8_t score, const char *label, lv_color_t color);

#endif