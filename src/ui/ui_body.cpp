#include "ui_body.h"
#include "ui_helpers.h"
#include "ui_aqi.h"
#include "ui_system.h"
#include "ui_sensors.h"
#include "../ui/theme.h"
#include "ui_blowup.h"
#include "ui_weather.h"

lv_obj_t *blowup_container = nullptr;

void create_body(lv_obj_t *parent) {
    Serial.println("[Body] Creating body container...");
    lv_obj_t *body = lv_obj_create(parent);
    lv_obj_set_size(body, 800, 448);
    lv_obj_align(body, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 4, 0);
    lv_obj_set_style_pad_gap(body, 4, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    // Column 1: Left Sidebar (162px)
    Serial.println("[Body] Creating Column 1 (AQI/System)...");
    lv_obj_t *col_left = lv_obj_create(body);
    lv_obj_set_width(col_left, 162);
    lv_obj_set_height(col_left, lv_pct(100));
    lv_obj_set_style_bg_opa(col_left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col_left, 0, 0);
    lv_obj_set_style_pad_all(col_left, 0, 0);
    lv_obj_set_style_pad_gap(col_left, 5, 0);
    lv_obj_set_flex_flow(col_left, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(col_left, LV_OBJ_FLAG_SCROLLABLE);
    
    create_aqi_widget(col_left);
    create_system_widget(col_left);

    // Column 2: Left Sensors (72px)
    Serial.println("[Body] Creating Column 2 (Sensors Left)...");
    lv_obj_t *col_sensors_l = lv_obj_create(body);
    lv_obj_set_width(col_sensors_l, 72);
    lv_obj_set_height(col_sensors_l, lv_pct(100));
    lv_obj_set_style_bg_opa(col_sensors_l, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col_sensors_l, 0, 0);
    lv_obj_set_style_pad_all(col_sensors_l, 0, 0);
    lv_obj_set_style_pad_gap(col_sensors_l, 4, 0);
    lv_obj_set_flex_flow(col_sensors_l, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(col_sensors_l, LV_OBJ_FLAG_SCROLLABLE);

    // Column 3: Center Blowup (flex grow)
    Serial.println("[Body] Creating Column 3 (Blowup)...");
    blowup_container = lv_obj_create(body);
    lv_obj_set_flex_grow(blowup_container, 1);
    lv_obj_set_height(blowup_container, lv_pct(100));
    lv_obj_set_style_bg_color(blowup_container, t.surface, 0);
    lv_obj_set_style_bg_opa(blowup_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(blowup_container, t.border, 0);
    lv_obj_set_style_border_width(blowup_container, 1, 0);
    lv_obj_set_style_radius(blowup_container, 10, 0);
    lv_obj_set_style_pad_all(blowup_container, 0, 0);
    lv_obj_clear_flag(blowup_container, LV_OBJ_FLAG_SCROLLABLE);

    create_blowup_panel(blowup_container);

    // Column 4: Right Sensors (72px)
    Serial.println("[Body] Creating Column 4 (Sensors Right)...");
    lv_obj_t *col_sensors_r = lv_obj_create(body);
    lv_obj_set_width(col_sensors_r, 72);
    lv_obj_set_height(col_sensors_r, lv_pct(100));
    lv_obj_set_style_bg_opa(col_sensors_r, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col_sensors_r, 0, 0);
    lv_obj_set_style_pad_all(col_sensors_r, 0, 0);
    lv_obj_set_style_pad_gap(col_sensors_r, 4, 0);
    lv_obj_set_flex_flow(col_sensors_r, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(col_sensors_r, LV_OBJ_FLAG_SCROLLABLE);

    // Column 5: Weather Panel (162px)
    Serial.println("[Body] Creating Column 5 (Weather)...");
    lv_obj_t *col_weather = lv_obj_create(body);
    lv_obj_set_width(col_weather, 162);
    lv_obj_set_height(col_weather, lv_pct(100));
    lv_obj_set_style_bg_opa(col_weather, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col_weather, 0, 0);
    lv_obj_set_style_pad_all(col_weather, 0, 0);
    lv_obj_set_flex_flow(col_weather, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(col_weather, LV_OBJ_FLAG_SCROLLABLE);

    create_weather_panel(col_weather);
    
    // Populate sensor chips
    Serial.println("[Body] Populating sensor chips...");
    create_sensor_chips(col_sensors_l, col_sensors_r);
    
    Serial.println("[Body] Body creation complete!");
}