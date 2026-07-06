#pragma once

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>

class String : public std::string {
public:
    String() = default;
    String(const char* s) : std::string(s ? s : "") {}
    explicit String(const std::string& s) : std::string(s) {}
    String(size_t count, char ch) : std::string(count, ch) {}

    String operator+(const String& other) const { return String(std::string(*this) + other); }
    String operator+(const char* other) const { return String(std::string(*this) + other); }
};

struct IPAddress {
    uint8_t bytes[4] = {0, 0, 0, 0};

    IPAddress() = default;
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : bytes{a, b, c, d} {}

    String toString() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);
        return String(buf);
    }
};

class SerialStub {
public:
    template <typename... Args>
    void printf(const char* fmt, Args... args) {
        std::printf(fmt, args...);
    }
    void println(const char* msg) { std::puts(msg); }
};

inline SerialStub Serial;
inline uint32_t millis() { return 0; }
inline void delay(uint32_t) {}

#define FIRMWARE_VERSION "1.0.0-phase1-test"
