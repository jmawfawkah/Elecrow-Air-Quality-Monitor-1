#include <Arduino.h>
#include <cstdio>
#include <cmath>
#include "ui_blowup.h"
#include "ui_helpers.h"
#include "../ui/theme.h"

void update_sensor_chart_display(int idx);

int current_blowup_view = BLOWUP_QUOTE;
int selected_sensor_idx = -1;

static lv_obj_t *blowup_views[7] = {nullptr};
static lv_obj_t *blowup_border_obj = nullptr;

// ── Quote View Elements ──────────────────────────────────────────────────────
static lv_obj_t *quote_text_label = nullptr;
static lv_obj_t *quote_author_label = nullptr;
static lv_obj_t *quote_hint_label = nullptr;
static bool quote_saved = false;
static uint32_t quote_id = 0;

static void quote_tap_cb(lv_event_t *e) {
    (void)e;
    if (quote_saved) return;
    quote_saved = true;
    if (quote_hint_label) {
        lv_label_set_text(quote_hint_label, "SAVED TO SD");
        lv_obj_set_style_text_color(quote_hint_label, t.good, 0);
    }
    Serial.println("[Blowup] Quote saved to SD");
}

static void create_quote_view(lv_obj_t *parent) {
    lv_obj_t *view = lv_btn_create(parent);
    blowup_views[BLOWUP_QUOTE] = view;
    lv_obj_set_size(view, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(view, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(view, 0, 0);
    lv_obj_set_style_shadow_width(view, 0, 0);
    lv_obj_set_style_pad_all(view, 14, 0);
    lv_obj_set_style_pad_hor(view, 16, 0);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(view, 10, 0);
    lv_obj_add_event_cb(view, quote_tap_cb, LV_EVENT_CLICKED, nullptr);

    // Large quote mark
    lv_obj_t *qmark = create_label(view, "\xE2\x80\x9C", &lv_font_montserrat_48, t.accent);
    lv_obj_set_style_text_opa(qmark, 64, 0); // ~25%
    lv_obj_align(qmark, LV_ALIGN_TOP_LEFT, 0, 0);

    // Quote body
    quote_text_label = lv_label_create(view);
    lv_label_set_text(quote_text_label, "Loading quote...");
    lv_obj_set_style_text_font(quote_text_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(quote_text_label, t.text, 0);
    lv_obj_set_style_text_align(quote_text_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(quote_text_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(quote_text_label, lv_pct(100));
    lv_obj_set_flex_grow(quote_text_label, 1);

    // Author
    quote_author_label = create_label(view, "— Author", &lv_font_montserrat_12, t.accent);

    // Save hint
    quote_hint_label = lv_label_create(view);
    lv_label_set_text(quote_hint_label, "TAP TO SAVE TO SD");
    lv_obj_set_style_text_font(quote_hint_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(quote_hint_label, t.text_dim, 0);
}

// ── Sensor Chart View Elements ───────────────────────────────────────────────
static lv_obj_t *sensor_big_value = nullptr;
static lv_obj_t *sensor_name_label = nullptr;
static lv_obj_t *sensor_addr_label = nullptr;
static lv_obj_t *sensor_badge = nullptr;
static lv_obj_t *sensor_chart = nullptr;
static lv_chart_series_t *sensor_series = nullptr;
static lv_obj_t *sensor_avg_label = nullptr;
static lv_obj_t *sensor_min_label = nullptr;
static lv_obj_t *sensor_max_label = nullptr;

#define SENSOR_HISTORY_SIZE 24
static lv_coord_t sensor_history[SENSOR_COUNT][SENSOR_HISTORY_SIZE];
static int sensor_history_idx[SENSOR_COUNT] = {0};
static bool sensor_history_filled[SENSOR_COUNT] = {false};

static void create_sensor_chart_view(lv_obj_t *parent) {
    lv_obj_t *view = lv_obj_create(parent);
    blowup_views[BLOWUP_SENSOR] = view;
    lv_obj_set_size(view, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(view, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(view, 0, 0);
    lv_obj_set_style_pad_all(view, 7, 0);
    lv_obj_set_style_pad_hor(view, 10, 0);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(view, 4, 0);
    lv_obj_add_flag(view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view, LV_OBJ_FLAG_SCROLLABLE);

    // Header row: big value | name + addr + badge
    lv_obj_t *hdr = create_clean_container(view, lv_pct(100), LV_SIZE_CONTENT, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    sensor_big_value = create_label(hdr, "--", &lv_font_montserrat_48, t.good);
    lv_obj_set_style_text_letter_space(sensor_big_value, -2, 0);

    lv_obj_t *right = create_clean_container(hdr, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);

    sensor_name_label = create_label(right, "VOC", &lv_font_montserrat_16, t.text);
    sensor_addr_label = create_label(right, "0x59", &lv_font_montserrat_10, t.text_muted);

    sensor_badge = lv_label_create(right);
    lv_label_set_text(sensor_badge, "GOOD");
    lv_obj_set_style_text_font(sensor_badge, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(sensor_badge, t.good, 0);
    lv_obj_set_style_bg_color(sensor_badge, t.good, 0);
    lv_obj_set_style_bg_opa(sensor_badge, 38, 0);
    lv_obj_set_style_pad_hor(sensor_badge, 6, 0);
    lv_obj_set_style_pad_ver(sensor_badge, 2, 0);
    lv_obj_set_style_radius(sensor_badge, 4, 0);
    lv_obj_set_style_border_width(sensor_badge, 0, 0);

    // Chart
    sensor_chart = lv_chart_create(view);
    lv_obj_set_width(sensor_chart, lv_pct(100));
    lv_obj_set_flex_grow(sensor_chart, 1);
    lv_chart_set_type(sensor_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(sensor_chart, SENSOR_HISTORY_SIZE);
    lv_obj_set_style_size(sensor_chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sensor_chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sensor_chart, 0, 0);
    lv_obj_set_style_line_width(sensor_chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(sensor_chart, 0, 0);
    sensor_series = lv_chart_add_series(sensor_chart, t.good, LV_CHART_AXIS_PRIMARY_Y);

    // Stats row
    lv_obj_t *stats = create_clean_container(view, lv_pct(100), LV_SIZE_CONTENT, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(stats, 16, 0);

    sensor_avg_label = create_label(stats, "AVG --", &lv_font_montserrat_10, t.text_muted);
    sensor_min_label = create_label(stats, "MIN --", &lv_font_montserrat_10, t.text_muted);
    sensor_max_label = create_label(stats, "MAX --", &lv_font_montserrat_10, t.text_muted);
}

// ── Placeholder Views ─────────────────────────────────────────────────────────
static void create_placeholder_view(lv_obj_t *parent, int idx, const char *name) {
    lv_obj_t *view = lv_obj_create(parent);
    blowup_views[idx] = view;
    lv_obj_set_size(view, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(view, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(view, 0, 0);
    lv_obj_clear_flag(view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(view, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *lbl = create_label(view, name, &lv_font_montserrat_24, t.text_muted);
    lv_obj_center(lbl);
}

// ── View Switching ────────────────────────────────────────────────────────────
void show_blowup_view(int view_idx, int sensor_idx) {
    if (view_idx < 0 || view_idx > 6) return;

    Serial.printf("[Blowup] Switching to view %d, sensor %d\n", view_idx, sensor_idx);

    for (int i = 0; i < 7; i++) {
        if (blowup_views[i]) {
            if (i == view_idx) {
                lv_obj_clear_flag(blowup_views[i], LV_OBJ_FLAG_HIDDEN);
                Serial.printf("[Blowup] Unhid view %d at addr %p\n", i, blowup_views[i]);
            } else {
                lv_obj_add_flag(blowup_views[i], LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            Serial.printf("[Blowup] ERROR: View %d is NULL!\n", i);
        }
    }

    current_blowup_view = view_idx;

    // Update border color
    if (blowup_border_obj) {
        if (view_idx == BLOWUP_QUOTE) {
            lv_obj_set_style_border_color(blowup_border_obj, t.border, 0);
            lv_obj_set_style_border_opa(blowup_border_obj, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_border_color(blowup_border_obj, t.accent, 0);
            lv_obj_set_style_border_opa(blowup_border_obj, 51, 0); // ~20%
        }
    } else {
        Serial.println("[Blowup] ERROR: blowup_border_obj is NULL!");
    }

    // If sensor view, update display
    if (view_idx == BLOWUP_SENSOR && sensor_idx >= 0) {
        selected_sensor_idx = sensor_idx;
        update_sensor_chart_display(sensor_idx);
    }
}

// ── Update Sensor Chart Display ──────────────────────────────────────────────
void update_sensor_chart_display(int idx) {
    if (idx < 0 || idx >= SENSOR_COUNT) return;
    if (!sensor_big_value) return;

    // Get sensor definition
    extern const struct SensorDef sensors[SENSOR_COUNT];
    extern float sensor_values[SENSOR_COUNT];
    extern bool sensor_connected[SENSOR_COUNT];
    extern lv_color_t get_status_color(float val, float goodMax, float warnMax);
    extern const char *get_status_text(float val, float goodMax, float warnMax);

    const SensorDef *def = &sensors[idx];
    float val = sensor_values[idx];
    bool conn = sensor_connected[idx];

    lv_color_t color = conn ? get_status_color(val, def->goodMax, def->warnMax) : t.text_dim;
    const char *status = conn ? get_status_text(val, def->goodMax, def->warnMax) : "---";

    // Update labels
    if (conn) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.*f", def->decimals, val);
        lv_label_set_text(sensor_big_value, buf);
    } else {
        lv_label_set_text(sensor_big_value, "--");
    }
    lv_obj_set_style_text_color(sensor_big_value, color, 0);

    lv_label_set_text(sensor_name_label, def->label);
    lv_label_set_text(sensor_addr_label, def->addr);

    lv_label_set_text(sensor_badge, status);
    lv_obj_set_style_text_color(sensor_badge, color, 0);
    lv_obj_set_style_bg_color(sensor_badge, color, 0);

    // Update chart series color
    lv_chart_set_series_color(sensor_chart, sensor_series, color);

    // Fill chart data from history
    lv_chart_set_ext_y_array(sensor_chart, sensor_series, sensor_history[idx]);

    // Calculate stats
    if (conn && sensor_history_filled[idx]) {
        lv_coord_t sum = 0, minV = 999999, maxV = -999999;
        for (int i = 0; i < SENSOR_HISTORY_SIZE; i++) {
            lv_coord_t v = sensor_history[idx][i];
            sum += v;
            if (v < minV) minV = v;
            if (v > maxV) maxV = v;
        }
        float avg = (float)sum / SENSOR_HISTORY_SIZE;

        char buf[32];
        snprintf(buf, sizeof(buf), "AVG %.*f", def->decimals, avg);
        lv_label_set_text(sensor_avg_label, buf);
        snprintf(buf, sizeof(buf), "MIN %.*f", def->decimals, minV);
        lv_label_set_text(sensor_min_label, buf);
        snprintf(buf, sizeof(buf), "MAX %.*f", def->decimals, maxV);
        lv_label_set_text(sensor_max_label, buf);
    } else {
        lv_label_set_text(sensor_avg_label, "AVG --");
        lv_label_set_text(sensor_min_label, "MIN --");
        lv_label_set_text(sensor_max_label, "MAX --");
    }

    lv_chart_refresh(sensor_chart);
}

// ── Record Sensor History ────────────────────────────────────────────────────
void record_sensor_history(int idx, float value) {
    if (idx < 0 || idx >= SENSOR_COUNT) return;
    sensor_history[idx][sensor_history_idx[idx]] = (lv_coord_t)value;
    sensor_history_idx[idx] = (sensor_history_idx[idx] + 1) % SENSOR_HISTORY_SIZE;
    if (sensor_history_idx[idx] == 0) sensor_history_filled[idx] = true;

    // If this sensor is currently displayed in chart, update it
    if (current_blowup_view == BLOWUP_SENSOR && selected_sensor_idx == idx) {
        update_sensor_chart_display(idx);
    }
}

// ── Main Create Function ─────────────────────────────────────────────────────
void create_blowup_panel(lv_obj_t *parent) {
    blowup_border_obj = parent;

    create_quote_view(parent);
    create_sensor_chart_view(parent);
    create_placeholder_view(parent, BLOWUP_FIRETV, "FIRE TV REMOTE");
    create_placeholder_view(parent, BLOWUP_WEATHER, "WEATHER CHART");
    create_placeholder_view(parent, BLOWUP_PORTS, "PORT STATUS");
    create_placeholder_view(parent, BLOWUP_SDCARD, "SD CARD");
    create_placeholder_view(parent, BLOWUP_HOURLY, "HOURLY FORECAST");

    show_blowup_view(BLOWUP_QUOTE);
}