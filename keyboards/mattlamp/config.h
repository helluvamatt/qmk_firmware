/*
Copyright 2018 Cole Markham

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CONFIG_H
#define CONFIG_H

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

#endif
