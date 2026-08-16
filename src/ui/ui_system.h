#ifndef UI_SYSTEM_H
#define UI_SYSTEM_H

#include <lvgl.h>
#include <Arduino.h>

extern lv_obj_t *sys_disp_ram;
extern lv_obj_t *sys_disp_psram;
extern lv_obj_t *sys_stamp_ram;
extern lv_obj_t *sys_stamp_psram;

void create_system_widget(lv_obj_t *parent);
void update_display_cpu_stats_impl();
void ui_set_ram_stats_impl(uint32_t eleHeap, uint32_t elePsram, uint32_t stampHeap);

#endif