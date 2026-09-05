/*
 *
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>
#include <zmk/display.h>

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include "central_widget.h"
#else
#include "peripheral_widget.h"
#endif

static struct zmk_widget_status widget;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    zmk_widget_status_init(&widget, screen);
    lv_obj_align(zmk_widget_status_obj(&widget), LV_ALIGN_TOP_LEFT, 0, 0);

    return screen;
}
