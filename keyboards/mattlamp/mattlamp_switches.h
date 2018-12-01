#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

void mattlamp_switches_init(void);
void mattlamp_switches_read(void);
bool mattlamp_switches_get(uint8_t sw);
