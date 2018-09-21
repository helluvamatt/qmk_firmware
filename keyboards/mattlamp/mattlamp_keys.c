#include "mattlamp_keys.h"

void _keys_send_combo(void);

void mattlamp_keys_init(void) {
#ifndef NO_EEPROM
	eeprom_read_block(keys_config.raw, EECONFIG_MATTLAMP_KEYS, KEYS_DATA_LEN);
#else
	keys_config.parms.action = KEYS_ACTION_LED_TOGGLE;
	keys_config.parms.param1 = LEDS_MODE_OFF;
	keys_config.parms.param2 = LEDS_MODE_SOLID;
#endif
}

void mattlamp_keys_pressed(void) {
	switch (keys_config.parms.action) {
		case KEYS_ACTION_KEY_COMBO:
			_keys_send_combo();
			break;
		case KEYS_ACTION_LED_STEP:
			mattlamp_leds_mode_step();
			break;
		case KEYS_ACTION_LED_SET:
			mattlamp_leds_mode_set(keys_config.parms.param1);
			break;
		case KEYS_ACTION_LED_TOGGLE:
			if (leds_config.parms.mode != keys_config.parms.param1)
				mattlamp_leds_mode_set(keys_config.parms.param1);
			else
				mattlamp_leds_mode_set(keys_config.parms.param2);
			break;
		case KEYS_ACTION_NONE:
		default:
			// No-op
			break;
	}
}

bool mattlamp_keys_config(uint8_t action, uint8_t param1, uint8_t param2) {
	xprintf("keys config: %u %u %u\n", action, param1, param2);
	if (action > KEYS_ACTION_LED_TOGGLE) return false;
	if (action == KEYS_ACTION_KEY_COMBO && !IS_ANY(param1)) return false;
	keys_config.parms.action = action;
	keys_config.parms.param1 = param1;
	keys_config.parms.param2 = param2;
#ifndef NO_EEPROM
	eeprom_update_block(keys_config.raw, EECONFIG_MATTLAMP_KEYS, KEYS_DATA_LEN);
#endif
	return true;
}

void _keys_send_combo(void) {
	register_mods(keys_config.parms.param2);
	register_code(keys_config.parms.param1);
	unregister_code(keys_config.parms.param1);
	unregister_mods(keys_config.parms.param2);
}