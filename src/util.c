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

void draw_icon_row(lv_obj_t *canvas, uint8_t battery_pct, const char *symbol) {
    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_label_dsc_t label_dsc_pct;
    init_label_dsc(&label_dsc_pct, LVGL_FOREGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_LEFT);
    lv_draw_label_dsc_t label_dsc_symbol;
    init_label_dsc(&label_dsc_symbol, LVGL_FOREGROUND, &lv_font_montserrat_18,
                  LV_TEXT_ALIGN_RIGHT);

    // Static, upright battery outline "logo" (not a dynamically filled bar).
    // Body: x=3..12 (10 wide), y=3..22 (20 tall). Nub: x=6..9 on top.
    lv_canvas_draw_rect(canvas, 6, 0, 4, 3, &rect_white_dsc);
    lv_canvas_draw_rect(canvas, 3, 3, 10, 20, &rect_white_dsc);
    lv_canvas_draw_rect(canvas, 5, 5, 6, 16, &rect_black_dsc);

    // Battery percentage, at least as large as the WPM numbers.
    char pct_text[6] = {};
    snprintf(pct_text, sizeof(pct_text), "%d%%", battery_pct);
    lv_canvas_draw_text(canvas, 17, 2, 45, &label_dsc_pct, pct_text);

    // Connection / output symbol, right-aligned.
    lv_canvas_draw_text(canvas, 0, 2, CANVAS_SIZE, &label_dsc_symbol, symbol);
}

void draw_bt_logo(lv_obj_t *canvas, lv_coord_t cx, lv_coord_t cy, lv_coord_t rw, lv_coord_t rh) {
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 2);

    // Vertical spine, top to bottom.
    lv_point_t spine[2] = {{cx, cy - rh}, {cx, cy + rh}};
    lv_canvas_draw_line(canvas, spine, 2, &line_dsc);

    // Diagonal: top point to lower-right point.
    lv_point_t diag_top[2] = {{cx, cy - rh}, {cx + rw, cy + rh / 2}};
    lv_canvas_draw_line(canvas, diag_top, 2, &line_dsc);

    // Diagonal: upper-right point to bottom point.
    lv_point_t diag_bottom[2] = {{cx + rw, cy - rh / 2}, {cx, cy + rh}};
    lv_canvas_draw_line(canvas, diag_bottom, 2, &line_dsc);
}
