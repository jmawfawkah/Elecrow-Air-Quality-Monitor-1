#include <cmath>
#include <cstdio>
#include "ui_aqi.h"
#include "ui_helpers.h"
#include "../ui/theme.h"

lv_obj_t *aqi_score_label = nullptr;
lv_obj_t *aqi_status_label = nullptr;

static uint8_t current_aqi_score = 85;
static lv_obj_t *aqi_sun_obj = nullptr;

static lv_color_t get_aqi_color(uint8_t score) {
    if (score >= 80) return t.good;
    if (score >= 60) return lv_color_hex(0x84cc16);
    if (score >= 40) return t.warn;
    if (score >= 20) return t.orange;
    return t.danger;
}

static const char* get_aqi_label_text(uint8_t score) {
    if (score >= 80) return "EXCELLENT";
    if (score >= 60) return "GOOD";
    if (score >= 40) return "MODERATE";
    if (score >= 20) return "POOR";
    return "HAZARDOUS";
}

static float get_smile_level(uint8_t score) {
    float level = (float)(score - 30) / 70.0f;
    if (level > 1.0f) level = 1.0f;
    if (level < -0.5f) level = -0.5f;
    return level;
}

static void draw_sun_face_cb(lv_event_t *e) {
    lv_obj_t *obj = (lv_obj_t*)lv_event_get_current_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);
    
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    int cx = (a.x1 + a.x2) / 2;
    int cy = (a.y1 + a.y2) / 2;
    int size = a.x2 - a.x1 + 1;
    float scale = size / 80.0f;
    
    lv_color_t color = get_aqi_color(current_aqi_score);
    
    // Outer glow
    lv_draw_rect_dsc_t glow_dsc;
    lv_draw_rect_dsc_init(&glow_dsc);
    glow_dsc.bg_color = color;
    glow_dsc.bg_opa = 20;
    glow_dsc.radius = LV_RADIUS_CIRCLE;
    glow_dsc.border_width = 0;
    int glow_r = 34 * scale;
    lv_area_t glow_area = {cx - glow_r, cy - glow_r, cx + glow_r, cy + glow_r};
    lv_draw_rect(layer, &glow_dsc, &glow_area);
    
    // 8 rays
    lv_draw_line_dsc_t ray_dsc;
    lv_draw_line_dsc_init(&ray_dsc);
    ray_dsc.color = color;
    ray_dsc.width = 2.5 * scale;
    ray_dsc.opa = 191;
    ray_dsc.round_start = 1;
    ray_dsc.round_end = 1;
    
    int ray_inner = 27 * scale;
    int ray_outer = 36 * scale;
    for (int i = 0; i < 8; i++) {
        float angle = i * 45.0f * 3.14159f / 180.0f;
        ray_dsc.p1.x = (int)(cx + cosf(angle) * ray_inner);
        ray_dsc.p1.y = (int)(cy + sinf(angle) * ray_inner);
        ray_dsc.p2.x = (int)(cx + cosf(angle) * ray_outer);
        ray_dsc.p2.y = (int)(cy + sinf(angle) * ray_outer);
        lv_draw_line(layer, &ray_dsc);
    }
    
    // Face circle
    lv_draw_rect_dsc_t face_dsc;
    lv_draw_rect_dsc_init(&face_dsc);
    face_dsc.bg_color = color;
    face_dsc.bg_opa = 224;
    face_dsc.radius = LV_RADIUS_CIRCLE;
    face_dsc.border_width = 0;
    int face_r = 22 * scale;
    lv_area_t face_area = {cx - face_r, cy - face_r, cx + face_r, cy + face_r};
    lv_draw_rect(layer, &face_dsc, &face_area);
    
    // Eyes
    lv_draw_rect_dsc_t eye_dsc;
    lv_draw_rect_dsc_init(&eye_dsc);
    eye_dsc.bg_color = lv_color_hex(0x000000);
    eye_dsc.bg_opa = 140;
    eye_dsc.radius = LV_RADIUS_CIRCLE;
    eye_dsc.border_width = 0;
    int eye_r = 2.5 * scale;
    int eye_off_x = 7 * scale;
    int eye_off_y = 3 * scale;
    lv_area_t left_eye = {cx - eye_off_x - eye_r, cy - eye_off_y - eye_r, 
                          cx - eye_off_x + eye_r, cy - eye_off_y + eye_r};
    lv_area_t right_eye = {cx + eye_off_x - eye_r, cy - eye_off_y - eye_r,
                           cx + eye_off_x + eye_r, cy - eye_off_y + eye_r};
    lv_draw_rect(layer, &eye_dsc, &left_eye);
    lv_draw_rect(layer, &eye_dsc, &right_eye);
    
    // Mouth (bezier curve as line segments)
    float smileLevel = get_smile_level(current_aqi_score);
    int mouth_y = cy + (int)(7 * scale);
    int mouth_left_x = cx - (int)(9 * scale);
    int mouth_right_x = cx + (int)(9 * scale);
    int mouth_ctrl_y = mouth_y + (int)(smileLevel * 8 * scale);
    
    lv_draw_line_dsc_t mouth_dsc;
    lv_draw_line_dsc_init(&mouth_dsc);
    mouth_dsc.color = lv_color_hex(0x000000);
    mouth_dsc.opa = 140;
    mouth_dsc.width = 2 * scale;
    mouth_dsc.round_start = 1;
    mouth_dsc.round_end = 1;
    
    mouth_dsc.p1.x = mouth_left_x;
    mouth_dsc.p1.y = mouth_y;
    
    for (int i = 1; i <= 12; i++) {
        float t = i / 12.0f;
        float mt = 1.0f - t;
        mouth_dsc.p2.x = (int)(mt * mt * mouth_left_x + 2 * mt * t * cx + t * t * mouth_right_x);
        mouth_dsc.p2.y = (int)(mt * mt * mouth_y + 2 * mt * t * mouth_ctrl_y + t * t * mouth_y);
        lv_draw_line(layer, &mouth_dsc);
        
        mouth_dsc.p1.x = mouth_dsc.p2.x;
        mouth_dsc.p1.y = mouth_dsc.p2.y;
    }
}

