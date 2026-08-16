#ifndef UI_HEADER_H
#define UI_HEADER_H

#include <lvgl.h>

extern lv_obj_t *time_label;
extern lv_obj_t *ssid_label;

void create_header(lv_obj_t *parent);

#endif