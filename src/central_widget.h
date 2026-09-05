/*
 *
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include "util.h"

#define WPM_SPARKLINE_SAMPLES 10
// 30 seconds and 30 minutes of once-per-second samples.
#define WPM_MAX_WINDOW_30S_SAMPLES 30
#define WPM_MAX_WINDOW_30M_SAMPLES 1800

struct central_state {
    uint8_t battery;
    bool charging;
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    uint8_t layer_index;
    const char *layer_label;
    uint8_t wpm_sparkline[WPM_SPARKLINE_SAMPLES];
    uint8_t wpm_max_30s;
    uint8_t wpm_max_30m;
};

struct zmk_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf_icon[CANVAS_SIZE * CANVAS_SIZE];
    lv_color_t cbuf_graph[CANVAS_SIZE * CANVAS_SIZE];
    lv_color_t cbuf_bt[CANVAS_SIZE * CANVAS_SIZE];
    lv_color_t cbuf_layer[CANVAS_SIZE * CANVAS_SIZE];
    struct central_state state;
};

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget);
