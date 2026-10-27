#pragma once
#include "core.h"
namespace cpw {
bool beginStorage();
bool loadConfiguration(Config &c, bool forceCard, std::string &message);
bool saveConfiguration(const Config &c);
bool loadCache(const Config &c, Weather &w);
bool saveCache(const Config &c, const std::string &raw, int64_t fetched);
bool cardAvailable();
} // namespace cpw
