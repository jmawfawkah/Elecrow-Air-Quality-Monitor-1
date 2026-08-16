#include "ui_helpers.h"

lv_obj_t *create_wifi_bars(lv_obj_t *parent, lv_color_t color, int strength) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 14, 12);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_gap(cont, 1, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *bar = lv_obj_create(cont);
        lv_obj_set_size(bar, 2, 3 + i * 3);
        lv_obj_set_style_radius(bar, 1, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_pad_all(bar, 0, 0);
        lv_obj_set_style_bg_color(bar, (strength >= (i + 1) * 25) ? color : t.text_dim, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    }
    return cont;
}

lv_obj_t *create_divider(lv_obj_t *parent) {
    lv_obj_t *div = lv_obj_create(parent);
    lv_obj_set_size(div, 1, 14);
    lv_obj_set_style_bg_color(div, t.border, 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_pad_all(div, 0, 0);
    return div;
}

lv_obj_t *create_label(lv_obj_t *parent, const char *txt, const lv_font_t *font, lv_color_t color) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

lv_obj_t *create_clean_container(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, lv_flex_flow_t flow) {
    lv_obj_t *cont = lv_obj_create(parent);
    if (w > 0) lv_obj_set_width(cont, w);
    if (h > 0) lv_obj_set_height(cont, h);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_gap(cont, 0, 0);
    lv_obj_set_flex_flow(cont, flow);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    return cont;
}

lv_obj_t *create_tab_btn(lv_obj_t *parent, const char *text, const lv_font_t *font) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_height(btn, 20);
    lv_obj_set_style_pad_hor(btn, 6, 0);
    lv_obj_set_style_pad_ver(btn, 2, 0);
    lv_obj_set_style_radius(btn, 5, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(btn, t.border, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, t.text_muted, 0);
    lv_obj_center(lbl);
    return btn;
}