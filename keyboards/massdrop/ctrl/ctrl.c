#include "ctrl.h"
#include "clks.h"
#include <string.h>

#include "usb/udi_device_epsize.h"
#include "led_matrix.h"
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
    case HID_PKT_REQ_CONFIG_LED_SET:
      {
        led_enabled = data[4] != 0;
        if (data[5] <= LED_MODE_MAX_INDEX) led_lighting_mode = data[5];
        if (data[6] <= LED_GCR_MAX) gcr_desired = data[6];
        if (data[7] < led_setups_count + 1) led_animation_id = data[7];
        led_animation_breathing = data[8] != 0;
        led_animation_direction = data[9] != 0;
        uint8_t new_speed_arr[4];
        memcpy(new_speed_arr, &data[10], sizeof(float)); // WARNING: Assumes MCU is little ending/network byte order!
        float new_speed = *(float *)(new_speed_arr);
        if (new_speed < 0) new_speed = 0;
        led_animation_speed = new_speed;
        I2C3733_Control_Set(led_enabled);
        if (led_animation_breathing) gcr_breathe = gcr_desired;
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
    case HID_PKT_REQ_CONFIG_LED_GET:
      {
        uint8_t* speed_arr = (uint8_t *)(&led_animation_speed); // WARNING: Assumes MCU is little endian/network byte order!
        send_buffer[4] = led_enabled;
        send_buffer[5] = led_lighting_mode;
        send_buffer[6] = gcr_desired;
        send_buffer[7] = gcr_breathe;
        send_buffer[8] = gcr_actual;
        send_buffer[9] = led_animation_id;
        send_buffer[10] = led_animation_breathing;
        send_buffer[11] = led_animation_direction;
        memcpy(&send_buffer[12], speed_arr, sizeof(float));
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
    case HID_PKT_REQ_LED_SET_ALL:
      led_matrix_set_all(data[4], data[5], data[6]);
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
          uint8_t r = data[offset++];
          uint8_t g = data[offset++];
          uint8_t b = data[offset++];
          led_matrix_set(i - 1, r, g, b);
        }
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
    case HID_PKT_REQ_DEVICE_PARAMS:
      {
        send_buffer[4] = ISSI3733_LED_COUNT;
        // WARNING: Assumes MCU is little endian/network byte order!
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
          // WARNING: Assumes MCU is little endian/network byte order!
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
    case HID_PKT_REQ_LED_PATTERN_SET:
      {
        uint8_t count = data[4];
        if (count > 10) count = 10;
        uint8_t repeat = data[5];
        if (repeat > 8) repeat = 8;
        issi3733_rgb_t colors[count];
        for (uint8_t i = 0; i < count; i++)
        {
          *colors[i].r = data[7+i*3];
          *colors[i].g = data[8+i*3];
          *colors[i].b = data[9+i*3];
        }
        led_matrix_set_custom_pattern(colors, count, repeat, data[6]);
        raw_hid_send(send_buffer, RAW_EPSIZE);
        break;
      }
    default:
			// Unknown message, send error
			send_buffer[0] = HID_PKT_RES_ERR;
			raw_hid_send(send_buffer, RAW_EPSIZE);
      break;
  }
}
