#include <Arduino.h>
#include <cstdio>
#include "ui_weather.h"
#include "ui_helpers.h"
#include "../ui/theme.h"

static lv_obj_t *temp_val_label = nullptr;
static lv_obj_t *cond_label = nullptr;
static lv_obj_t *hum_val_label = nullptr;
static lv_obj_t *wind_val_label = nullptr;
static lv_obj_t *sunrise_label = nullptr;
static lv_obj_t *sunset_label = nullptr;

void create_weather_panel(lv_obj_t *parent) {
    Serial.println("[Weather] Start");
    
    // Main panel container
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(panel, t.surface, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, t.border, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(panel, 6, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    
    Serial.println("[Weather] Header");
    // Simple header
    create_label(panel, "WEATHER", &lv_font_montserrat_12, t.accent);
    
    Serial.println("[Weather] Temp");
    // Temperature
    temp_val_label = create_label(panel, "72F", &lv_font_montserrat_32, t.text);
    lv_obj_set_style_text_letter_space(temp_val_label, -1, 0);
    
    Serial.println("[Weather] Condition");
    // Condition
    cond_label = create_label(panel, "Partly Cloudy", &lv_font_montserrat_12, t.text_sub);
    
    Serial.println("[Weather] Humidity");
    // Humidity
    hum_val_label = create_label(panel, "HUM 58%", &lv_font_montserrat_10, t.text_muted);
    
    Serial.println("[Weather] Wind");
    // Wind
    wind_val_label = create_label(panel, "WIND 9mph", &lv_font_montserrat_10, t.text_muted);
    
    Serial.println("[Weather] Sunrise");
    // Sunrise
    sunrise_label = create_label(panel, "Rise 6:42AM", &lv_font_montserrat_10, t.text_sub);
    
    Serial.println("[Weather] Sunset");
    // Sunset
    sunset_label = create_label(panel, "Set 8:14PM", &lv_font_montserrat_10, t.text_sub);
    
    Serial.println("[Weather] Forecast labels");
    // Simple 3-day forecast
    create_label(panel, "TODAY > 95F", &lv_font_montserrat_10, t.accent);
    create_label(panel, "Mon 8/15  93F", &lv_font_montserrat_10, t.text_sub);
    create_label(panel, "Tue 8/16  90F", &lv_font_montserrat_10, t.text_sub);
    
    Serial.println("[Weather] Done");
}

void ui_set_weather_current(int temp, int feelsLike, int humidity, int windSpeed, int cloudCover, const char *condition, const char *icon) {
    if (temp_val_label) { char buf[8]; snprintf(buf, sizeof(buf), "%dF", temp); lv_label_set_text(temp_val_label, buf); }
    if (cond_label) lv_label_set_text(cond_label, condition);
    if (hum_val_label) { char buf[16]; snprintf(buf, sizeof(buf), "HUM %d%%", humidity); lv_label_set_text(hum_val_label, buf); }
    if (wind_val_label) { char buf[16]; snprintf(buf, sizeof(buf), "WIND %dmph", windSpeed); lv_label_set_text(wind_val_label, buf); }
}

void ui_set_weather_sun(const char *sunrise, const char *sunset) {
    if (sunrise_label) { char buf[32]; snprintf(buf, sizeof(buf), "Rise %s", sunrise); lv_label_set_text(sunrise_label, buf); }
    if (sunset_label) { char buf[32]; snprintf(buf, sizeof(buf), "Set %s", sunset); lv_label_set_text(sunset_label, buf); }
}

void ui_set_weather_hourly(int *temps, int count) { (void)temps; (void)count; }
void ui_set_weather_daily(const char **days, int *maxTemps, int count) { (void)days; (void)maxTemps; (void)count; }