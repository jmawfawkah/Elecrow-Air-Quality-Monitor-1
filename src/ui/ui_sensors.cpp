#include <Arduino.h>
#include <cstdio>
#include "ui_sensors.h"
#include "ui_helpers.h"
#include "../ui/theme.h"
#include "ui_blowup.h"
#include "ui.h"

const SensorDef sensors[SENSOR_COUNT] = {
    {"eco2", "CO2",    "ppm",  400,  5000, 1000, 2000,  "0x58", 0}, // SGP30 eCO2
    {"tvoc", "TVOC",   "ppb",  0,    5000, 1000, 3000,  "0x58", 0}, // SGP30 TVOC
    {"voc",  "VOC",    "idx",  0,    500,  100,  250,   "0x59", 0}, // SGP40 VOC Index
    {"hum",  "HUM",    "%RH",  0,    100,  60,   80,    "0x38", 1}, // AHT20 Humidity
    {"temp", "TEMP",   "F",    32,   120,  77,   86,    "0x38", 1}, // AHT20 Temp
    {"co",   "CO",     "ppm",  1,    1000, 35,   200,   "0x75", 0},
    {"no2",  "NO2",    "ppm",  0.05, 10,   0.1,  0.5,   "0x75", 2},
    {"eth",  "C2H5OH", "ppm",  10,   500,  50,   200,   "0x75", 0},
    {"h2",   "H2",     "ppm",  1,    1000, 50,   300,   "0x75", 0},
    {"nh3",  "NH3",    "ppm",  1,    500,  25,   50,    "0x75", 0},
    {"ch4",  "CH4",    "ppm",  1000, 5000, 1500, 3000,  "0x75", 0},
};

// ── Custom Hybrid AQI Calculation (0-100 scale) ──────────────────────────────
struct AQIResult {
    uint8_t score;
    const char *label;
    lv_color_t color;
};

static AQIResult calculate_aqi() {
    float sum = 0;
    int count = 0;
    
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (!sensor_connected[i]) continue;
        
        const SensorDef *def = &sensors[i];
        float val = sensor_values[i];
        
        // Normalize: 0 = at min (good), 1 = at max (bad)
        float normalized = (val - def->min) / (def->max - def->min);
        if (normalized < 0) normalized = 0;
        if (normalized > 1) normalized = 1;
        
        sum += normalized;
        count++;
    }
    
    if (count == 0) {
        return { 0, "NO DATA", t.text_muted };
    }
    
    float avg = sum / count;
    uint8_t score = (uint8_t)((1.0 - avg) * 100);
    
    if (score >= 80) return { score, "EXCELLENT", t.good };
    if (score >= 60) return { score, "GOOD", lv_color_hex(0x84cc16) };
    if (score >= 40) return { score, "MODERATE", t.warn };
    if (score >= 20) return { score, "POOR", lv_color_hex(0xf97316) };
    return { score, "HAZARDOUS", t.danger };
}

struct SensorChip {
    lv_obj_t *container;
    lv_obj_t *value_label;
    lv_obj_t *status_dot;
    lv_obj_t *status_text;
    lv_obj_t *progress_bar;
    lv_obj_t *border_obj;
};
static SensorChip chips[SENSOR_COUNT];
float sensor_values[SENSOR_COUNT] = {0};
bool sensor_connected[SENSOR_COUNT] = {false};

static lv_obj_t *insp_chip = nullptr;
static lv_obj_t *insp_badge = nullptr;
static lv_obj_t *insp_bar = nullptr;

lv_color_t get_status_color(float val, float goodMax, float warnMax) {
    if (val <= goodMax) return t.good;
    if (val <= warnMax) return t.warn;
    return t.danger;
}

const char *get_status_text(float val, float goodMax, float warnMax) {
    if (val <= goodMax) return "GOOD";
    if (val <= warnMax) return "MOD";
    return "POOR";
}

static void sensor_chip_cb(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    Serial.printf("[UI] Sensor chip %d (%s) tapped\n", idx, sensors[idx].id);
    show_blowup_view(BLOWUP_SENSOR, idx);
}

static void inspiration_chip_cb(lv_event_t *e) {
    (void)e;
    Serial.println("[UI] Inspiration chip tapped");
    show_blowup_view(BLOWUP_QUOTE);
}

