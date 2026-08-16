#include "settings_screen.h"
#include "wifi_screen.h"
#include "ui.h"
#include "../ui/theme.h"
#include <Arduino.h>

extern lv_obj_t *main_screen;

// Global settings and offsets
AppSettings appSettings = { true, true, 3 };
float aht20_temp_offset = -1.4;
float aht20_hum_offset = 4.8;

static lv_obj_t *settings_screen = nullptr;
static lv_obj_t *dashboard_screen = nullptr;

static lv_obj_t * temp_offset_label = nullptr;
static lv_obj_t * hum_offset_label = nullptr;

static void back_btn_cb(lv_event_t * e) {
    (void)e;
    Serial.println("[UI] Settings back button pressed");
    lv_screen_load(main_screen);
}

static void wifi_btn_cb(lv_event_t *e) {
    hide_settings_screen();
    show_wifi_screen();
}

static void msgbox_timer_cb(lv_timer_t *timer) {
    lv_obj_t *msg = (lv_obj_t*)lv_timer_get_user_data(timer);
    if (msg) lv_obj_del(msg);
    lv_timer_del(timer);
}

static void clock_btn_cb(lv_event_t *e) {
    lv_obj_t *settings_scr = lv_scr_act();
    lv_obj_t *msg = lv_msgbox_create(settings_scr);
    lv_msgbox_add_title(msg, "Clock Config");
    lv_msgbox_add_text(msg, "Clock settings coming soon");
    lv_obj_center(msg);
    lv_obj_set_style_bg_color(msg, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(msg, LV_OPA_COVER, 0);
    
    lv_timer_create(msgbox_timer_cb, 2000, msg);
}

// ── Calibration Callbacks ────────────────────────────────────────────────────
static void temp_offset_slider_cb(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t*)lv_event_get_target(e); // Added cast
    int32_t val = lv_slider_get_value(slider);
    aht20_temp_offset = val / 10.0;
    lv_label_set_text_fmt(temp_offset_label, "%+.1f F", aht20_temp_offset);
    Serial.printf("[Settings] Temp offset: %.1f\n", aht20_temp_offset);
}

static void hum_offset_slider_cb(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t*)lv_event_get_target(e); // Added cast
    int32_t val = lv_slider_get_value(slider);
    aht20_hum_offset = val / 10.0;
    lv_label_set_text_fmt(hum_offset_label, "%+.1f %%", aht20_hum_offset);
    Serial.printf("[Settings] Hum offset: %.1f\n", aht20_hum_offset);
}

