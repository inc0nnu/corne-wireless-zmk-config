# Copyright (c) 2026 The ZMK Contributors
#
# SPDX-License-Identifier: MIT
#
# Custom nice!view status screen, replacing the one nice_view itself
# would otherwise provide (see Kconfig: NICE_VIEW_WIDGET_STATUS=n in
# config/corne.conf disables that one so this one is used instead).

if(CONFIG_ZMK_DISPLAY)
  # Extra (ZMK_EXTRA_MODULES) modules don't inherit the app target's own
  # `include` search path the way shields under zmk/app/boards/shields do,
  # so ZMK's public headers (zmk/display.h, zmk/battery.h, ...) have to be
  # added explicitly here. ZEPHYR_ZMK_MODULE_DIR is set by Zephyr's module
  # system for the "zmk" module itself (the same pattern used for e.g.
  # ZEPHYR_ZMK_STUDIO_MESSAGES_MODULE_DIR in zmk/app/CMakeLists.txt).
  zephyr_library_include_directories(${ZEPHYR_ZMK_MODULE_DIR}/app/include)
  zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
  zephyr_library_sources(src/util.c)
  zephyr_library_sources(src/status_screen.c)

  if(NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zephyr_library_sources(src/central_widget.c)
  else()
    zephyr_library_sources(src/peripheral_widget.c)
    zephyr_library_sources(src/jp_text_image.c)
  endif()
endif()