void create_aqi_widget(lv_obj_t *parent) {
    lv_obj_t *widget = lv_obj_create(parent);
    lv_obj_set_width(widget, lv_pct(100));
    lv_obj_set_height(widget, 135);
    lv_obj_set_style_bg_color(widget, t.surface, 0);
    lv_obj_set_style_bg_opa(widget, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(widget, t.border, 0);
    lv_obj_set_style_border_width(widget, 1, 0);
    lv_obj_set_style_radius(widget, 10, 0);
    lv_obj_set_style_pad_all(widget, 8, 0);
    lv_obj_set_flex_flow(widget, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(widget, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(widget, 2, 0);
    lv_obj_clear_flag(widget, LV_OBJ_FLAG_SCROLLABLE);

    create_label(widget, "INDOOR AQI", &lv_font_montserrat_10, t.text_muted);

    aqi_sun_obj = lv_obj_create(widget);
    lv_obj_set_size(aqi_sun_obj, 62, 62);
    lv_obj_set_style_bg_opa(aqi_sun_obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(aqi_sun_obj, 0, 0);
    lv_obj_set_style_pad_all(aqi_sun_obj, 0, 0);
    lv_obj_clear_flag(aqi_sun_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(aqi_sun_obj, draw_sun_face_cb, LV_EVENT_DRAW_MAIN, NULL);

    aqi_score_label = create_label(widget, "85", &lv_font_montserrat_20, t.good);

    aqi_status_label = lv_label_create(widget);
    lv_label_set_text(aqi_status_label, "EXCELLENT");
    lv_obj_set_style_text_font(aqi_status_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(aqi_status_label, t.good, 0);
    lv_obj_set_style_bg_color(aqi_status_label, t.good, 0);
    lv_obj_set_style_bg_opa(aqi_status_label, 46, 0);
    lv_obj_set_style_pad_hor(aqi_status_label, 8, 0);
    lv_obj_set_style_pad_ver(aqi_status_label, 2, 0);
    lv_obj_set_style_radius(aqi_status_label, 4, 0);
}

void ui_set_aqi_impl(uint8_t score, const char *label, lv_color_t color) {
    if (!aqi_score_label) return;
    
    current_aqi_score = score; // Update score for sun face drawing
    
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", score);
    lv_label_set_text(aqi_score_label, buf);
    lv_obj_set_style_text_color(aqi_score_label, color, 0);
    
    lv_label_set_text(aqi_status_label, label);
    lv_obj_set_style_text_color(aqi_status_label, color, 0);
    lv_obj_set_style_bg_color(aqi_status_label, color, 0);
    
    // Force sun face to redraw with new color/smile
    if (aqi_sun_obj) {
        lv_obj_invalidate(aqi_sun_obj);
    }
}