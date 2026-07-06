#pragma once

#include <stdint.h>

bool ws2812Init(uint8_t pin);
void ws2812SetPixel(uint8_t pin, uint8_t r, uint8_t g, uint8_t b);
