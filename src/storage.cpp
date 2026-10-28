#include "storage.h"
#include <LittleFS.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>

namespace cpw {
static Preferences preferences;
static SPIClass cardSPI(HSPI);
static bool sdReady = false, fsReady = false, nvsReady = false;
static int newestSlot = 0;
static bool mountCard() {
  SD.end();
  cardSPI.end();
  cardSPI.begin(39, 13, 40, 10);
  sdReady = SD.begin(10, cardSPI, 4000000);
  return sdReady;
}
bool beginStorage() {
  nvsReady = preferences.begin("crowweather", false);
  fsReady = LittleFS.begin(true);
  mountCard();
  return nvsReady;
}
bool cardAvailable() { return sdReady; }
static bool readCard(const char *name, std::string &result, std::string &error,
                     bool optional = false) {
  if (!SD.exists(name)) {
    result.clear();
    if (optional)
      return true;
    const std::string filename = std::string(name).substr(1);
    if (SD.exists((std::string(name) + ".txt").c_str()))
      error = "microSD: rename " + filename + ".txt to " + filename;
    else if (filename == "secrets" && SD.exists("/secret"))
      error = "microSD: rename secret to secrets (plural)";
    else
      error = "microSD: missing /" + filename + " in card root";
    return false;
  }
  File f = SD.open(name, FILE_READ);
  if (!f) {
    result.clear();
    error = "microSD: cannot read " + std::string(name);
    return false;
  }
  if (f.size() > 2048) {
    error = "microSD: " + std::string(name) + " exceeds 2048 bytes";
    return false;
  }
  result = f.readString().c_str();
  return true;
}
bool saveConfiguration(const Config &c) {
  if (!nvsReady)
    return false;
  auto raw = configJson(c);
  return preferences.putString("config", raw.c_str()) == raw.size();
}
bool loadConfiguration(Config &c, bool forceCard, std::string &message) {
  message.clear();
  if (forceCard)
    mountCard();
  Config saved, candidate;
  std::string ignored;
  bool hasSaved =
      nvsReady && restoreConfig(preferences.getString("config", "").c_str(), saved, ignored);
  std::string secret, location, settings;
  if (!sdReady)
    message = "microSD: mount failed; reinsert card and Reload";
  bool files = sdReady && readCard("/secrets", secret, message) &&
               readCard("/location", location, message) &&
               readCard("/settings", settings, message, true);
  if (files && parseConfig(secret, location, settings, candidate, message)) {
    if (hasSaved && !forceCard) {
      candidate.forecastDays = saved.forecastDays;
      candidate.largeThree = saved.largeThree;
      candidate.battery = saved.battery;
      candidate.sun = saved.sun;
      candidate.wind = saved.wind;
    }
    if (!saveConfiguration(candidate)) {
      message = "Cannot save settings; previous configuration retained";
      if (hasSaved && !forceCard) {
        c = saved;
        return true;
      }
      return false;
    }
    c = candidate;
    message.clear();
    return true;
  }
  if (message.empty())
    message = "microSD: add secrets and location files";
  if (!forceCard && hasSaved) {
    c = saved;
    message = "Using saved setup; " + message;
    return true;
  }
  return false;
}
static bool cacheRead(fs::FS &store, int slot, const Config &c, Weather &w) {
  String path = slot ? "/cache-b" : "/cache-a";
  File file = store.open(path, FILE_READ);
  if (!file || file.size() > MaxResponse + 512)
    return false;
  String header = file.readStringUntil('\n');
  if (header.length() > 500)
    return false;
  JsonDocument meta;
  if (deserializeJson(meta, header))
    return false;
  if (meta["location"].as<std::string>() != locationKey(c))
    return false;
  int64_t stamp = meta["fetched"] | int64_t(0);
  std::string error, raw = file.readString().c_str();
  return parseWeather(raw, stamp, w, error);
}
bool loadCache(const Config &c, Weather &w) {
  bool found = false;
  for (int source = 0; source < 2; ++source) {
    if (source ? !sdReady : !fsReady)
      continue;
    fs::FS &store = source ? static_cast<fs::FS &>(SD) : static_cast<fs::FS &>(LittleFS);
    for (int slot = 0; slot < 2; ++slot) {
      Weather candidate;
      if (cacheRead(store, slot, c, candidate) && (!found || candidate.fetched > w.fetched)) {
        w = std::move(candidate);
        newestSlot = slot;
        found = true;
      }
    }
  }
  return found;
}
static bool cacheWrite(fs::FS &store, int target, const Config &c, const std::string &raw,
                       int64_t fetched) {
  const char *path = target ? "/cache-b" : "/cache-a";
  store.remove("/cache-tmp");
  File f = store.open("/cache-tmp", FILE_WRITE);
  if (!f)
    return false;
  JsonDocument meta;
  meta["location"] = locationKey(c);
  meta["fetched"] = fetched;
  std::string header;
  serializeJson(meta, header);
  header += '\n';
  bool good = f.write((const uint8_t *)header.data(), header.size()) == header.size() &&
              f.write((const uint8_t *)raw.data(), raw.size()) == raw.size();
  f.flush();
  f.close();
  if (!good) {
    store.remove("/cache-tmp");
    return false;
  }
  // The newest valid slot is never removed during replacement of the other slot.
  store.remove(path);
  return store.rename("/cache-tmp", path);
}
bool saveCache(const Config &c, const std::string &raw, int64_t fetched) {
  if (raw.size() > MaxResponse)
    return false;
  int target = 1 - newestSlot;
  bool internal = fsReady && cacheWrite(LittleFS, target, c, raw, fetched);
  bool card = sdReady && cacheWrite(SD, target, c, raw, fetched);
  if (internal || card)
    newestSlot = target;
  return internal || card;
}
} // namespace cpw