void show_settings_screen() {
    dashboard_screen = lv_scr_act();
    
    settings_screen = lv_obj_create(NULL);
    // Use theme colors instead of hardcoded
    lv_obj_set_style_bg_color(settings_screen, t.bg, 0);
    lv_obj_set_style_bg_opa(settings_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Top bar
    lv_obj_t *top_bar = lv_obj_create(settings_screen);
    lv_obj_set_size(top_bar, 800, 50);
    lv_obj_set_style_bg_color(top_bar, t.surface, 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_style_bg_color(back_btn, t.surface2, 0);
    lv_obj_set_style_shadow_width(back_btn, 0, 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(back_lbl, t.text, 0);
    lv_obj_center(back_lbl);
    
    // Title
    lv_obj_t *title = lv_label_create(top_bar);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, t.text, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
    // Main container (increased height to fit calibration)
    lv_obj_t *container = lv_obj_create(settings_screen);
    lv_obj_set_size(container, 400, 420);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 20, 0);
    lv_obj_set_style_pad_gap(container, 15, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(container, LV_ALIGN_CENTER, 0, 10);
    
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // WiFi button
    lv_obj_t *wifi_btn = lv_btn_create(container);
    lv_obj_set_size(wifi_btn, 360, 60);
    lv_obj_set_style_bg_color(wifi_btn, t.accent, 0);
    lv_obj_set_style_radius(wifi_btn, 10, 0);
    lv_obj_set_style_shadow_width(wifi_btn, 0, 0);
    lv_obj_add_event_cb(wifi_btn, wifi_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *wifi_lbl = lv_label_create(wifi_btn);
    lv_label_set_text(wifi_lbl, LV_SYMBOL_WIFI "  WiFi Login");
    lv_obj_set_style_text_color(wifi_lbl, t.bg, 0);
    lv_obj_set_style_text_font(wifi_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(wifi_lbl);
    
    // Clock button
    lv_obj_t *clock_btn = lv_btn_create(container);
    lv_obj_set_size(clock_btn, 360, 60);
    lv_obj_set_style_bg_color(clock_btn, lv_color_hex(0x9B59B6), 0);
    lv_obj_set_style_radius(clock_btn, 10, 0);
    lv_obj_set_style_shadow_width(clock_btn, 0, 0);
    lv_obj_add_event_cb(clock_btn, clock_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *clock_lbl = lv_label_create(clock_btn);
    lv_label_set_text(clock_lbl, LV_SYMBOL_BELL "  Clock Config");
    lv_obj_set_style_text_color(clock_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(clock_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(clock_lbl);

    // ── Sensor Calibration Section ────────────────────────────────────────────
    lv_obj_t * cal_title = lv_label_create(container);
    lv_label_set_text(cal_title, "Sensor Calibration");
    lv_obj_set_style_text_color(cal_title, t.text, 0);
    lv_obj_set_style_text_font(cal_title, &lv_font_montserrat_20, 0);

    // Temp Offset Row
    lv_obj_t * temp_row = lv_obj_create(container);
    lv_obj_set_size(temp_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(temp_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(temp_row, 0, 0);
    lv_obj_set_style_pad_all(temp_row, 0, 0);
    lv_obj_set_flex_flow(temp_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(temp_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * temp_lbl = lv_label_create(temp_row);
    lv_label_set_text(temp_lbl, "Temp Offset");
    lv_obj_set_style_text_font(temp_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(temp_lbl, t.text, 0);

    temp_offset_label = lv_label_create(temp_row);
    lv_label_set_text_fmt(temp_offset_label, "%+.1f F", aht20_temp_offset);
    lv_obj_set_style_text_font(temp_offset_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(temp_offset_label, t.accent, 0);

    lv_obj_t * temp_slider = lv_slider_create(container);
    lv_obj_set_width(temp_slider, lv_pct(100));
    lv_slider_set_range(temp_slider, -100, 100); // -10.0 to +10.0
    lv_slider_set_value(temp_slider, (int)(aht20_temp_offset * 10), LV_ANIM_OFF);
    lv_obj_add_event_cb(temp_slider, temp_offset_slider_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Humidity Offset Row
    lv_obj_t * hum_row = lv_obj_create(container);
    lv_obj_set_size(hum_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hum_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hum_row, 0, 0);
    lv_obj_set_style_pad_all(hum_row, 0, 0);
    lv_obj_set_flex_flow(hum_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hum_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * hum_lbl = lv_label_create(hum_row);
    lv_label_set_text(hum_lbl, "Humidity Offset");
    lv_obj_set_style_text_font(hum_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hum_lbl, t.text, 0);

    hum_offset_label = lv_label_create(hum_row);
    lv_label_set_text_fmt(hum_offset_label, "%+.1f %%", aht20_hum_offset);
    lv_obj_set_style_text_font(hum_offset_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hum_offset_label, t.accent, 0);

    lv_obj_t * hum_slider = lv_slider_create(container);
    lv_obj_set_width(hum_slider, lv_pct(100));
    lv_slider_set_range(hum_slider, -200, 200); // -20.0 to +20.0
    lv_slider_set_value(hum_slider, (int)(aht20_hum_offset * 10), LV_ANIM_OFF);
    lv_obj_add_event_cb(hum_slider, hum_offset_slider_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    
    lv_scr_load(settings_screen);
}

void hide_settings_screen() {
    if (settings_screen) {
        lv_scr_load(dashboard_screen);
        lv_obj_del(settings_screen);
        settings_screen = nullptr;
    }
}