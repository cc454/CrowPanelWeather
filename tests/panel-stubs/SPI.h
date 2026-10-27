#pragma once
#include <cstdint>
#define MSBFIRST 1
#define SPI_MODE0 0
struct SPISettings {
  SPISettings(int, int, int) {}
};
struct SPIStub {
  void begin(int, int, int, int) {}
  void beginTransaction(SPISettings) {}
  void endTransaction() {}
  uint8_t transfer(uint8_t);
};
inline SPIStub SPI;
