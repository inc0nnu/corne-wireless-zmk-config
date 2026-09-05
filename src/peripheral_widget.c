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
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>

#include "peripheral_widget.h"

// Static "always flowing" Japanese-text graphic, replacing the random
// balloon/mountain art. See src/jp_text_image.c.
LV_IMG_DECLARE(jp_text_img);

// Roughly the same physical band as the icon row on the central half,
// offset 160-68 = 92 - "roughly" because the Japanese-text art image below
// is 131 native pixels wide rather than 130, borrowing 1px from the icon
// row's own visible height (30 -> 29) to give the art's frame 1px more
// room between its top border and the kanji. See central_widget.c for the
// full explanation of this offset math.
#define ICON_OFFSET 92
#define ICON_CANVAS_IDX 0

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct peripheral_conn_state {
    bool connected;
};

static void draw_icon(lv_obj_t *widget, lv_color_t cbuf[], const struct peripheral_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, ICON_CANVAS_IDX);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    draw_icon_row(canvas, state->battery, state->charging,
                 state->connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);

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
    return (struct battery_status_state){
        .level = zmk_battery_state_of_charge(),
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

static struct peripheral_conn_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_conn_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void set_connection_status(struct zmk_widget_status *widget,
                                  struct peripheral_conn_state state) {
    widget->state.connected = state.connected;

    draw_icon(widget->obj, widget->cbuf_icon, &widget->state);
}

static void output_status_update_cb(struct peripheral_conn_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_connection_status(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_conn_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    lv_obj_t *icon = lv_canvas_create(widget->obj);
    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, ICON_OFFSET, 0);
    lv_canvas_set_buffer(icon, widget->cbuf_icon, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    // Static Japanese-text graphic filling the remaining native-x 0..131
    // (physical height 131 = 160 - the 29px icon row), drawn directly with
    // no runtime rotation, exactly like the stock balloon/mountain art.
    lv_obj_t *art = lv_img_create(widget->obj);
    lv_img_set_src(art, &jp_text_img);
    lv_obj_align(art, LV_ALIGN_TOP_LEFT, 0, 0);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_peripheral_status_init();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
