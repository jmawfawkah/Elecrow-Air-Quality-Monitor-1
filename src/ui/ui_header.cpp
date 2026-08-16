#include "ui_header.h"
#include "ui_helpers.h"
#include "../ui/theme.h"
#include "../ui/settings_screen.h"
#include "ui_blowup.h"
#include <Arduino.h>

lv_obj_t *time_label = nullptr;
lv_obj_t *ssid_label = nullptr;

static void settings_btn_cb(lv_event_t *e) {
    (void)e;
    Serial.println("[UI] Settings button pressed");
    show_settings_screen();
}

static void ports_btn_cb(lv_event_t *e) {
    (void)e;
    Serial.println("[UI] Ports button pressed");
    show_blowup_view(BLOWUP_PORTS);
}

static void sd_btn_cb(lv_event_t *e) {
    (void)e;
    Serial.println("[UI] SD button pressed");
    show_blowup_view(BLOWUP_SDCARD);
}

static void firetv_btn_cb(lv_event_t *e) {
    (void)e;
    Serial.println("[UI] Fire TV button pressed");
    show_blowup_view(BLOWUP_FIRETV);
}

static void daynight_btn_cb(lv_event_t *e) {
    (void)e;
    Serial.println("[UI] Day/Night toggle pressed");
    appSettings.darkMode = !appSettings.darkMode;
    init_theme(appSettings.darkMode);
    lv_obj_invalidate(lv_scr_act());
}

void create_header(lv_obj_t *parent) {
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, 800, 28);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, t.surface, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(header, t.border, 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_all(header, 4, 0);
    lv_obj_set_style_pad_gap(header, 4, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *brand = create_label(header, "AIRWATCH PRO", &lv_font_montserrat_10, t.accent);
    lv_obj_set_style_text_letter_space(brand, 2, 0);

    create_divider(header);
    create_wifi_bars(header, t.accent, 85);

    ssid_label = create_label(header, "HomeNet_5G", &lv_font_montserrat_10, t.text_muted);
    lv_obj_set_style_max_width(ssid_label, 80, 0);
    lv_label_set_long_mode(ssid_label, LV_LABEL_LONG_DOT);

    lv_obj_t *spacer = lv_obj_create(header);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_height(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_style_pad_all(spacer, 0, 0);

    time_label = create_label(header, "--:--:--", &lv_font_montserrat_10, t.text_muted);

    create_divider(header);

    lv_obj_t *ports_btn = create_tab_btn(header, "PORTS", &lv_font_montserrat_10);
    lv_obj_add_event_cb(ports_btn, ports_btn_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *sd_btn = create_tab_btn(header, "SD", &lv_font_montserrat_10);
    lv_obj_add_event_cb(sd_btn, sd_btn_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *firetv_btn = create_tab_btn(header, "FIRE TV", &lv_font_montserrat_10);
    lv_obj_add_event_cb(firetv_btn, firetv_btn_cb, LV_EVENT_CLICKED, nullptr);

    create_divider(header);

    lv_obj_t *daynight_btn = create_tab_btn(header, LV_SYMBOL_IMAGE, &lv_font_montserrat_10);
    lv_obj_set_style_bg_color(daynight_btn, t.accent, 0);
    lv_obj_set_style_bg_opa(daynight_btn, LV_OPA_10, 0);
    lv_obj_set_style_border_color(daynight_btn, t.accent, 0);
    lv_obj_set_style_border_opa(daynight_btn, LV_OPA_20, 0);
    lv_obj_add_event_cb(daynight_btn, daynight_btn_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *settings_btn = lv_btn_create(header);
    lv_obj_set_size(settings_btn, 20, 20);
    lv_obj_set_style_radius(settings_btn, 5, 0);
    lv_obj_set_style_bg_opa(settings_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(settings_btn, t.border, 0);
    lv_obj_set_style_border_width(settings_btn, 1, 0);
    lv_obj_set_style_pad_all(settings_btn, 0, 0);
    lv_obj_set_style_shadow_width(settings_btn, 0, 0);
    lv_obj_t *gear = lv_label_create(settings_btn);
    lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(gear, t.text_muted, 0);
    lv_obj_set_style_text_font(gear, &lv_font_montserrat_10, 0);
    lv_obj_center(gear);
    lv_obj_add_event_cb(settings_btn, settings_btn_cb, LV_EVENT_CLICKED, nullptr);
}