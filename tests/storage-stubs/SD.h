#pragma once
#include "MockStorage.h"
#include "SPI.h"
struct SDStub : fs::FS {
  bool begin(int, SPIClass &, int) {
    ++storage_test::mounts;
    return mounted = storage_test::inserted;
  }
  void end() { mounted = false; }
};
inline SDStub SD;
