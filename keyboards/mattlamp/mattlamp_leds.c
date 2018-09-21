#include "mattlamp_leds.h"

void _leds_frame(uint8_t mode);
void _leds_frame_solid(void);
void _leds_frame_cycle(void);
void _leds_frame_chase(void);
void _leds_frame_strobe_all(void);
void _leds_frame_strobe_chase(void);
void _leds_frame_fade(void);

void _leds_frame_cycle_effects(void);

void _leds_set_all_real(uint8_t r, uint8_t g, uint8_t b);
void _leds_set_one_real(uint8_t r, uint8_t g, uint8_t b, uint8_t i);

void _leds_reset_anim(void);

/******************************************************************************
 * Module static vars
 *****************************************************************************/

#define FADE_FRAMES 128
#define FADE_FRAMES_2 (FADE_FRAMES*2)

bool frame_zero = true;
LED_TYPE led[RGBLED_NUM];
uint16_t last_timer;
uint16_t frame_index = 0;

#define LEDS_CYCLE_EFFECTS_START LEDS_MODE_SOLID
#define LEDS_CYCLE_EFFECTS_END LEDS_MODE_FADE
#define LEDS_CYCLE_EFFECTS_TIMEOUT 10000
bool effect_cycle_started = false;
uint32_t effect_cycle_timer;
uint8_t effect_cycle_current = LEDS_CYCLE_EFFECTS_START;

/******************************************************************************
 * Public interface
 *****************************************************************************/

void mattlamp_leds_init(void) {
#ifndef NO_EEPROM
	// Read current configuration from EEPROM
	eeprom_read_block(leds_config.raw, EECONFIG_MATTLAMP_LEDS, LEDS_DATA_LEN);
#endif
	// Sane defaults
	if (leds_config.parms.mode > LEDS_MAX_MODES) {
		leds_config.parms.mode = LEDS_MODE_OFF;
		leds_config.parms.timeout1 = 1500;
		leds_config.parms.timeout2 = 50;
		leds_config.parms.color_len = 1;
		for (uint8_t i = 0; i < 8; i++) {
			leds_config.parms.colors[i].r = leds_config.parms.colors[i].g = leds_config.parms.colors[i].b = i == 0 ? 0xFF : 0;
		}
		mattlamp_leds_commit();
	}
	_leds_reset_anim();
}

void mattlamp_leds_frame(void) {
	switch (leds_config.parms.mode) {
	case LEDS_MODE_LIVE:
		// Do nothing, other methods must be called to set leds
		break;
	case LEDS_MODE_OFF:
		_leds_set_all_real(0, 0, 0);
		ws2812_setleds(led, RGBLED_NUM);
		break;
	case LEDS_MODE_CYCLE_EFFECTS:
		_leds_frame_cycle_effects();
		break;
	default:
		_leds_frame(leds_config.parms.mode);
		break;
	}
}

void mattlamp_leds_set_all(uint8_t r, uint8_t g, uint8_t b) {
	if (leds_config.parms.mode == LEDS_MODE_LIVE) {
		_leds_set_all_real(r, g, b);
		ws2812_setleds(led, RGBLED_NUM);
	}
}

void mattlamp_leds_set_one(uint8_t r, uint8_t g, uint8_t b, uint8_t i) {
	if (leds_config.parms.mode == LEDS_MODE_LIVE && i < RGBLED_NUM) {
		_leds_set_one_real(r, g, b, i);
		ws2812_setleds(led, RGBLED_NUM);
	}
}

void mattlamp_leds_mode_step(void) {
	if (++leds_config.parms.mode > LEDS_MAX_MODES) leds_config.parms.mode = LEDS_MODE_OFF;
	mattlamp_leds_config_updated();
	xprintf("mattlamp_leds_mode_step: mode advanced to %u\n", leds_config.parms.mode);
}

void mattlamp_leds_mode_set(uint8_t mode) {
	if (mode > LEDS_MAX_MODES) mode = LEDS_MAX_MODES;
	leds_config.parms.mode = mode;
	mattlamp_leds_config_updated();
	xprintf("mattlamp_leds_mode_set: mode set to %u\n", leds_config.parms.mode);
}

void mattlamp_leds_commit(void) {
#ifndef NO_EEPROM
	// Write the active in-memory configuration to EEPROM
	eeprom_update_block(leds_config.raw, EECONFIG_MATTLAMP_LEDS, LEDS_DATA_LEN);
#endif
}

void mattlamp_leds_config_updated(void) {
	_leds_reset_anim();
	effect_cycle_current = LEDS_CYCLE_EFFECTS_START;
	effect_cycle_started = false;
}

bool mattlamp_leds_get_status(uint8_t i, uint8_t* r, uint8_t* g, uint8_t* b) {
	if (i < RGBLED_NUM) {
		*r = led[i].r;
		*g = led[i].g;
		*b = led[i].b;
		return true;
	}
	return false;
}

/******************************************************************************
 * Private methods
 *****************************************************************************/

void _leds_reset_anim(void) {
	frame_index = 0;
	frame_zero = true;
}

void _leds_set_all_real(uint8_t r, uint8_t g, uint8_t b) {
	for (uint16_t i = 0; i < RGBLED_NUM; i++) {
		led[i].r = r;
		led[i].g = g;
		led[i].b = b;
	}
}

void _leds_set_one_real(uint8_t r, uint8_t g, uint8_t b, uint8_t i) {
	if (i > (RGBLED_NUM - 1)) return;
	led[i].r = r;
	led[i].g = g;
	led[i].b = b;
}

