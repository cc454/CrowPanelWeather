#pragma once
// Minimal host-only compatibility shim for exercising the real GFX renderer.
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <cstdio>
#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))
#define pgm_read_dword(p) (*(const uint32_t *)(p))
class __FlashStringHelper;
using String = std::string;
inline void yield() {}
struct SerialStub {
  void println(const char *) {}
};
inline SerialStub Serial;
