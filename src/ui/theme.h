#ifndef THEME_H
#define THEME_H

#include <lvgl.h>

struct AirWatchTheme {
    lv_color_t bg, surface, surface2, border, border_strong;
    lv_color_t text, text_sub, text_muted, text_dim;
    lv_color_t accent, accent_dim, accent_border;
    lv_color_t chart_grid, good, warn, danger;
    lv_color_t orange, blue, amber, slate, purple;
};

extern AirWatchTheme t;

void init_theme(bool dark = true);

#endif