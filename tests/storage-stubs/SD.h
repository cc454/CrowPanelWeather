#pragma once
#include "MockStorage.h"
#include "Arduino.h"
#include "SPI.h"
struct SDStub : fs::FS {
  bool begin(int, SPIClass &, int) {
    ++storage_test::mounts;
    return mounted = storage_test::inserted && storage_test::pins[42] == HIGH &&
                     storage_test::clock - storage_test::powerEnabledAt >= 10;
  }
  void end() { mounted = false; }
};
inline SDStub SD;
