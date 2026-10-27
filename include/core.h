#pragma once
#include <ArduinoJson.h>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace cpw {
constexpr int Hour = 3600;
constexpr size_t MaxResponse = 96 * 1024;
struct Config {
  std::string ssid, password, label, timezone = "auto";
  double latitude = NAN, longitude = NAN;
  int forecastDays = 5;
  bool battery = false, sun = false, wind = false;
};
struct Hourly {
  int64_t time;
  float temperature;
};
struct Daily {
  std::string date;
  float high = NAN, low = NAN, probability = NAN, uv = NAN;
  int code = -1;
  int64_t sunrise = 0, sunset = 0;
};
struct Weather {
  float current = NAN, feels = NAN, wind = NAN, direction = NAN;
  int code = -1;
  bool isDay = true;
  int64_t fetched = 0, currentTime = 0;
  std::string timezone, posix;
  std::vector<Hourly> hours;
  std::vector<Daily> days;
};
bool parseConfig(const std::string &secrets, const std::string &location,
                 const std::string &settings, Config &out, std::string &error);
bool parseWeather(const std::string &raw, int64_t fetched, Weather &out, std::string &error);
std::string configJson(const Config &config);
bool restoreConfig(const std::string &raw, Config &out, std::string &error);
std::string locationKey(const Config &c);
std::string requestUrl(const Config &c);
std::string posixTimezone(const std::string &name);
std::string localDate(int64_t timestamp);
std::string localTime(int64_t timestamp, const char *format = "%H:%M");
bool applyTimezone(const Weather &weather);
float overnightLow(const Weather &weather, int64_t now);
std::vector<Hourly> dayHours(const Weather &weather, const std::string &date);
std::vector<Daily> futureDays(const Weather &weather, int64_t now, int count);
const Daily *today(const Weather &weather, int64_t now);
struct Grid {
  float low, high;
  bool valid;
};
Grid gridBounds(float low, float high);
uint32_t retryDelay(unsigned failureCount);
uint32_t retryAfter(const std::string &header, int64_t now);
int64_t advanceSchedule(int64_t due, int64_t now);
const char *weatherIcon(int code, bool isDay = true);
const char *weatherLabel(int code);
} // namespace cpw
