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
#if CPW_PANEL_BITBANG
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  digitalWrite(12, LOW);
#else
  SPI.begin(12, -1, 11, CS);
#endif
  delay(100);
}
void Panel::transfer(uint8_t value) {
#if CPW_PANEL_BITBANG
  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(12, LOW);
    digitalWrite(11, (value >> bit) & 1);
    delayMicroseconds(1);
    digitalWrite(12, HIGH);
    delayMicroseconds(1);
  }
#else
  SPI.transfer(value);
#endif
}
void Panel::command(uint8_t value) {
  digitalWrite(DC, LOW);
  digitalWrite(CS, LOW);
  transfer(value);
  digitalWrite(CS, HIGH);
}
void Panel::data(uint8_t value) {
  digitalWrite(DC, HIGH);
  digitalWrite(CS, LOW);
  transfer(value);
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
    if (millis() - start > 20000) {
      Serial.println("[display] BUSY stayed HIGH for 20 seconds");
      return false;
    }
    delay(5);
  }
  return true;
}
void Panel::pixels(uint8_t cmd, const uint8_t *bits, bool blank) {
  command(cmd);
  digitalWrite(DC, HIGH);
  digitalWrite(CS, LOW);
  for (size_t i = 0; i < 15000; ++i) {
#if CPW_PANEL_BITBANG
    data(blank ? 0xff : uint8_t(~bits[i]));
#else
    transfer(blank ? 0xff : uint8_t(~bits[i]));
#endif
    if (i % 1024 == 0)
      yield();
  }
  digitalWrite(CS, HIGH);
}
bool Panel::show(const uint8_t *bits) {
  if (!bits) {
    Serial.println("[display] Framebuffer allocation failed");
    return false;
  }
  Serial.printf("[display] Reset; BUSY=%d, power7=%d, power41=%d\n",
                digitalRead(BUSY_PIN), digitalRead(7), digitalRead(41));
#if !CPW_PANEL_BITBANG
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
#endif
  digitalWrite(RST, HIGH);
#if CPW_GREEN_PANEL
  delay(10);
  digitalWrite(RST, LOW);
  delay(100);
  digitalWrite(RST, HIGH);
  delay(100);
#else
  delay(100);
  digitalWrite(RST, LOW);
  delay(10);
  digitalWrite(RST, HIGH);
  delay(10);
#endif
#if !CPW_GREEN_PANEL
  // Legacy controller is ready before its software reset. The green
  // controller must receive initialization first, as in Elecrow's example.
  if (!ready()) {
#if !CPW_PANEL_BITBANG
    SPI.endTransaction();
#endif
    return false;
  }
#endif
  Serial.println("[display] Reset complete; sending registers and image");
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
  delay(300);
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
#if !CPW_PANEL_BITBANG
    SPI.endTransaction();
#endif
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
  Serial.printf("[display] Refresh sent; BUSY=%d\n", digitalRead(BUSY_PIN));
  bool ok = ready();
#if !CPW_PANEL_BITBANG
  SPI.endTransaction();
#endif
  Serial.println(ok ? "[display] Refresh wait complete (check screen visually)"
                    : "[display] Refresh timed out");
  return ok;
}
void Panel::sleep() {
#if !CPW_PANEL_BITBANG
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
#endif
#if CPW_GREEN_PANEL
  reg(0x07, {0xa5});
#else
  reg(0x10, {0x01});
#endif
#if !CPW_PANEL_BITBANG
  SPI.endTransaction();
#endif
  delay(50);
}
