/*
Copyright 2018 Matt Schneeberger

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

// We are tightly bound to this keyboard model
#include "mattlamp.h"
#include "mattlamp_leds.h"
#include "mattlamp_keys.h"
#include "mattlamp_switches.h"
#include "raw_hid.h"
#include "usb_descriptor.h"

enum hid_pkt_req {
	HID_PKT_REQ_PING,
	HID_PKT_REQ_CONFIG_KEY_SET,
	HID_PKT_REQ_CONFIG_KEY_GET,
	HID_PKT_REQ_CONFIG_LED_SET,
	HID_PKT_REQ_CONFIG_LED_GET,
	HID_PKT_REQ_LED_SET_ALL,
	HID_PKT_REQ_LED_SET_ONE,
	HID_PKT_REQ_DEVICE_PARAMS,
	HID_PKT_REQ_LED_STATUS,
	HID_PKT_REQ_COMMIT_LED_CONFIG,
	HID_PKT_REQ_SWITCH_STATE,
};

enum hid_pkt_res {
	HID_PKT_RES_ACK,
	HID_PKT_RES_ERR,
	HID_PKT_RES_MORE,
};

enum mattlamp_keycodes {
	KC_ONE = SAFE_RANGE,
	KC_SW1,
	KC_SW2,
	KC_SW3
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
	LAYOUT( KC_ONE ),
};

static bool initialized = false;
static bool switch_states[SWITCHES_COUNT];

// Low-level matrix init function
void matrix_init_user(void) {
	debug_enable = true;
	if (!initialized) {
		print("Initializing in matrix_init_user...\n");
		mattlamp_leds_init();
		mattlamp_keys_init();
		mattlamp_switches_init();
		initialized = true;
	}
}

// Low-level matrix loop function, called on every loop event
void matrix_scan_user(void) {
	// Advance our LED animations
	mattlamp_leds_frame();

	// Process switches
	mattlamp_switches_read();
	for (uint8_t i = 0; i < SWITCHES_COUNT; i++) {
		bool sw = mattlamp_switches_get(i);
		if (sw != switch_states[i]) {
			switch_states[i] = sw;
			// TODO Decide what to do when the switch state changes
			if (sw) {
				xprintf("SW%u turned on\n", i+1);
			} else {
				xprintf("SW%u turned off\n", i+1);
			}
		}
	}

}

// Handle the key events
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
	if (keycode == KC_ONE && record->event.pressed) { // We only have one
		mattlamp_keys_pressed();
		return false;
	}
	return true;
}

// Handle raw HID commands
void raw_hid_receive(uint8_t *data, uint8_t length) {
	uint8_t send_buffer[RAW_EPSIZE];
	memset(send_buffer, 0, RAW_EPSIZE);
	switch (data[0]) {
		case HID_PKT_REQ_PING:
			send_buffer[0] = HID_PKT_RES_ACK;
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		case HID_PKT_REQ_CONFIG_KEY_GET:
			send_buffer[0] = HID_PKT_RES_ACK;
			send_buffer[1] = keys_config.parms.action;
			send_buffer[2] = keys_config.parms.param1;
			send_buffer[3] = keys_config.parms.param2;
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		case HID_PKT_REQ_CONFIG_KEY_SET:
			send_buffer[0] = mattlamp_keys_config(data[1], data[2], data[3]) ? HID_PKT_RES_ACK : HID_PKT_RES_ERR;
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		case HID_PKT_REQ_CONFIG_LED_GET:
			send_buffer[0] = HID_PKT_RES_ACK;
			memcpy(send_buffer+1, leds_config.raw, LEDS_DATA_LEN);
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		case HID_PKT_REQ_CONFIG_LED_SET:
			memcpy(leds_config.raw, data+1, LEDS_DATA_LEN);
			mattlamp_leds_config_updated();
			send_buffer[0] = HID_PKT_RES_ACK;
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		case HID_PKT_REQ_LED_SET_ALL:
			mattlamp_leds_set_all(data[1], data[2], data[3]);
			send_buffer[0] = HID_PKT_RES_ACK;
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		case HID_PKT_REQ_LED_SET_ONE:
			mattlamp_leds_set_one(data[1], data[2], data[3], data[4]);
			send_buffer[0] = HID_PKT_RES_ACK;
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		case HID_PKT_REQ_DEVICE_PARAMS:
			send_buffer[0] = HID_PKT_RES_ACK;
			send_buffer[1] = RGBLED_NUM;
			send_buffer[2] = SWITCHES_COUNT;
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		case HID_PKT_REQ_LED_STATUS:
			{
				uint8_t r, g, b;
				uint8_t start = data[1];
				uint8_t i;
				send_buffer[0] = (start + 10) < RGBLED_NUM ? HID_PKT_RES_MORE : HID_PKT_RES_ACK;
				for (i = 0; i < 10; i++) { // 10 = max that can fit in page (limited by packet size of 32)
					if (mattlamp_leds_get_status(start + i, &r, &g, &b)) {
						send_buffer[i * 3 + 2] = r;
						send_buffer[i * 3 + 3] = g;
						send_buffer[i * 3 + 4] = b;
					} else break;
				}
				send_buffer[1] = i; // How many were actually returned
				raw_hid_send(send_buffer, RAW_EPSIZE);
			}
			break;
		case HID_PKT_REQ_COMMIT_LED_CONFIG:
			mattlamp_leds_commit();
			send_buffer[0] = HID_PKT_RES_ACK;
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		case HID_PKT_REQ_SWITCH_STATE:
			send_buffer[0] = HID_PKT_RES_ACK;
			send_buffer[1] = SWITCHES_COUNT;
			for (uint8_t i = 0; i < SWITCHES_COUNT; i++) {
				send_buffer[i / 8 + 2] |= (switch_states[i] & 0x1u) << (i % 8);
			}
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
		default:
			// Unknown message, send error
			send_buffer[0] = HID_PKT_RES_ERR;
			raw_hid_send(send_buffer, RAW_EPSIZE);
			break;
	}
}
