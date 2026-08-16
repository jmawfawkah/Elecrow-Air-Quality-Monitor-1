#ifndef UI_WEATHER_H
#define UI_WEATHER_H

#include <lvgl.h>

void create_weather_panel(lv_obj_t *parent);

// Setters for weather data (to be wired up later)
void ui_set_weather_current(int temp, int feelsLike, int humidity, int windSpeed, int cloudCover, const char *condition, const char *icon);
void ui_set_weather_sun(const char *sunrise, const char *sunset);
void ui_set_weather_hourly(int *temps, int count);
void ui_set_weather_daily(const char **days, int *maxTemps, int count);

#endif