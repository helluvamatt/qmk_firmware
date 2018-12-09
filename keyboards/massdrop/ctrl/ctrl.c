#include "ctrl.h"
#include "clks.h"
#include <string.h>

#ifdef RGB_MATRIX_ENABLE
#include "led_matrix.h"
#include "rgb_matrix.h"
#include "config_led.h"
#endif
#ifdef RAW_ENABLE
#include "usb/udi_device_epsize.h"
#include "raw_hid.h"

void raw_hid_receive(uint8_t *data, uint8_t length) {
	uint8_t send_buffer[RAW_EPSIZE];
	memset(send_buffer, 0, RAW_EPSIZE);
  // Header (4 bytes)
  send_buffer[0] = HID_PKT_RES_ACK;
  send_buffer[1] = data[1];
  send_buffer[2] = data[2];
  send_buffer[3] = data[3];
  switch (data[0]) {
    case HID_PKT_REQ_PING:
      raw_hid_send(send_buffer, RAW_EPSIZE);
      break;
#ifdef RGB_MATRIX_ENABLE
    case HID_PKT_REQ_CONFIG_LED_SET:
      {
        rgb_matrix_config.enable = data[4] != 0;
        rgb_matrix_config.mode = data[5];
        rgb_matrix_config.val = data[6];
        rgb_matrix_config.sat = data[7];
        rgb_matrix_config.hue = data[8] & 0xFF;
        rgb_matrix_config.speed = data[9];
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
    case HID_PKT_REQ_CONFIG_LED_GET:
      {
        send_buffer[4] = rgb_matrix_config.enable;
        send_buffer[5] = rgb_matrix_config.mode;
        send_buffer[6] = rgb_matrix_config.val;
        send_buffer[7] = rgb_matrix_config.sat;
        send_buffer[8] = rgb_matrix_config.hue & 0xFF;
        send_buffer[9] = rgb_matrix_config.speed;
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
    case HID_PKT_REQ_LED_SET_ALL:
      rgb_matrix_set_color_all(data[4], data[5], data[6]);
			raw_hid_send(send_buffer, RAW_EPSIZE);
      break;
    case HID_PKT_REQ_LED_SET:
      {
        // Loop over entire packet (64 byte packet = 4 byte header + 15 leds RGB+index)
        uint8_t offset = 4;
        while (offset < RAW_EPSIZE)
        {
          uint8_t i = data[offset++]; // 1-based index
          if (i == 0) break; // 0 indicates no more in this set
          if (i < ISSI3733_LED_COUNT) {
            uint8_t r = data[offset++];
            uint8_t g = data[offset++];
            uint8_t b = data[offset++];
            rgb_matrix_set_color(i - 1, r, g, b);
          }
        }
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
    case HID_PKT_REQ_DEVICE_PARAMS:
      {
        send_buffer[4] = ISSI3733_LED_COUNT;
        float l, t, r, b, w, h;
        led_matrix_get_disp_size(&l, &t, &r, &b, &w, &h);
        uint8_t* l_arr = (uint8_t *)(&l);
        uint8_t* t_arr = (uint8_t *)(&t);
        uint8_t* r_arr = (uint8_t *)(&r);
        uint8_t* b_arr = (uint8_t *)(&b);
        uint8_t* w_arr = (uint8_t *)(&w);
        uint8_t* h_arr = (uint8_t *)(&h);
        memcpy(&send_buffer[5], l_arr, 4);
        memcpy(&send_buffer[9], t_arr, 4);
        memcpy(&send_buffer[13], r_arr, 4);
        memcpy(&send_buffer[17], b_arr, 4);
        memcpy(&send_buffer[21], w_arr, 4);
        memcpy(&send_buffer[25], h_arr, 4);
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
    case HID_PKT_REQ_COMMIT_LED_CONFIG:
      // TODO Handle EEPROM
			raw_hid_send(send_buffer, RAW_EPSIZE);
      break;
    case HID_PKT_REQ_LED_GET:
      {
        float x, y;
        if (led_matrix_get(data[4], &send_buffer[4], &send_buffer[5], &send_buffer[6], &x, &y))
        {
          uint8_t* x_arr = (uint8_t *)(&x);
          uint8_t* y_arr = (uint8_t *)(&y);
          memcpy(&send_buffer[7], x_arr, 4);
          memcpy(&send_buffer[11], y_arr, 4);
        } else {
          send_buffer[0] = HID_PKT_RES_ERR;
        }
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
    case HID_PKT_REQ_LEDS_GET:
      {
        uint8_t page_size = ((RAW_EPSIZE - 4) / 3); // Without header, 3 bytes per item
        send_buffer[0] = (data[3] + 1) * page_size < ISSI3733_LED_COUNT ? HID_PKT_RES_MORE : HID_PKT_RES_ACK;
        uint8_t offset = 4;
        uint32_t start = data[3] * page_size;
        for (uint8_t i = 0; i < page_size; i++) {
          uint8_t* r = &send_buffer[offset++];
          uint8_t* g = &send_buffer[offset++];
          uint8_t* b = &send_buffer[offset++];
          led_matrix_get(start + i, r, g, b, NULL, NULL);
        }
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
#endif // RGB_MATRIX_ENABLE
    default:
			// Unknown message, send error
			send_buffer[0] = HID_PKT_RES_ERR;
			raw_hid_send(send_buffer, RAW_EPSIZE);
      break;
  }
}
#endif // RAW_ENABLE

#ifdef RGB_MATRIX_ENABLE

const rgb_led g_rgb_leds[DRIVER_LED_TOTAL] = {
  // KC_ESC, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, KC_PSCR, KC_SLCK, KC_PAUS
  { { 0|(0<<4) }, { 7, 5 }, 0 },
  { { 0|(1<<4) }, { 31, 5 }, 0 },
  { { 0|(2<<4) }, { 43, 5 }, 0 },
  { { 0|(3<<4) }, { 55, 5 }, 0 },
  { { 0|(4<<4) }, { 67, 5 }, 0 },
  { { 0|(5<<4) }, { 85, 5 }, 0 },
  { { 0|(6<<4) }, { 97, 5 }, 0 },
  { { 0|(7<<4) }, { 109, 5 }, 0 },
  { { 6|(0<<4) }, { 121, 5 }, 0 },
  { { 6|(1<<4) }, { 139, 5 }, 0 },
  { { 6|(2<<4) }, { 151, 5 }, 0 },
  { { 6|(3<<4) }, { 163, 5 }, 0 },
  { { 6|(4<<4) }, { 175, 5 }, 0 },
  { { 6|(5<<4) }, { 193, 5 }, 0 },
  { { 6|(6<<4) }, { 205, 5 }, 1 },
  { { 6|(7<<4) }, { 217, 5 }, 0 },
  // KC_GRV, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINS, KC_EQL, KC_BSPC, KC_INS, KC_HOME, KC_PGUP
  { { 1|(0<<4) }, { 7, 20 }, 0 },
  { { 1|(1<<4) }, { 19, 20 }, 0 },
  { { 1|(2<<4) }, { 31, 20 }, 0 },
  { { 1|(3<<4) }, { 43, 20 }, 0 },
  { { 1|(4<<4) }, { 55, 20 }, 0 },
  { { 1|(5<<4) }, { 67, 20 }, 0 },
  { { 1|(6<<4) }, { 79, 20 }, 0 },
  { { 1|(7<<4) }, { 91, 20 }, 0 },
  { { 7|(0<<4) }, { 103, 20 }, 0 },
  { { 7|(1<<4) }, { 115, 20 }, 0 },
  { { 7|(2<<4) }, { 127, 20 }, 0 },
  { { 7|(3<<4) }, { 139, 20 }, 0 },
  { { 7|(4<<4) }, { 151, 20 }, 0 },
  { { 7|(5<<4) }, { 169, 20 }, 0 },
  { { 7|(6<<4) }, { 193, 20 }, 0 },
  { { 7|(7<<4) }, { 205, 20 }, 0 },
  { { 9|(7<<4) }, { 217, 20 }, 0 },
  // KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRC, KC_RBRC, KC_BSLS, KC_DEL, KC_END, KC_PGDN
  { { 2|(0<<4) }, { 10, 30 }, 0 },
  { { 2|(1<<4) }, { 25, 30 }, 0 },
  { { 2|(2<<4) }, { 37, 30 }, 0 },
  { { 2|(3<<4) }, { 49, 30 }, 0 },
  { { 2|(4<<4) }, { 61, 30 }, 0 },
  { { 2|(5<<4) }, { 73, 30 }, 0 },
  { { 2|(6<<4) }, { 85, 30 }, 0 },
  { { 2|(7<<4) }, { 97, 30 }, 0 },
  { { 8|(0<<4) }, { 109, 30 }, 0 },
  { { 8|(1<<4) }, { 121, 30 }, 0 },
  { { 8|(2<<4) }, { 133, 30 }, 0 },
  { { 8|(3<<4) }, { 145, 30 }, 0 },
  { { 8|(4<<4) }, { 157, 30 }, 0 },
  { { 8|(5<<4) }, { 172, 30 }, 0 },
  { { 8|(6<<4) }, { 193, 30 }, 0 },
  { { 8|(7<<4) }, { 205, 30 }, 0 },
  { { 9|(6<<4) }, { 217, 30 }, 0 },
  // KC_CAPS, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT, KC_ENT
  { { 3|(0<<4) }, { 11, 39 }, 1 },
  { { 3|(1<<4) }, { 28, 39 }, 0 },
  { { 3|(2<<4) }, { 40, 39 }, 0 },
  { { 3|(3<<4) }, { 52, 39 }, 0 },
  { { 3|(4<<4) }, { 64, 39 }, 0 },
  { { 3|(5<<4) }, { 76, 39 }, 0 },
  { { 3|(6<<4) }, { 88, 39 }, 0 },
  { { 3|(7<<4) }, { 100, 39 }, 0 },
  { { 9|(0<<4) }, { 112, 39 }, 0 },
  { { 9|(1<<4) }, { 124, 39 }, 0 },
  { { 9|(2<<4) }, { 136, 39 }, 0 },
  { { 9|(3<<4) }, { 148, 39 }, 0 },
  { { 9|(4<<4) }, { 168, 39 }, 0 },
  // KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT, KC_UP
  { { 4|(0<<4) }, { 14, 49 }, 0 },
  { { 4|(1<<4) }, { 34, 49 }, 0 },
  { { 4|(2<<4) }, { 46, 49 }, 0 },
  { { 4|(3<<4) }, { 58, 49 }, 0 },
  { { 4|(4<<4) }, { 70, 49 }, 0 },
  { { 4|(5<<4) }, { 82, 49 }, 0 },
  { { 4|(6<<4) }, { 94, 49 }, 0 },
  { { 4|(7<<4) }, { 106, 49 }, 0 },
  { { 10|(0<<4) }, { 118, 49 }, 0 },
  { { 10|(1<<4) }, { 130, 49 }, 0 },
  { { 10|(2<<4) }, { 142, 49 }, 0 },
  { { 10|(3<<4) }, { 165, 49 }, 0 },
  { { 9|(5<<4) }, { 205, 49 }, 0 },
  // KC_LCTL, KC_LGUI, KC_LALT, KC_SPC, KC_RALT, MO(1), KC_APP, KC_RCTL, KC_LEFT, KC_DOWN, KC_RGHT
  { { 5|(0<<4) }, { 8, 59 }, 0 },
  { { 5|(1<<4) }, { 23, 59 }, 0 },
  { { 5|(2<<4) }, { 38, 59 }, 0 },
  { { 5|(3<<4) }, { 83, 59 }, 0 },
  { { 5|(4<<4) }, { 129, 59 }, 0 },
  { { 5|(5<<4) }, { 144, 59 }, 0 },
  { { 5|(6<<4) }, { 159, 59 }, 0 },
  { { 5|(7<<4) }, { 174, 59 }, 0 },
  { { 10|(4<<4) }, { 193, 59 }, 0 },
  { { 10|(5<<4) }, { 205, 59 }, 0 },
  { { 10|(6<<4) }, { 217, 59 }, 0 },
  // Underglow / Border
  { { 0xFF }, { 222, 64 }, 0 },
  { { 0xFF }, { 204, 64 }, 0 },
  { { 0xFF }, { 186, 64 }, 0 },
  { { 0xFF }, { 167, 64 }, 0 },
  { { 0xFF }, { 149, 64 }, 0 },
  { { 0xFF }, { 130, 64 }, 0 },
  { { 0xFF }, { 112, 64 }, 0 },
  { { 0xFF }, { 94, 64 }, 0 },
  { { 0xFF }, { 75, 64 }, 0 },
  { { 0xFF }, { 57, 64 }, 0 },
  { { 0xFF }, { 38, 64 }, 0 },
  { { 0xFF }, { 20, 64 }, 0 },
  { { 0xFF }, { 0, 64 }, 0 },
  { { 0xFF }, { 0, 47 }, 0 },
  { { 0xFF }, { 0, 32 }, 0 },
  { { 0xFF }, { 0, 17 }, 0 },
  { { 0xFF }, { 0, 0 }, 0 },
  { { 0xFF }, { 20, 0 }, 0 },
  { { 0xFF }, { 38, 0 }, 0 },
  { { 0xFF }, { 57, 0 }, 0 },
  { { 0xFF }, { 75, 0 }, 0 },
  { { 0xFF }, { 94, 0 }, 0 },
  { { 0xFF }, { 112, 0 }, 0 },
  { { 0xFF }, { 130, 0 }, 0 },
  { { 0xFF }, { 149, 0 }, 0 },
  { { 0xFF }, { 167, 0 }, 0 },
  { { 0xFF }, { 186, 0 }, 0 },
  { { 0xFF }, { 204, 0 }, 0 },
  { { 0xFF }, { 222, 1 }, 0 },
  { { 0xFF }, { 224, 17 }, 0 },
  { { 0xFF }, { 224, 32 }, 0 },
  { { 0xFF }, { 224, 47 }, 0 },
};

#endif
