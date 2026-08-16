#ifndef UI_BLOWUP_H
#define UI_BLOWUP_H

#include <lvgl.h>
#include "ui_sensors.h"

extern int current_blowup_view;
extern int selected_sensor_idx;

void create_blowup_panel(lv_obj_t *parent);
void show_blowup_view(int view_idx, int sensor_idx = -1);

// View indices
#define BLOWUP_QUOTE       0
#define BLOWUP_SENSOR      1
#define BLOWUP_FIRETV      2
#define BLOWUP_WEATHER     3
#define BLOWUP_PORTS       4
#define BLOWUP_SDCARD      5
#define BLOWUP_HOURLY      6

#endif