#ifndef __MATTLAMP_LEDS_H__
#define __MATTLAMP_LEDS_H__

#include <stdlib.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#include "ws2812.h"
#include "config.h"
#include "timer.h"
#include "debug.h"

#define LEDS_DATA_LEN 31
#define EECONFIG_MATTLAMP_LEDS (uint8_t*)0xF

// Turns lights off, ignores all other inputs
#define LEDS_MODE_OFF 0
// Doesn't change led, use leds_set_all or leds_set_one to change colors from off
#define LEDS_MODE_LIVE 1
// Sets all leds to the first color in the configuration
#define LEDS_MODE_SOLID 2
// Cycle all leds through the configured colors, first 4 mode-specific bytes specify the delay in ms before changing the color
#define LEDS_MODE_CYCLE 3
// Cycle individual leds through the configured colors, first 4 mode-specific bytes specify the delay in ms before changing the color
#define LEDS_MODE_CHASE 4
// Strobe effect, flash all leds quickly, first two mode-specific bytes specify the on duration in ms, next two mode-specific bytes specify the off duration, only uses the first defined color
#define LEDS_MODE_STROBE_ALL 5
// Strobe chase effect, flash individual leds quickly in sequence, first two mode-specific bytes specify the on duration in ms, next two mode-specific bytes specify the off duration, only uses the first defined color
#define LEDS_MODE_STROBE_CHASE 6
// Fade all leds between configured colors, first 4 mode-specific bytes specify the fade duration in ms (eg. 2000 ms = fade in 2000 ms + fade out 2000 ms = 4000 ms total cycle for each color)
#define LEDS_MODE_FADE 7
// Cycle through all the above effects
#define LEDS_MODE_CYCLE_EFFECTS 8

// Last mode in the sequence
#define LEDS_MAX_MODES LEDS_MODE_CYCLE_EFFECTS

union leds_config_u {
	uint8_t raw[LEDS_DATA_LEN];
	struct leds_config_t {
		uint8_t mode : 4; // Current mode
		uint8_t color_len : 4; // Number of actual colors
		LED_TYPE colors[8]; // 8 colors allowed
		uint32_t timeout1;
		uint16_t timeout2;
	} parms;
} leds_config;

void mattlamp_leds_init(void);
void mattlamp_leds_mode_step(void);
void mattlamp_leds_mode_set(uint8_t mode);
void mattlamp_leds_commit(void);
void mattlamp_leds_frame(void);
void mattlamp_leds_config_updated(void);

void mattlamp_leds_set_all(uint8_t r, uint8_t g, uint8_t b);
void mattlamp_leds_set_one(uint8_t r, uint8_t g, uint8_t b, uint8_t i);

bool mattlamp_leds_get_status(uint8_t i, uint8_t* r, uint8_t* g, uint8_t* b);

#endif