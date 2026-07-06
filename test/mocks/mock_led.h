#pragma once

#include <cstdint>

struct MockLedState {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    bool blink = false;
};

inline MockLedState gMockLed;
