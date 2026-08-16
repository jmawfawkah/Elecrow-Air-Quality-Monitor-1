#include "ui.h"
#include "ui_helpers.h"
#include "ui_header.h"
#include "ui_body.h"
#include "ui_aqi.h"
#include "ui_system.h"
#include "ui_sensors.h"
#include "theme.h"
#include <Arduino.h>

lv_obj_t *main_screen = nullptr;
lv_obj_t *wifi_screen = nullptr;

static void draw_dot_grid_cb(lv_event_t * e) {
    lv_obj_t * obj = (lv_obj_t*)lv_event_get_current_target(e);
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_area_t obj_area;
    lv_obj_get_coords(obj, &obj_area);

    lv_draw_rect_dsc_t dot_dsc;
    lv_draw_rect_dsc_init(&dot_dsc);
    dot_dsc.bg_color = t.accent;
    dot_dsc.bg_opa = 6;
    dot_dsc.radius = LV_RADIUS_CIRCLE;

    for (int x = obj_area.x1; x < obj_area.x2; x += 40) {
        for (int y = obj_area.y1; y < obj_area.y2; y += 40) {
            lv_area_t dot = { x, y, x + 1, y + 1 };
            lv_draw_rect(layer, &dot_dsc, &dot);
        }
    }
}

void ui_init() {
    Serial.println("[UI] Initializing theme...");
    init_theme(true);
    
    Serial.println("[UI] Starting UI init...");
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, t.bg, 0);
    lv_obj_set_style_bg_opa(main_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(main_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    Serial.println("[UI] Creating header...");
    create_header(main_screen);
    
    Serial.println("[UI] Creating body...");
    create_body(main_screen);
    
    Serial.println("[UI] Loading screen...");
    lv_scr_load(main_screen);
    lv_obj_invalidate(main_screen);
    
    Serial.println("[UI] UI init complete!");
}

// ── Public API implementations (route to modules) ─────────────────────────────
void ui_set_wifi(bool connected, const char *ssid) {
    if (ssid_label) lv_label_set_text(ssid_label, ssid ? ssid : "Disconnected");
}

void ui_set_time(const char *time_str) {
    if (time_label) lv_label_set_text(time_label, time_str);
}

void ui_set_ram_stats(uint32_t eleHeap, uint32_t elePsram, uint32_t stampHeap) {
    ui_set_ram_stats_impl(eleHeap, elePsram, stampHeap);
}

void update_display_cpu_stats() {
    update_display_cpu_stats_impl();
}

void ui_set_status(const char *msg) { Serial.printf("[UI Status] %s\n", msg); }
void ui_set_outdoor_aqi(uint16_t aqi) { (void)aqi; }
void ui_set_eco2(uint16_t ppm) { ui_set_eco2_impl(ppm); }
void ui_set_tvoc(uint16_t ppb) { ui_set_tvoc_impl(ppb); }
void ui_set_voc_index(uint16_t index) { ui_set_voc_index_impl(index); }

void ui_set_aqi(uint8_t score, const char *label, lv_color_t color) { 
    ui_set_aqi_impl(score, label, color); 
}

void ui_set_voc(uint16_t val) { ui_set_voc_impl(val); }
void ui_set_humidity(float val) { ui_set_humidity_impl(val); }
void ui_set_temp(float val) { ui_set_temp_impl(val); }
void ui_set_co(float val) { ui_set_co_impl(val); }
void ui_set_no2(float val) { ui_set_no2_impl(val); }
void ui_set_eth(float val) { ui_set_eth_impl(val); }
void ui_set_h2(float val) { ui_set_h2_impl(val); }
void ui_set_nh3(float val) { ui_set_nh3_impl(val); }
void ui_set_ch4(float val) { ui_set_ch4_impl(val); }

void ui_show_blowup(int view, int sensorIndex) {
    show_blowup_view(view, sensorIndex);
}