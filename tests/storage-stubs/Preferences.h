#pragma once
#include "MockStorage.h"
class Preferences {
public:
  bool begin(const char *, bool) { return true; }
  String getString(const char *key, const char *fallback) {
    auto &data = storage_test::preferences;
    return data.count(key) ? data[key] : fallback;
  }
  size_t putString(const char *key, const char *value) {
    if (!storage_test::allowSave)
      return 0;
    storage_test::preferences[key] = value;
    return storage_test::preferences[key].size();
  }
};
