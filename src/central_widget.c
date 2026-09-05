/*
 *
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include "central_widget.h"
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>

// Native-x offsets of the four stacked canvases. Each canvas is
// CANVAS_SIZE (68) native pixels wide/tall; after the 90-degree rotation
// applied in rotate_canvas(), native-x becomes the physical vertical
// position on the (portrait-mounted) display, and a canvas placed at
// offset `cum_prev - CANVAS_SIZE` occupies the physical band from
// `cum_prev - H` to `cum_prev`, where H is how many of its rows are
// actually used before the next canvas is drawn on top of it.
//
//   icon  (H=30): physical band 130-160, offset 160-68 = 92
//   graph (H=68): physical band  62-130, offset 130-68 = 62
//   bt    (H=36): physical band  26- 62, offset  62-68 = -6
//   layer (H=26): physical band   0- 26, offset  26-68 = -42
//
// graph's H is now the full 68 (its entire canvas), i.e. BT_OFFSET was
// pulled back far enough that bt's range no longer overlaps graph's range
// at all - the WPM numbers were getting their bottom rows overwritten by
// bt's background fill (real hardware, not just a hypothetical) once they
// moved down to y=47, so graph needed all the room it could get. bt's own
// H shrank from 44 to 36 to make room (its logo/digit content was moved up
// - "kleinere padding" - to comfortably fit the smaller band).
#define ICON_OFFSET 92
#define GRAPH_OFFSET 62
#define BT_OFFSET -6
#define LAYER_OFFSET -42

#define ICON_CANVAS_IDX 0
#define GRAPH_CANVAS_IDX 1
#define BT_CANVAS_IDX 2
#define LAYER_CANVAS_IDX 3

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};

struct layer_status_state {
    zmk_keymap_layer_index_t index;
    const char *label;
};

struct wpm_status_state {
    uint8_t wpm;
};

static const char *current_output_symbol(const struct central_state *state) {
    switch (state->selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        return LV_SYMBOL_USB;
    case ZMK_TRANSPORT_BLE:
        if (state->active_profile_bonded) {
            return state->active_profile_connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE;
        }
        return LV_SYMBOL_SETTINGS;
    default:
        return "";
    }
}

static void draw_icon(lv_obj_t *widget, lv_color_t cbuf[], const struct central_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, ICON_CANVAS_IDX);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    draw_icon_row(canvas, state->battery, state->charging, current_output_symbol(state));

    rotate_canvas(canvas, cbuf);
}

static void draw_graph(lv_obj_t *widget, lv_color_t cbuf[], const struct central_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, GRAPH_CANVAS_IDX);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);
    lv_draw_label_dsc_t label_dsc_left;
    init_label_dsc(&label_dsc_left, LVGL_FOREGROUND, &lv_font_montserrat_20, LV_TEXT_ALIGN_LEFT);
    lv_draw_label_dsc_t label_dsc_right;
    init_label_dsc(&label_dsc_right, LVGL_FOREGROUND, &lv_font_montserrat_20, LV_TEXT_ALIGN_RIGHT);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Sparkline box: inset 1px on every side so it doesn't touch the
    // screen edge, extended 5px further at the bottom on top of the
    // earlier 5px height increase. Note: the graph canvas's own available
    // height shrank by 4px, from 64 to 60, to give the icon row its extra
    // 4px above (see the offset comment block up top) - between that and
    // this second extension the numbers below are now packed in quite
    // tightly, worth double-checking on the real screen that they aren't
    // clipped by the Bluetooth section below.
    lv_canvas_draw_rect(canvas, 1, 1, 66, 46, &rect_white_dsc);
    lv_canvas_draw_rect(canvas, 2, 2, 64, 44, &rect_black_dsc);

    int max = 0;
    int min = 256;
    for (int i = 0; i < WPM_SPARKLINE_SAMPLES; i++) {
        if (state->wpm_sparkline[i] > max) {
            max = state->wpm_sparkline[i];
        }
        if (state->wpm_sparkline[i] < min) {
            min = state->wpm_sparkline[i];
        }
    }
    int range = max - min;
    if (range == 0) {
        range = 1;
    }

    lv_point_t points[WPM_SPARKLINE_SAMPLES];
    for (int i = 0; i < WPM_SPARKLINE_SAMPLES; i++) {
        points[i].x = 3 + i * 6;
        points[i].y = 36 - (state->wpm_sparkline[i] - min) * 31 / range;
    }
    lv_canvas_draw_line(canvas, points, WPM_SPARKLINE_SAMPLES, &line_dsc);

    // Two numbers below the graph, no captions: rolling max WPM over the
    // last 30 minutes (left) and over the last 30 seconds (right). Note
    // that max_30s can mathematically never exceed max_30m, since the 30s
    // window is always a subset of the samples in the 30m window.
    //
    // Draw width widened from 30 to 40: at font 20, a 3-digit value (e.g.
    // "117") is wider than the old 30px box, which was silently clipping
    // the last digit ("11" instead of "117").
    char max_30m_text[6] = {};
    snprintf(max_30m_text, sizeof(max_30m_text), "%d", state->wpm_max_30m);
    lv_canvas_draw_text(canvas, 2, 47, 40, &label_dsc_left, max_30m_text);

    char max_30s_text[6] = {};
    snprintf(max_30s_text, sizeof(max_30s_text), "%d", state->wpm_max_30s);
    lv_canvas_draw_text(canvas, 26, 47, 40, &label_dsc_right, max_30s_text);

    rotate_canvas(canvas, cbuf);
}

static void draw_bt(lv_obj_t *widget, lv_color_t cbuf[], const struct central_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, BT_CANVAS_IDX);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_20, LV_TEXT_ALIGN_LEFT);

    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // The real Bluetooth logo bitmap (traced from Ivo's reference image),
    // placed directly to the left of the profile digit and top-aligned with
    // it so the two read as a pair. Moved up (y 11 -> 8, "kleinere padding")
    // to comfortably fit bt's own band now that it shrank from H=44 to
    // H=36 (see the offset comment block up top), and both nudged 2px
    // right to sit better centered in the row.
    draw_bt_logo(canvas, 18, 8);

    char profile_text[3] = {};
    snprintf(profile_text, sizeof(profile_text), "%d", state->active_profile_index + 1);
    lv_canvas_draw_text(canvas, 32, 8, 30, &label_dsc, profile_text);

    rotate_canvas(canvas, cbuf);
}

static void draw_layer(lv_obj_t *widget, lv_color_t cbuf[], const struct central_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, LAYER_CANVAS_IDX);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);

    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    if (state->layer_label == NULL || strlen(state->layer_label) == 0) {
        char text[10] = {};
        sprintf(text, "LAYER %i", state->layer_index);
        lv_canvas_draw_text(canvas, 0, 3, 68, &label_dsc, text);
    } else {
        lv_canvas_draw_text(canvas, 0, 3, 68, &label_dsc, state->layer_label);
    }

    rotate_canvas(canvas, cbuf);
}

static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;

    draw_icon(widget->obj, widget->cbuf_icon, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);

    return (struct battery_status_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

static void set_output_status(struct zmk_widget_status *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;

    draw_icon(widget->obj, widget->cbuf_icon, &widget->state);
    draw_bt(widget->obj, widget->cbuf_bt, &widget->state);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;

    draw_layer(widget->obj, widget->cbuf_layer, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = index, .label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index))};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

// --- WPM sparkline (unchanged mechanism: shifts in on every wpm_state_changed
// event, same as stock nice!view) ---

static void set_wpm_status(struct zmk_widget_status *widget, struct wpm_status_state state) {
    for (int i = 0; i < WPM_SPARKLINE_SAMPLES - 1; i++) {
        widget->state.wpm_sparkline[i] = widget->state.wpm_sparkline[i + 1];
    }
    widget->state.wpm_sparkline[WPM_SPARKLINE_SAMPLES - 1] = state.wpm;

    draw_graph(widget->obj, widget->cbuf_graph, &widget->state);
}

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_status(widget, state); }
}

static struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    return (struct wpm_status_state){.wpm = zmk_wpm_get_state()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state, wpm_status_update_cb,
                            wpm_status_get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

// --- Rolling-max WPM over the last 30 seconds / 30 minutes ---
//
// zmk_wpm_state_changed only fires when the WPM value *changes*, which is
// too sparse a signal for a rolling time window: a plateaued WPM would
// never "age out" of the window. So instead we sample zmk_wpm_get_state()
// directly, once a second, on our own timer, independent of that event.

static uint8_t wpm_window_30s[WPM_MAX_WINDOW_30S_SAMPLES];
static uint8_t wpm_window_30m[WPM_MAX_WINDOW_30M_SAMPLES];
static size_t wpm_window_30s_idx;
static size_t wpm_window_30m_idx;

static uint8_t wpm_window_max(const uint8_t *buf, size_t len) {
    uint8_t max = 0;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] > max) {
            max = buf[i];
        }
    }
    return max;
}

static void set_wpm_max_status(struct zmk_widget_status *widget, uint8_t max_30s,
                               uint8_t max_30m) {
    widget->state.wpm_max_30s = max_30s;
    widget->state.wpm_max_30m = max_30m;

    draw_graph(widget->obj, widget->cbuf_graph, &widget->state);
}

static void wpm_max_sample_work_cb(struct k_work *work) {
    uint8_t sample = (uint8_t)zmk_wpm_get_state();

    wpm_window_30s[wpm_window_30s_idx] = sample;
    wpm_window_30s_idx = (wpm_window_30s_idx + 1) % WPM_MAX_WINDOW_30S_SAMPLES;

    wpm_window_30m[wpm_window_30m_idx] = sample;
    wpm_window_30m_idx = (wpm_window_30m_idx + 1) % WPM_MAX_WINDOW_30M_SAMPLES;

    uint8_t max_30s = wpm_window_max(wpm_window_30s, WPM_MAX_WINDOW_30S_SAMPLES);
    uint8_t max_30m = wpm_window_max(wpm_window_30m, WPM_MAX_WINDOW_30M_SAMPLES);

    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_wpm_max_status(widget, max_30s, max_30m);
    }
}

K_WORK_DEFINE(wpm_max_sample_work, wpm_max_sample_work_cb);

static void wpm_max_sample_timer_cb(struct k_timer *timer) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &wpm_max_sample_work);
    }
}

K_TIMER_DEFINE(wpm_max_sample_timer, wpm_max_sample_timer_cb, NULL);

static bool wpm_max_sample_timer_started = false;

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    lv_obj_t *icon = lv_canvas_create(widget->obj);
    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, ICON_OFFSET, 0);
    lv_canvas_set_buffer(icon, widget->cbuf_icon, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *graph = lv_canvas_create(widget->obj);
    lv_obj_align(graph, LV_ALIGN_TOP_LEFT, GRAPH_OFFSET, 0);
    lv_canvas_set_buffer(graph, widget->cbuf_graph, CANVAS_SIZE, CANVAS_SIZE,
                         LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *bt = lv_canvas_create(widget->obj);
    lv_obj_align(bt, LV_ALIGN_TOP_LEFT, BT_OFFSET, 0);
    lv_canvas_set_buffer(bt, widget->cbuf_bt, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *layer = lv_canvas_create(widget->obj);
    lv_obj_align(layer, LV_ALIGN_TOP_LEFT, LAYER_OFFSET, 0);
    lv_canvas_set_buffer(layer, widget->cbuf_layer, CANVAS_SIZE, CANVAS_SIZE,
                         LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_output_status_init();
    widget_layer_status_init();
    widget_wpm_status_init();

    if (!wpm_max_sample_timer_started) {
        wpm_max_sample_timer_started = true;
        k_timer_start(&wpm_max_sample_timer, K_SECONDS(1), K_SECONDS(1));
    }

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
