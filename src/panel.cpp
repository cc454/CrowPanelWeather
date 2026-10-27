// Original driver implementation. Register values follow Elecrow's published
// legacy and green-sticker hardware examples; see THIRD_PARTY.md for references.
#include "panel.h"
#include <SPI.h>
static constexpr int CS = 45, DC = 46, RST = 47, BUSY_PIN = 48;
void Panel::begin() {
  pinMode(41, OUTPUT);
  digitalWrite(41, HIGH);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  pinMode(DC, OUTPUT);
  pinMode(RST, OUTPUT);
  pinMode(BUSY_PIN, INPUT);
  SPI.begin(12, -1, 11, CS);
}
void Panel::command(uint8_t value) {
  digitalWrite(DC, LOW);
  digitalWrite(CS, LOW);
  SPI.transfer(value);
  digitalWrite(CS, HIGH);
}
void Panel::data(uint8_t value) {
  digitalWrite(DC, HIGH);
  digitalWrite(CS, LOW);
  SPI.transfer(value);
  digitalWrite(CS, HIGH);
}
void Panel::reg(uint8_t cmd, std::initializer_list<uint8_t> values) {
  command(cmd);
  for (auto v : values)
    data(v);
}
bool Panel::ready() {
  uint32_t start = millis();
  while (digitalRead(BUSY_PIN)) {
    if (millis() - start > 20000)
      return false;
    delay(5);
  }
  return true;
}
void Panel::pixels(uint8_t cmd, const uint8_t *bits, bool blank) {
  command(cmd);
  digitalWrite(DC, HIGH);
  digitalWrite(CS, LOW);
  for (size_t i = 0; i < 15000; ++i) {
    SPI.transfer(blank ? 0xff : uint8_t(~bits[i]));
    if (i % 1024 == 0)
      yield();
  }
  digitalWrite(CS, HIGH);
}
bool Panel::show(const uint8_t *bits) {
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(RST, HIGH);
  delay(10);
  digitalWrite(RST, LOW);
  delay(100);
  digitalWrite(RST, HIGH);
  delay(100);
  if (!ready()) {
    SPI.endTransaction();
    return false;
  }
#if CPW_GREEN_PANEL
  reg(0x00, {0x3f, 0x4d});
  reg(0x01, {0x03, 0x10, 0x3f, 0x3f, 0x03});
  reg(0x06, {0x96, 0x96, 0x29});
  reg(0x30, {0x09});
  reg(0x61, {0x01, 0x90, 0x01, 0x2c});
  reg(0x82, {0x05});
  reg(0x50, {0x97});
  reg(0x60, {0x22});
  reg(0xe3, {0x88});
  pixels(0x10, bits, true);
  reg(0x50, {0xd7});
  pixels(0x13, bits);
  // Full-refresh waveform tables: seven control bytes followed by zero padding.
  const uint8_t wave[5][7] = {{1, 0x14, 0x0a, 0x14, 0, 1, 1},
                              {1, 0x54, 0x0a, 0x94, 0, 1, 1},
                              {1, 0x54, 0x0a, 0x94, 0, 1, 1},
                              {1, 0x94, 0x0a, 0x54, 0, 1, 1},
                              {1, 0x94, 0x0a, 0x54, 0, 1, 1}};
  for (int row = 0; row < 5; ++row) {
    command(0x20 + row);
    for (int i = 0; i < 42; ++i)
      data(i < 7 ? wave[row][i] : 0);
  }
  reg(0x17, {0xa5});
#else
  command(0x12);
  delay(10);
  if (!ready()) {
    SPI.endTransaction();
    return false;
  }
  reg(0x21, {0x40, 0});
  reg(0x3c, {0x05});
  reg(0x11, {0x03});
  reg(0x44, {0, 49});
  reg(0x45, {0, 0, 0x2b, 0x01});
  reg(0x4e, {0});
  reg(0x4f, {0, 0});
  pixels(0x26, bits, true);
  pixels(0x24, bits);
  reg(0x22, {0xf7});
  command(0x20);
#endif
  delay(100);
  bool ok = ready();
  SPI.endTransaction();
  return ok;
}
void Panel::sleep() {
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
#if CPW_GREEN_PANEL
  reg(0x07, {0xa5});
#else
  reg(0x10, {0x01});
#endif
  SPI.endTransaction();
  delay(50);
}
