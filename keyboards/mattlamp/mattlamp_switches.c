#include "mattlamp_switches.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <util/delay.h>
#include "timer.h"

bool _switch_read(bool states[]);

/******************************************************************************
 * Module static vars
 *****************************************************************************/

const uint8_t PROGMEM switch_defs[SWITCHES_COUNT][2] = SWITCHES_PINS;
static bool switch_states[SWITCHES_COUNT];

#if (DEBOUNCING_DELAY > 0)
static uint16_t debouncing_time;
static bool debouncing = false;
bool switch_states_debouncing[SWITCHES_COUNT];
#endif

/******************************************************************************
 * Public interface
 *****************************************************************************/

void mattlamp_switches_init(void) {
    for (uint8_t i = 0; i < SWITCHES_COUNT; i++) {
        uint8_t pin_in = (uint8_t)pgm_read_byte(&switch_defs[i][0]);
        uint8_t pin_out = (uint8_t)pgm_read_byte(&switch_defs[i][1]);

        // Both pins become inputs with pullup so the output pin is effectively disabled and we can later read from the input pin
        _SFR_IO8((pin_out >> 4) + 1) &= ~_BV(pin_out & 0xF); // DDxn: IN
        _SFR_IO8((pin_out >> 4) + 2) |=  _BV(pin_out & 0xF); // PORTxn: HI
        _SFR_IO8((pin_in >> 4) + 1) &= ~_BV(pin_in & 0xF); // DDxn: IN
        _SFR_IO8((pin_in >> 4) + 2) |=  _BV(pin_in & 0xF); // PORTxn: HI
    }
}

void mattlamp_switches_read(void) {

#if (DEBOUNCING_DELAY > 0)
    bool changed = _switch_read(switch_states_debouncing);
    if (changed) {
        debouncing = true;
        debouncing_time = timer_read();
    }
#else
    _switch_read(switch_states);
#endif

#if (DEBOUNCING_DELAY > 0)
    if (debouncing && (timer_elapsed(debouncing_time) > DEBOUNCING_DELAY)) {
        for (uint8_t i = 0; i < SWITCHES_COUNT; i++) {
            switch_states[i] = switch_states_debouncing[i];
        }
        debouncing = false;
    }
#endif

}

bool mattlamp_switches_get(uint8_t sw) {
    if (sw < SWITCHES_COUNT) {
        return switch_states[sw];
    }
    return false;
}

/******************************************************************************
 * Private methods
 *****************************************************************************/

bool _switch_read(bool _states[]) {

    bool changed = false;
    for (uint8_t i = 0; i < SWITCHES_COUNT; i++) {
        uint8_t pin_in = (uint8_t)pgm_read_byte(&switch_defs[i][0]);
        uint8_t pin_out = (uint8_t)pgm_read_byte(&switch_defs[i][1]);

        // Configure output pin as an output, set it low, and wait for that to stabilize
        _SFR_IO8((pin_out >> 4) + 1) |=  _BV(pin_out & 0xF); // DDxn: OUT
        _SFR_IO8((pin_out >> 4) + 2) &= ~_BV(pin_out & 0xF); // PORTxn: LOW
        _delay_us(30);

        // Read the input pin
        bool result = !(_SFR_IO8(pin_in >> 4) & _BV(pin_in & 0xF));
        if (_states[i] != result) {
            _states[i] = result;
            changed = true;
        }

        // Reset the output pin
        _SFR_IO8((pin_out >> 4) + 1) &= ~_BV(pin_out & 0xF); // DDxn: IN
        _SFR_IO8((pin_out >> 4) + 2) |=  _BV(pin_out & 0xF); // PORTxn: HI
    }

    // Return result
    return changed;
}
