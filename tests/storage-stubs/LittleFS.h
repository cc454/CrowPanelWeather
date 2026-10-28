#pragma once
#include "MockStorage.h"
struct LittleFSStub : fs::FS {
  bool begin(bool) { return mounted = true; }
};
inline LittleFSStub LittleFS;