void _leds_frame(uint8_t mode) {
	switch (mode)
	{
	case LEDS_MODE_SOLID:
		_leds_frame_solid();
		break;
	case LEDS_MODE_CYCLE:
		_leds_frame_cycle();
		break;
	case LEDS_MODE_CHASE:
		_leds_frame_chase();
		break;
	case LEDS_MODE_STROBE_ALL:
		_leds_frame_strobe_all();
		break;
	case LEDS_MODE_STROBE_CHASE:
		_leds_frame_strobe_chase();
		break;
	case LEDS_MODE_FADE:
		_leds_frame_fade();
		break;
	}
}

void _leds_frame_solid(void) {
	if (frame_zero) {
		if (leds_config.parms.color_len > 0) {
			_leds_set_all_real(leds_config.parms.colors[0].r, leds_config.parms.colors[0].g, leds_config.parms.colors[0].b);
		} else {
			_leds_set_all_real(0, 0, 0);
		}
		frame_zero = false;
		ws2812_setleds(led, RGBLED_NUM);
	}
}

void _leds_frame_cycle(void) {
	if (leds_config.parms.color_len < 1 || (!frame_zero && timer_elapsed(last_timer) < leds_config.parms.timeout1)) {
		return;
	}
	last_timer = timer_read();
	frame_zero = false;

	_leds_set_all_real(leds_config.parms.colors[frame_index].r, leds_config.parms.colors[frame_index].g, leds_config.parms.colors[frame_index].b);

	if (++frame_index >= leds_config.parms.color_len) frame_index = 0;
	ws2812_setleds(led, RGBLED_NUM);
}

void _leds_frame_chase(void) {
	if (leds_config.parms.color_len < 1 || (!frame_zero && timer_elapsed(last_timer) < leds_config.parms.timeout1)) {
		return;
	}
	last_timer = timer_read();
	frame_zero = false;

	uint8_t i, j;
	for (i = 0; i < RGBLED_NUM; i++) {
		j = (i + frame_index) % leds_config.parms.color_len;
		_leds_set_one_real(leds_config.parms.colors[j].r, leds_config.parms.colors[j].g, leds_config.parms.colors[j].b, i);
	}

	frame_index++;
	if (frame_index >= leds_config.parms.color_len) frame_index = 0;
	ws2812_setleds(led, RGBLED_NUM);
}

void _leds_frame_strobe_all(void) {
	if (leds_config.parms.color_len < 1 || (!frame_zero && timer_elapsed(last_timer) < leds_config.parms.timeout2)) {
		return;
	}
	last_timer = timer_read();
	frame_zero = false;

	// Frame 0: On color 0
	// Frame 1,2: Off
	// Frame 3: On color 1
	// Frame 4,5: Off
	// ...
	if (frame_index % 3 == 0) {
		uint8_t i = frame_index / 3;
		_leds_set_all_real(leds_config.parms.colors[i].r, leds_config.parms.colors[i].g, leds_config.parms.colors[i].b);
	} else {
		_leds_set_all_real(0, 0, 0);
	}
	ws2812_setleds(led, RGBLED_NUM);

	// Advance frame index
	if (++frame_index >= (leds_config.parms.color_len * 3)) frame_index = 0;
}

void _leds_frame_strobe_chase(void) {
	if (leds_config.parms.color_len < 1 || (!frame_zero && timer_elapsed(last_timer) < leds_config.parms.timeout2)) {
		return;
	}
	last_timer = timer_read();
	frame_zero = false;

	// Pick a random LED and a random color and strobe it
	_leds_set_all_real(0, 0, 0);
	if (frame_index == 0) {
		uint8_t i = (uint8_t)(rand() % RGBLED_NUM);
		uint8_t j = (uint8_t)(rand() % leds_config.parms.color_len);
		_leds_set_one_real(leds_config.parms.colors[j].r, leds_config.parms.colors[j].g, leds_config.parms.colors[j].b, i);
	}
	ws2812_setleds(led, RGBLED_NUM);

	// Advance frame index
	if (++frame_index >= 3) frame_index = 0;
}

void _leds_frame_fade(void) {
	if (leds_config.parms.color_len < 1 || (!frame_zero && timer_elapsed(last_timer) < leds_config.parms.timeout1 / FADE_FRAMES_2)) {
		return;
	}
	last_timer = timer_read();
	frame_zero = false;

	// Determine what step we are on
	uint16_t step = frame_index % FADE_FRAMES_2;  // Gives range [0 - FADE_FRAMES_2)
	if (step >= FADE_FRAMES) {
		step -= FADE_FRAMES;
		step = FADE_FRAMES - step - 1;
	}

	// Determine what color we are using
	uint16_t color = frame_index / FADE_FRAMES_2; // Gives range [0 - color_len)

	// Determine the value factor and set led colors
	float value = step / (float)FADE_FRAMES;
	_leds_set_all_real(
		(uint8_t)(leds_config.parms.colors[color].r * value),
		(uint8_t)(leds_config.parms.colors[color].g * value),
		(uint8_t)(leds_config.parms.colors[color].b * value));
		ws2812_setleds(led, RGBLED_NUM);

	// Advance frame index
	if (++frame_index >= ((uint16_t)FADE_FRAMES_2 * leds_config.parms.color_len)) frame_index = 0;
}

void _leds_frame_cycle_effects(void) {
	_leds_frame(effect_cycle_current);

	if (!effect_cycle_started || timer_elapsed32(effect_cycle_timer) > LEDS_CYCLE_EFFECTS_TIMEOUT) {
		effect_cycle_timer = timer_read32();
		if (effect_cycle_started) {
			if (++effect_cycle_current > LEDS_CYCLE_EFFECTS_END) effect_cycle_current = LEDS_CYCLE_EFFECTS_START;
			_leds_reset_anim();
		} else effect_cycle_started = true;
	}
}
