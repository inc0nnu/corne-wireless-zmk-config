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

    // No battery outline icon anymore - just the percentage, left-aligned,
    // starting near the left edge of the row.
    //
    // While charging, the nRF52's charge-rail ADC can't read a true battery
    // voltage, so the percentage itself is typically stuck near 100% in
    // that state (a hardware limitation, not something this screen can see
    // through) - show a charging glyph instead of a misleading "100%".
    if (charging) {
        lv_canvas_draw_text(canvas, 2, 2, 45, &label_dsc_pct, LV_SYMBOL_CHARGE);
    } else {
        char pct_text[6] = {};
        snprintf(pct_text, sizeof(pct_text), "%d%%", battery_pct);
        lv_canvas_draw_text(canvas, 2, 2, 45, &label_dsc_pct, pct_text);
    }

    // Connection / output symbol, right-aligned.
    lv_canvas_draw_text(canvas, 0, 4, CANVAS_SIZE, &label_dsc_symbol, symbol);
}

void draw_bt_logo(lv_obj_t *canvas, lv_coord_t cx, lv_coord_t cy, lv_coord_t rw, lv_coord_t rh) {
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 2);

    // Vertical spine, top to bottom.
    lv_point_t spine[2] = {{cx, cy - rh}, {cx, cy + rh}};
    lv_canvas_draw_line(canvas, spine, 2, &line_dsc);

    // Upper "flag": top of the spine down to the single right-hand point.
    // Both diagonals meeting at the SAME point (cx + rw, cy) is what makes
    // this read as the actual Bluetooth logo shape (two triangles sharing
    // a vertex on the spine's right side) instead of two crossed strokes.
    lv_point_t diag_top[2] = {{cx, cy - rh}, {cx + rw, cy}};
    lv_canvas_draw_line(canvas, diag_top, 2, &line_dsc);

    // Lower "flag": the same right-hand point down to the bottom of the spine.
    lv_point_t diag_bottom[2] = {{cx + rw, cy}, {cx, cy + rh}};
    lv_canvas_draw_line(canvas, diag_bottom, 2, &line_dsc);
}
