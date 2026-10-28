#pragma once
#define HSPI 1
class SPIClass {
public:
  explicit SPIClass(int) {}
  void begin(int, int, int, int) {}
  void end() {}
};
