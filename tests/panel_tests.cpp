// Exercise the actual driver against controller startup states. In particular,
// green BUSY remains high after reset until initialization/refresh is sent.
#include "panel.h"
#include <SPI.h>
#include <cassert>
#include <cstdio>

static uint32_t tick = 0, lowAt = 0, lowDuration = 0;
static int pins[49] = {};
static bool busy = false, holdBusy = false;
static unsigned bytes = 0, updates = 0;
void pinMode(int, int) {}
void digitalWrite(int pin, int level) {
  int previous = pins[pin];
  pins[pin] = level;
  if (pin == 47 && level == LOW)
    lowAt = tick;
  if (pin == 47 && previous == LOW && level == HIGH) {
    lowDuration = tick - lowAt;
    busy = true;
  }
}
int digitalRead(int pin) { return pin == 48 ? int(busy) : pins[pin]; }
uint32_t millis() { return tick; }
void delay(uint32_t ms) { tick += ms; }
void delayMicroseconds(uint32_t) {}
uint8_t SPIStub::transfer(uint8_t value) {
  ++bytes;
  if (!pins[46]) {
    if (value == 0x17) {
      ++updates;
      busy = holdBusy;
    }
  }
  return 0;
}
int main() {
  uint8_t image[15000] = {};
  Panel panel;
  panel.begin();
  assert(!panel.show(nullptr));
  assert(bytes == 0);
  assert(panel.show(image));
  assert(updates == 1 && bytes > 30000);
  assert(lowDuration == 100u);
  holdBusy = true;
  uint32_t before = tick;
  assert(!panel.show(image));
  assert(tick - before >= 20000);
  std::puts("Green panel: startup, reset pulse, null buffer and stuck-BUSY checks passed");
}
