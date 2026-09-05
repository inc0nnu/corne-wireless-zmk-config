/*
 *
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>
#include "util.h"

void rotate_canvas(lv_obj_t *canvas, lv_color_t cbuf[]) {
    static lv_color_t cbuf_tmp[CANVAS_SIZE * CANVAS_SIZE];
    memcpy(cbuf_tmp, cbuf, sizeof(cbuf_tmp));
    lv_img_dsc_t img;
    img.data = (void *)cbuf_tmp;
    img.header.cf = LV_IMG_CF_TRUE_COLOR;
    img.header.w = CANVAS_SIZE;
    img.header.h = CANVAS_SIZE;

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
    lv_canvas_transform(canvas, &img, 900, LV_IMG_ZOOM_NONE, -1, 0, CANVAS_SIZE / 2,
                        CANVAS_SIZE / 2, true);
}

void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align) {
    lv_draw_label_dsc_init(label_dsc);
    label_dsc->color = color;
    label_dsc->font = font;
    label_dsc->align = align;
}

void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color) {
    lv_draw_rect_dsc_init(rect_dsc);
    rect_dsc->bg_color = bg_color;
}

void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width) {
    lv_draw_line_dsc_init(line_dsc);
    line_dsc->color = color;
    line_dsc->width = width;
}

void init_arc_dsc(lv_draw_arc_dsc_t *arc_dsc, lv_color_t color, uint8_t width) {
    lv_draw_arc_dsc_init(arc_dsc);
    arc_dsc->color = color;
    arc_dsc->width = width;
}

void draw_icon_row(lv_obj_t *canvas, uint8_t battery_pct, bool charging, const char *symbol) {
    lv_draw_label_dsc_t label_dsc_pct;
    init_label_dsc(&label_dsc_pct, LVGL_FOREGROUND, &lv_font_montserrat_20, LV_TEXT_ALIGN_LEFT);
    lv_draw_label_dsc_t label_dsc_symbol;
    init_label_dsc(&label_dsc_symbol, LVGL_FOREGROUND, &lv_font_montserrat_18,
                  LV_TEXT_ALIGN_RIGHT);

    // No battery outline icon anymore - just the percentage (no "%" sign),
    // left-aligned, starting near the left edge of the row.
    //
    // While charging, the nRF52's charge-rail ADC can't read a true battery
    // voltage, so the percentage itself is typically stuck near 100% in
    // that state (a hardware limitation, not something this screen can see
    // through) - show a charging glyph instead of a misleading "100%".
    //
    // The symbol glyph (montserrat_18) actually renders visually taller
    // than the percentage digits (montserrat_20), so the percentage is
    // nudged down a few px to align their vertical centers rather than
    // their top-left draw origins.
    if (charging) {
        lv_canvas_draw_text(canvas, 2, 5, 45, &label_dsc_pct, LV_SYMBOL_CHARGE);
    } else {
        char pct_text[6] = {};
        snprintf(pct_text, sizeof(pct_text), "%d", battery_pct);
        lv_canvas_draw_text(canvas, 2, 5, 45, &label_dsc_pct, pct_text);
    }

    // Connection / output symbol, right-aligned.
    lv_canvas_draw_text(canvas, 0, 1, CANVAS_SIZE, &label_dsc_symbol, symbol);
}

// The real Bluetooth logo, traced from the official glyph (supplied by Ivo)
// and downsampled to a fixed BT_LOGO_W x BT_LOGO_H 1bpp bitmap, MSB-first,
// each row padded to a byte boundary. 1 = draw in LVGL_FOREGROUND, 0 = leave
// untouched (background is expected to already be filled by the caller).
static const uint8_t bt_logo_bits[BT_LOGO_H][2] = {
    {0x06, 0x00}, {0x06, 0x00}, {0x07, 0x00}, {0xc7, 0x80}, {0xe6, 0xc0}, {0x76, 0xe0},
    {0x3f, 0xc0}, {0x1f, 0x80}, {0x0f, 0x00}, {0x0f, 0x00}, {0x1f, 0x80}, {0x1f, 0xc0},
    {0x36, 0xe0}, {0xe6, 0xe0}, {0xc7, 0x80}, {0x07, 0x80}, {0x07, 0x00}, {0x06, 0x00},
};

void draw_bt_logo(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y) {
    for (int row = 0; row < BT_LOGO_H; row++) {
        for (int col = 0; col < BT_LOGO_W; col++) {
            uint8_t byte = bt_logo_bits[row][col / 8];
            if (byte & (0x80 >> (col % 8))) {
                lv_canvas_set_px_color(canvas, x + col, y + row, LVGL_FOREGROUND);
            }
        }
    }
}