static void create_sensor_chip(lv_obj_t *parent, int idx) {
    Serial.printf("[Chip %d] Start (Free RAM: %d)\n", idx, ESP.getFreeHeap());
    const SensorDef *def = &sensors[idx];

    SensorChip *chip = &chips[idx];

    Serial.printf("[Chip %d] Creating button\n", idx);
    lv_obj_t *cont = lv_btn_create(parent);
    chip->container = cont;
    chip->border_obj = cont;
    lv_obj_set_width(cont, lv_pct(100));
    lv_obj_set_flex_grow(cont, 1);
    lv_obj_set_style_bg_color(cont, t.surface, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cont, t.good, 0);
    lv_obj_set_style_border_opa(cont, 51, 0);
    lv_obj_set_style_border_width(cont, 1, 0);
    lv_obj_set_style_radius(cont, 8, 0);
    lv_obj_set_style_pad_all(cont, 5, 0);
    lv_obj_set_style_pad_hor(cont, 7, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(cont, 2, 0);
    lv_obj_set_style_shadow_width(cont, 0, 0);
    
    // Disable press animation for tearing test
    lv_obj_set_style_bg_color(cont, t.surface, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(cont, t.good, LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(cont, 51, LV_STATE_PRESSED);

    Serial.printf("[Chip %d] Adding event\n", idx);
    lv_obj_add_event_cb(cont, sensor_chip_cb, LV_EVENT_CLICKED, (void*)(intptr_t)idx);

    Serial.printf("[Chip %d] Creating row1\n", idx);
    lv_obj_t *row1 = create_clean_container(cont, lv_pct(100), LV_SIZE_CONTENT, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    create_label(row1, def->label, &lv_font_montserrat_10, t.text_muted);

    lv_obj_t *right = create_clean_container(row1, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(right, 3, 0);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    create_label(right, def->addr, &lv_font_montserrat_10, t.text_dim);

    Serial.printf("[Chip %d] Creating dot\n", idx);
    chip->status_dot = lv_obj_create(right);
    lv_obj_set_size(chip->status_dot, 5, 5);
    lv_obj_set_style_radius(chip->status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(chip->status_dot, t.good, 0);
    lv_obj_set_style_bg_opa(chip->status_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chip->status_dot, 0, 0);
    lv_obj_set_style_pad_all(chip->status_dot, 0, 0);
    lv_obj_set_style_shadow_color(chip->status_dot, t.good, 0);
    lv_obj_set_style_shadow_width(chip->status_dot, 6, 0);
    lv_obj_set_style_shadow_opa(chip->status_dot, 178, 0);

    Serial.printf("[Chip %d] Creating row2\n", idx);
    lv_obj_t *row2 = create_clean_container(cont, lv_pct(100), LV_SIZE_CONTENT, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row2, 4, 0);

    chip->value_label = create_label(row2, "--", &lv_font_montserrat_16, t.good);
    lv_obj_set_style_text_letter_space(chip->value_label, -1, 0);

    create_label(row2, def->unit, &lv_font_montserrat_10, t.text_muted);

    Serial.printf("[Chip %d] Creating badge\n", idx);
    chip->status_text = lv_label_create(cont);
    lv_label_set_text(chip->status_text, "GOOD");
    lv_obj_set_style_text_font(chip->status_text, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(chip->status_text, t.good, 0);
    lv_obj_set_style_bg_color(chip->status_text, t.good, 0);
    lv_obj_set_style_bg_opa(chip->status_text, 38, 0);
    lv_obj_set_style_pad_hor(chip->status_text, 4, 0);
    lv_obj_set_style_pad_ver(chip->status_text, 1, 0);
    lv_obj_set_style_radius(chip->status_text, 3, 0);
    lv_obj_set_style_border_width(chip->status_text, 0, 0);

    Serial.printf("[Chip %d] Creating bar\n", idx);
    chip->progress_bar = lv_bar_create(cont);
    lv_obj_set_width(chip->progress_bar, lv_pct(100));
    lv_obj_set_height(chip->progress_bar, 2);
    lv_bar_set_range(chip->progress_bar, 0, 100);
    lv_bar_set_value(chip->progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(chip->progress_bar, t.good, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(chip->progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(chip->progress_bar, t.text_dim, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(chip->progress_bar, 51, LV_PART_MAIN);
    lv_obj_set_style_radius(chip->progress_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(chip->progress_bar, 1, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(chip->progress_bar, 0, LV_PART_MAIN);
    
    Serial.printf("[Chip %d] Done\n", idx);
}

static void create_inspiration_chip(lv_obj_t *parent) {
    Serial.println("[Insp] Creating button...");
    insp_chip = lv_btn_create(parent);
    lv_obj_set_width(insp_chip, lv_pct(100));
    lv_obj_set_flex_grow(insp_chip, 1);
    lv_obj_set_style_bg_color(insp_chip, t.surface, 0);
    lv_obj_set_style_bg_opa(insp_chip, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(insp_chip, t.border, 0);
    lv_obj_set_style_border_width(insp_chip, 1, 0);
    lv_obj_set_style_radius(insp_chip, 8, 0);
    lv_obj_set_style_pad_all(insp_chip, 5, 0);
    lv_obj_set_style_pad_hor(insp_chip, 7, 0);
    lv_obj_set_flex_flow(insp_chip, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(insp_chip, 2, 0);
    lv_obj_set_style_shadow_width(insp_chip, 0, 0);
    
    // Disable press animation for tearing test
    lv_obj_set_style_bg_color(insp_chip, t.surface, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(insp_chip, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(insp_chip, t.border, LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(insp_chip, LV_OPA_COVER, LV_STATE_PRESSED);

    Serial.println("[Insp] Adding event callback...");
    lv_obj_add_event_cb(insp_chip, inspiration_chip_cb, LV_EVENT_CLICKED, nullptr);

    Serial.println("[Insp] Creating labels...");
    create_label(insp_chip, "INSPIRATION", &lv_font_montserrat_10, t.text_muted);
    create_label(insp_chip, LV_SYMBOL_AUDIO, &lv_font_montserrat_16, t.text_muted);

    Serial.println("[Insp] Creating badge...");
    insp_badge = lv_label_create(insp_chip);
    lv_label_set_text(insp_badge, "TAP TO READ");
    lv_obj_set_style_text_font(insp_badge, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(insp_badge, t.text_dim, 0);

    Serial.println("[Insp] Creating progress bar...");
    insp_bar = lv_bar_create(insp_chip);
    lv_obj_set_width(insp_bar, lv_pct(100));
    lv_obj_set_height(insp_bar, 2);
    lv_bar_set_range(insp_bar, 0, 100);
    lv_bar_set_value(insp_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(insp_bar, t.good, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(insp_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(insp_bar, t.text_dim, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(insp_bar, 51, LV_PART_MAIN);
    lv_obj_set_style_radius(insp_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(insp_bar, 1, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(insp_bar, 0, LV_PART_MAIN);
    
    Serial.println("[Insp] Done.");
}

void create_sensor_chips(lv_obj_t *parent_left, lv_obj_t *parent_right) {
    Serial.println("[Sensors] Creating left chips...");
    // 6 sensors on the left
    for (int i = 0; i < 6; i++) {
        Serial.printf("[Sensors] Creating chip %d\n", i);
        create_sensor_chip(parent_left, i);
    }
    Serial.println("[Sensors] Creating right chips...");
    // 5 sensors on the right
    for (int i = 6; i < 11; i++) {
        Serial.printf("[Sensors] Creating chip %d\n", i);
        create_sensor_chip(parent_right, i);
    }
    Serial.println("[Sensors] Creating inspiration chip...");
    create_inspiration_chip(parent_right);
    Serial.println("[Sensors] Done.");
}

void update_sensor_chip(int idx, float value, bool connected) {
    if (idx < 0 || idx >= SENSOR_COUNT) return;
    
    sensor_values[idx] = value;
    sensor_connected[idx] = connected;
    
    SensorChip *chip = &chips[idx];
    const SensorDef *def = &sensors[idx];
    
    if (!chip->value_label) return;
    
    lv_color_t color = connected ? get_status_color(value, def->goodMax, def->warnMax) : t.text_dim;
    const char *status = connected ? get_status_text(value, def->goodMax, def->warnMax) : "---";
    
    if (connected) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.*f", def->decimals, value);
        lv_label_set_text(chip->value_label, buf);
    } else {
        lv_label_set_text(chip->value_label, "--");
    }
    lv_obj_set_style_text_color(chip->value_label, color, 0);
    
    lv_obj_set_style_bg_color(chip->status_dot, color, 0);
    lv_obj_set_style_shadow_color(chip->status_dot, color, 0);
    
    lv_label_set_text(chip->status_text, status);
    lv_obj_set_style_text_color(chip->status_text, color, 0);
    lv_obj_set_style_bg_color(chip->status_text, color, 0);
    
    lv_obj_set_style_border_color(chip->border_obj, color, 0);
    
    if (connected) {
        float pct = ((value - def->min) / (def->max - def->min)) * 100.0f;
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        lv_bar_set_value(chip->progress_bar, (int)pct, LV_ANIM_OFF);
    } else {
        lv_bar_set_value(chip->progress_bar, 0, LV_ANIM_OFF);
    }
    lv_obj_set_style_bg_color(chip->progress_bar, color, LV_PART_INDICATOR);
    
    record_sensor_history(idx, value);

    AQIResult aqi = calculate_aqi();
    ui_set_aqi(aqi.score, aqi.label, aqi.color);
}

// --- UI Setter Implementations ---
// Mapped to the new array indices above

void ui_set_eco2_impl(uint16_t ppm) { update_sensor_chip(0, (float)ppm, true); }
void ui_set_tvoc_impl(uint16_t ppb) { update_sensor_chip(1, (float)ppb, true); }
void ui_set_voc_index_impl(uint16_t index) { update_sensor_chip(2, (float)index, true); }
void ui_set_humidity_impl(float val) { update_sensor_chip(3, val, true); }
void ui_set_temp_impl(float val) { update_sensor_chip(4, val, true); }
void ui_set_co_impl(float val) { update_sensor_chip(5, val, true); }
void ui_set_no2_impl(float val) { update_sensor_chip(6, val, true); }
void ui_set_eth_impl(float val) { update_sensor_chip(7, val, true); }
void ui_set_h2_impl(float val) { update_sensor_chip(8, val, true); }
void ui_set_nh3_impl(float val) { update_sensor_chip(9, val, true); }
void ui_set_ch4_impl(float val) { update_sensor_chip(10, val, true); }