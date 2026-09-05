/*
 *
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/endpoints.h>

#define NICEVIEW_PROFILE_COUNT 5

#define CANVAS_SIZE 68

#define LVGL_BACKGROUND                                                                          \
    IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_black() : lv_color_white()
#define LVGL_FOREGROUND                                                                          \
    IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_white() : lv_color_black()

struct battery_status_state {
    uint8_t level;
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_present;
#endif
};

void rotate_canvas(lv_obj_t *canvas, lv_color_t cbuf[]);

void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                     lv_text_align_t align);
void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color);
void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width);
void init_arc_dsc(lv_draw_arc_dsc_t *arc_dsc, lv_color_t color, uint8_t width);

/**
 * Draws the "icon row" shared by both halves: the battery percentage,
 * left-aligned (no battery outline icon), plus a single connection/output
 * symbol right-aligned. While charging, the nRF52 charge-rail ADC can't
 * read a true battery voltage, so zmk_battery_state_of_charge() itself is
 * typically stuck near 100% in that state - rather than show that
 * misleading number, a charging glyph is shown instead.
 *
 * This is factored out into one shared function specifically so both halves
 * render it at an identical size/position.
 *
 * @param canvas The (unrotated) canvas to draw into.
 * @param battery_pct Battery charge, 0-100. Ignored while charging.
 * @param charging Whether the keyboard half is currently on USB power.
 * @param symbol One of the LV_SYMBOL_* strings (e.g. LV_SYMBOL_WIFI).
 */
void draw_icon_row(lv_obj_t *canvas, uint8_t battery_pct, bool charging, const char *symbol);

/**
 * Draws a minimal Bluetooth "rune" glyph: a vertical spine the full height
 * of the glyph, plus two diagonals that both meet the spine's endpoints and
 * a single point on the right at the same height as the spine's center
 * (cx + rw, cy) - this is what forms the two triangular "flags" that make
 * it read as the actual Bluetooth logo shape, rather than two unrelated
 * diagonal strokes.
 *
 * @param rw Half-width of the glyph.
 * @param rh Half-height of the glyph.
 */
void draw_bt_logo(lv_obj_t *canvas, lv_coord_t cx, lv_coord_t cy, lv_coord_t rw, lv_coord_t rh);
