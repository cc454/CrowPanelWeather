#pragma once
#include "core.h"
namespace cpw {
struct FetchResult {
  std::string raw, error;
  int64_t fetched = 0;
  uint32_t retryAfter = 0;
  bool wifiConnected = false;
};
bool startFetch(const Config &c);
bool fetchRunning();
FetchResult *pollFetch(); // Caller owns the returned pointer.
} // namespace cpw
