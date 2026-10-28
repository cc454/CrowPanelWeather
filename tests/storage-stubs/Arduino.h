#pragma once
#include <cstdint>
#define HIGH 1
#define LOW 0
#define OUTPUT 1
namespace storage_test {
inline int pins[49] = {};
inline uint32_t clock = 0, powerEnabledAt = 0;
} // namespace storage_test
inline void pinMode(int, int) {}
inline void digitalWrite(int pin, int value) {
  storage_test::pins[pin] = value;
  if (pin == 42 && value == HIGH)
    storage_test::powerEnabledAt = storage_test::clock;
}
inline void delay(uint32_t ms) { storage_test::clock += ms; }
struct SerialStub {
  void println(const char *) {}
};
inline SerialStub Serial;
