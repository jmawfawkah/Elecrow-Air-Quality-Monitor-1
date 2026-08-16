#include "ui_system.h"
#include "ui_helpers.h"
#include "../ui/theme.h"

lv_obj_t *sys_disp_ram = nullptr;
lv_obj_t *sys_disp_psram = nullptr;
lv_obj_t *sys_stamp_ram = nullptr;
lv_obj_t *sys_stamp_psram = nullptr;

static lv_obj_t *sys_disp_temp = nullptr;
static lv_obj_t *sys_disp_free = nullptr;
static lv_obj_t *sys_stamp_temp = nullptr;
static lv_obj_t *sys_stamp_free = nullptr;

static uint32_t last_heap_free = 0;
static uint32_t last_psram_free = 0;

static lv_obj_t *create_cpu_card(lv_obj_t *parent, const char *name,
                                  lv_obj_t **temp_lbl, lv_obj_t **ram_lbl, 
                                  lv_obj_t **free_lbl, lv_obj_t **psram_lbl) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_style_bg_color(card, t.surface2, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 7, 0);
    lv_obj_set_style_pad_all(card, 5, 0);
    lv_obj_set_style_pad_hor(card, 7, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(card, 2, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    create_label(card, name, &lv_font_montserrat_10, t.accent);

    const char *labels[] = {"TEMP", "RAM", "FREE", "PSRAM"};
    lv_obj_t **value_ptrs[] = {temp_lbl, ram_lbl, free_lbl, psram_lbl};
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = create_clean_container(card, lv_pct(100), LV_SIZE_CONTENT, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        
        create_label(row, labels[i], &lv_font_montserrat_10, t.text_muted);
        lv_obj_t *val = create_label(row, "--", &lv_font_montserrat_10, t.text_sub);
        if (value_ptrs[i]) *value_ptrs[i] = val;
    }
    return card;
}

void create_system_widget(lv_obj_t *parent) {
    lv_obj_t *widget = lv_obj_create(parent);
    lv_obj_set_width(widget, lv_pct(100));
    lv_obj_set_flex_grow(widget, 1);
    lv_obj_set_style_bg_color(widget, t.surface, 0);
    lv_obj_set_style_bg_opa(widget, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(widget, t.border, 0);
    lv_obj_set_style_border_width(widget, 1, 0);
    lv_obj_set_style_radius(widget, 10, 0);
    lv_obj_set_style_pad_all(widget, 7, 0);
    lv_obj_set_style_pad_hor(widget, 8, 0);
    lv_obj_set_flex_flow(widget, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(widget, 4, 0);
    lv_obj_clear_flag(widget, LV_OBJ_FLAG_SCROLLABLE);

    create_label(widget, "SYSTEM", &lv_font_montserrat_10, t.text_muted);

    create_cpu_card(widget, "DISPLAY CPU", 
                    &sys_disp_temp, &sys_disp_ram, &sys_disp_free, &sys_disp_psram);

    create_cpu_card(widget, "DSTAMPS3",
                    &sys_stamp_temp, &sys_stamp_ram, &sys_stamp_free, &sys_stamp_psram);

    // WiFi row at bottom
    lv_obj_t *wifi_row = create_clean_container(widget, lv_pct(100), LV_SIZE_CONTENT, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t *wifi_left = create_clean_container(wifi_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(wifi_left, 5, 0);
    create_wifi_bars(wifi_left, t.accent, 85);
    lv_obj_t *ssid2 = create_label(wifi_left, "HomeNet", &lv_font_montserrat_10, t.text_muted);
    lv_obj_set_style_max_width(ssid2, 74, 0);
    lv_label_set_long_mode(ssid2, LV_LABEL_LONG_DOT);
    
    create_label(wifi_row, "-52 dBm", &lv_font_montserrat_10, t.text_sub);
}

void update_display_cpu_stats_impl() {
    char buf[32];
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t psramFree = ESP.getFreePsram();
    
    if (freeHeap != last_heap_free) {
        last_heap_free = freeHeap;
        uint32_t heapTotal = 320 * 1024;
        uint32_t heapUsed = heapTotal - freeHeap;
        if (sys_disp_ram) {
            snprintf(buf, sizeof(buf), "%lu/%luK", (unsigned long)(heapUsed / 1024), (unsigned long)(heapTotal / 1024));
            lv_label_set_text(sys_disp_ram, buf);
        }
        if (sys_disp_free) {
            snprintf(buf, sizeof(buf), "%luK", (unsigned long)(freeHeap / 1024));
            lv_label_set_text(sys_disp_free, buf);
        }
    }
    
    if (psramFree != last_psram_free) {
        last_psram_free = psramFree;
        uint32_t psramSize = ESP.getPsramSize();
        uint32_t psramUsed = psramSize - psramFree;
        if (sys_disp_psram) {
            snprintf(buf, sizeof(buf), "%lu/%luK", (unsigned long)(psramUsed / 1024), (unsigned long)(psramSize / 1024));
            lv_label_set_text(sys_disp_psram, buf);
        }
    }
}

void ui_set_ram_stats_impl(uint32_t eleHeap, uint32_t elePsram, uint32_t stampHeap) {
    char buf[32];
    
    if (sys_stamp_ram) {
        uint32_t stampTotal = 512 * 1024;
        uint32_t stampUsed = stampTotal - stampHeap;
        snprintf(buf, sizeof(buf), "%lu/%luK", (unsigned long)(stampUsed / 1024), (unsigned long)(stampTotal / 1024));
        lv_label_set_text(sys_stamp_ram, buf);
    }
    if (sys_stamp_free) {
        snprintf(buf, sizeof(buf), "%luK", (unsigned long)(stampHeap / 1024));
        lv_label_set_text(sys_stamp_free, buf);
    }
    if (sys_stamp_psram) {
        lv_label_set_text(sys_stamp_psram, "N/A");
    }
    if (sys_stamp_temp) {
        lv_label_set_text(sys_stamp_temp, "N/A");
    }
}