#include "led_ws2812.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
gpio_num_t ledGpio = GPIO_NUM_NC;

void IRAM_ATTR sendByte(uint8_t byte) {
    for (int bit = 7; bit >= 0; --bit) {
        gpio_set_level(ledGpio, 1);
        if (byte & (1 << bit)) {
            esp_rom_delay_us(1);
        } else {
            esp_rom_delay_us(0);
        }
        gpio_set_level(ledGpio, 0);
        esp_rom_delay_us(1);
    }
}
}  // namespace

bool ws2812Init(uint8_t pin) {
    ledGpio = static_cast<gpio_num_t>(pin);
    pinMode(pin, OUTPUT);
    gpio_set_drive_capability(ledGpio, GPIO_DRIVE_CAP_3);
    gpio_set_level(ledGpio, 0);
    return true;
}

void ws2812SetPixel(uint8_t pin, uint8_t r, uint8_t g, uint8_t b) {
    if (pin != static_cast<uint8_t>(ledGpio)) {
        ws2812Init(pin);
    }

    taskDISABLE_INTERRUPTS();
    // WS2812 GRB order
    sendByte(g);
    sendByte(r);
    sendByte(b);
    taskENABLE_INTERRUPTS();

    esp_rom_delay_us(80);
}
