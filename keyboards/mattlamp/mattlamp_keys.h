#ifndef __MATTLAMP_KEYS_H__
#define __MATTLAMP_KEYS_H__

#include <stdlib.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#include "config.h"
#include "mattlamp_leds.h"
#include "keycode.h"
#include "send_string_keycodes.h"
#include "quantum.h"

#define KEYS_DATA_LEN 3
#define EECONFIG_MATTLAMP_KEYS (uint8_t*)(0x30)

// No-op
#define KEYS_ACTION_NONE 0
// Advance LED animation
#define KEYS_ACTION_LED_STEP 1
// 1 key press and release with optional modifiers
#define KEYS_ACTION_KEY_COMBO 2
// Set LED mode to the mode specified by the first byte parameter
#define KEYS_ACTION_LED_SET 3
// Toggle LED mode between the two modes specified by the first and second byte parameters
#define KEYS_ACTION_LED_TOGGLE 4

union keys_config_u {
	uint8_t raw[KEYS_DATA_LEN];
	struct keys_config_t {
		uint8_t action;
		uint8_t param1;
		uint8_t param2;
	} parms;
} keys_config;

void mattlamp_keys_init(void);
void mattlamp_keys_pressed(void);
bool mattlamp_keys_config(uint8_t action, uint8_t param1, uint8_t param2);

#endif
