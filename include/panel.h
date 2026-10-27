#pragma once
#include <Arduino.h>
#include <initializer_list>
class Panel {
public:
  void begin();
  bool show(const uint8_t *blackBits);
  void sleep();

private:
  void command(uint8_t value);
  void data(uint8_t value);
  void reg(uint8_t command, std::initializer_list<uint8_t> values);
  bool ready();
  void pixels(uint8_t command, const uint8_t *bits, bool blank = false);
};
