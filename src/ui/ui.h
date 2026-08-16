#ifndef UI_H
#define UI_H

#include <lvgl.h>
#include <Arduino.h>
#include "ui_blowup.h"
#include "ui_weather.h"

extern lv_obj_t *main_screen;
extern lv_obj_t *wifi_screen;

void ui_init();
void ui_set_wifi(bool connected, const char *ssid = nullptr);
void ui_set_time(const char *time_str);
void ui_set_ram_stats(uint32_t eleHeap, uint32_t elePsram, uint32_t stampHeap);
void update_display_cpu_stats();

void ui_set_status(const char *msg);
void ui_set_outdoor_aqi(uint16_t aqi);
void ui_set_eco2(uint16_t ppm);
void ui_set_tvoc(uint16_t ppb);
void ui_set_voc_index(uint16_t index);

void ui_set_aqi(uint8_t score, const char *label, lv_color_t color);
void ui_set_voc(uint16_t val);
void ui_set_humidity(float val);
void ui_set_temp(float val);
void ui_set_co(float val);
void ui_set_no2(float val);
void ui_set_eth(float val);
void ui_set_h2(float val);
void ui_set_nh3(float val);
void ui_set_ch4(float val);
void ui_set_eco2(int eco2);
void ui_set_temp(float temp);
void ui_set_humidity(float humidity);

void ui_show_blowup(int view, int sensorIndex = 0);

#endif