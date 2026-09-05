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

// Hardcoded to white-on-black rather than branching on
// CONFIG_NICE_VIEW_WIDGET_INVERTED: that Kconfig symbol isn't reliably
// taking effect for this nice_view_adapter+nice_view shield combination
// (screens were still rendering black-on-white with it set to y in
// corne.conf), so this no longer depends on it at all.
#define LVGL_BACKGROUND lv_color_black()
#define LVGL_FOREGROUND lv_color_white()

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

#define BT_LOGO_W 11
#define BT_LOGO_H 18

/**
 * Draws the real Bluetooth logo shape (traced from the official glyph and
 * baked into a small fixed-size 1bpp bitmap - see bt_logo_bits in util.c),
 * top-left corner at (x, y). Every set bit is painted in LVGL_FOREGROUND;
 * unset bits are left untouched (the caller is expected to have already
 * filled the background).
 */
void draw_bt_logo(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y);
