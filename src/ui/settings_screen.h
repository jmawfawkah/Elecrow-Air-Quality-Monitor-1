#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include <lvgl.h>

// Global settings state
struct AppSettings {
    bool darkMode;
    bool alertsEnabled;
    int refreshRate; // 1, 3, 5, or 10 seconds
};

extern AppSettings appSettings;

// Global calibration offsets (used by aht20.cpp)
extern float aht20_temp_offset;
extern float aht20_hum_offset;

void show_settings_screen();
void hide_settings_screen();

#endif