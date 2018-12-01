#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define VENDOR_ID       0xFEED
#define PRODUCT_ID      0xDAD1
#define DEVICE_VER      0x0001
#define MANUFACTURER    SCHNEENET
#define PRODUCT         Matt Lamp
#define DESCRIPTION     Custom table lamp

/* key matrix size */
#define MATRIX_ROWS 1
#define MATRIX_COLS 1

/* key matrix pins */
#define MATRIX_ROW_PINS { B0 }
#define MATRIX_COL_PINS { B4 }
#define UNUSED_PINS

/* COL2ROW or ROW2COL */
#define DIODE_DIRECTION ROW2COL

/* Set 0 if debouncing isn't needed */
#define DEBOUNCING_DELAY 50

/* prevent stuck modifiers */
#define PREVENT_STUCK_MODIFIERS

/* disable unused features to save space */
#define NO_ACTION_LAYER
#define NO_ACTION_TAPPING
#define NO_ACTION_ONESHOT
#define NO_ACTION_MACRO

/* We call ws2812 directly, so we need these defined */
#define RGB_DI_PIN D3
#define RGBLED_NUM 5

/* Enable raw HID communication endpoint */
#define RAW_ENABLE

/* Switches config */
#define SWITCHES_COUNT 3
#define SWITCHES_PINS { \
    { B5, B6 }, \
    { C4, C5 }, \
    { D4, D5 }  \
}
