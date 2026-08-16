#ifndef UI_SENSORS_H
#define UI_SENSORS_H

#include <lvgl.h>

#define SENSOR_COUNT 11

struct SensorDef {
    const char *id;
    const char *label;
    const char *unit;
    float min, max, goodMax, warnMax;
    const char *addr;
    uint8_t decimals;
};

extern const SensorDef sensors[SENSOR_COUNT];
extern float sensor_values[SENSOR_COUNT];
extern bool sensor_connected[SENSOR_COUNT];

lv_color_t get_status_color(float val, float goodMax, float warnMax);
const char *get_status_text(float val, float goodMax, float warnMax);

void create_sensor_chips(lv_obj_t *parent_left, lv_obj_t *parent_right);
void update_sensor_chip(int idx, float value, bool connected);
void record_sensor_history(int idx, float value);

void ui_set_voc_impl(uint16_t val);
void ui_set_humidity_impl(float val);
void ui_set_temp_impl(float val);
void ui_set_co_impl(float val);
void ui_set_no2_impl(float val);
void ui_set_eth_impl(float val);
void ui_set_h2_impl(float val);
void ui_set_nh3_impl(float val);
void ui_set_ch4_impl(float val);
void ui_set_eco2_impl(uint16_t ppm);
void ui_set_tvoc_impl(uint16_t ppb);
void ui_set_voc_index_impl(uint16_t index);

#endif