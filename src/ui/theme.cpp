#include "theme.h"

AirWatchTheme t;

void init_theme(bool dark) {
    if (dark) {
        t.bg            = lv_color_hex(0x0b0f1a);
        t.surface       = lv_color_hex(0x111827);
        t.surface2      = lv_color_hex(0x1a2236);
        t.border        = lv_color_hex(0x1c202a); // Approximating white at 7% opa
        t.border_strong = lv_color_hex(0x2b2e38); // Approximating white at 13% opa
        t.text          = lv_color_hex(0xf1f5f9);
        t.text_sub      = lv_color_hex(0x9ca3af);
        t.text_muted    = lv_color_hex(0x6b7280);
        t.text_dim      = lv_color_hex(0x374151);
        t.accent        = lv_color_hex(0x22d3ee);
        t.accent_dim    = lv_color_hex(0x132b3b);
        t.accent_border = lv_color_hex(0x143d4f);
        t.chart_grid    = lv_color_hex(0x1a1f2a);
        t.good          = lv_color_hex(0x22c55e);
        t.warn          = lv_color_hex(0xf59e0b);
        t.danger        = lv_color_hex(0xef4444);
        t.orange        = lv_color_hex(0xf97316);
        t.blue          = lv_color_hex(0x60a5fa);
        t.amber         = lv_color_hex(0xf59e0b);
        t.slate         = lv_color_hex(0x94a3b8);
        t.purple        = lv_color_hex(0xa78bfa);
    } else {
        // Light mode mappings from skeleton
        t.bg            = lv_color_hex(0xe8edf4);
        t.surface       = lv_color_hex(0xf6f8fb);
        t.surface2      = lv_color_hex(0xdde3ec);
        t.border        = lv_color_hex(0xd0d6e0);
        t.border_strong = lv_color_hex(0xb8c0cc);
        t.text          = lv_color_hex(0x0f172a);
        t.text_sub      = lv_color_hex(0x475569);
        t.text_muted    = lv_color_hex(0x94a3b8);
        t.text_dim      = lv_color_hex(0xcbd5e1);
        t.accent        = lv_color_hex(0x0891b2);
        t.accent_dim    = lv_color_hex(0xe0f0f4);
        t.accent_border = lv_color_hex(0xa0d8e4);
        t.chart_grid    = lv_color_hex(0xdde3ec);
        t.good          = lv_color_hex(0x16a34a);
        t.warn          = lv_color_hex(0xd97706);
        t.danger        = lv_color_hex(0xdc2626);
        t.orange        = lv_color_hex(0xf97316);
        t.blue          = lv_color_hex(0x60a5fa);
        t.amber         = lv_color_hex(0xf59e0b);
        t.slate         = lv_color_hex(0x94a3b8);
        t.purple        = lv_color_hex(0xa78bfa);
    }
}