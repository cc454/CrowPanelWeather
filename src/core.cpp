#include "core.h"
#include "timezones.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <cstring>
#ifdef _WIN32
// Native test portability; ESP32/newlib provides the POSIX versions directly.
static tm *localtime_r(const time_t *value, tm *out) {
  return localtime_s(out, value) == 0 ? out : nullptr;
}
static tm *gmtime_r(const time_t *value, tm *out) {
  return gmtime_s(out, value) == 0 ? out : nullptr;
}
static int setenv(const char *key, const char *value, int) { return _putenv_s(key, value); }
#endif

namespace cpw {
namespace {
bool doc(const std::string &raw, JsonDocument &d, const char *file, std::string &error) {
  const size_t start = raw.compare(0, 3, "\xef\xbb\xbf") == 0 ? 3 : 0;
  if (raw.size() > MaxResponse || deserializeJson(d, raw.data() + start, raw.size() - start) ||
      !d.is<JsonObject>()) {
    error = std::string(file) + ": invalid JSON object";
    return false;
  }
  return true;
}
float number(JsonVariantConst v) {
  if (!v.is<float>() && !v.is<int>())
    return NAN;
  float result = v.as<float>();
  return std::isfinite(result) ? result : NAN;
}
bool settingsFrom(JsonVariantConst d, Config &c, std::string &e) {
  if (!d["forecast_days"].isNull()) {
    if (!d["forecast_days"].is<int>()) {
      e = "settings: forecast_days must be 3 or 5";
      return false;
    }
    c.forecastDays = d["forecast_days"].as<int>();
    if (c.forecastDays != 3 && c.forecastDays != 5) {
      e = "settings: forecast_days must be 3 or 5";
      return false;
    }
  }
  if (!d["forecast_view"].isNull()) {
    if (!d["forecast_view"].is<const char *>()) {
      e = "settings: invalid forecast_view";
      return false;
    }
    const std::string view = d["forecast_view"].as<std::string>();
    if (view != "classic_three" && view != "classic_five" && view != "large_three") {
      e = "settings: forecast_view must be classic_three, classic_five or large_three";
      return false;
    }
    c.largeThree = view == "large_three";
    c.forecastDays = view == "classic_five" ? 5 : 3;
  }
  if (!d["power_profile"].isNull()) {
    if (!d["power_profile"].is<const char *>()) {
      e = "settings: invalid power_profile";
      return false;
    }
    std::string profile = d["power_profile"].as<std::string>();
    if (profile != "usb" && profile != "battery") {
      e = "settings: power_profile must be usb or battery";
      return false;
    }
    c.battery = profile == "battery";
  }
  for (auto key : {"show_sun_times", "show_wind"})
    if (!d[key].isNull() && !d[key].is<bool>()) {
      e = std::string("settings: ") + key + " must be boolean";
      return false;
    }
  c.sun = d["show_sun_times"] | false;
  c.wind = d["show_wind"] | false;
  return true;
}
bool validDate(const std::string &s) {
  return s.size() == 10 && s[4] == '-' && s[7] == '-' &&
         s.find_first_not_of("0123456789-") == std::string::npos;
}
} // namespace
bool parseConfig(const std::string &secret, const std::string &location,
                 const std::string &settings, Config &out, std::string &e) {
  Config c;
  JsonDocument s, l, t;
  if (!doc(secret, s, "secrets", e) || !doc(location, l, "location", e))
    return false;
  if (!s["wifi_ssid"].is<const char *>() || !s["wifi_password"].is<const char *>()) {
    e = "secrets: SSID and password must be strings";
    return false;
  }
  c.ssid = s["wifi_ssid"].as<std::string>();
  c.password = s["wifi_password"].as<std::string>();
  if (c.ssid.empty() || c.ssid.size() > 32 || c.password.size() > 63 ||
      c.ssid.find('\0') != std::string::npos || c.password.find('\0') != std::string::npos) {
    e = "secrets: invalid SSID/password length";
    return false;
  }
  if ((!l["latitude"].is<double>() && !l["latitude"].is<int>()) ||
      (!l["longitude"].is<double>() && !l["longitude"].is<int>())) {
    e = "location: numeric latitude and longitude required";
    return false;
  }
  c.latitude = l["latitude"].as<double>();
  c.longitude = l["longitude"].as<double>();
  if (!std::isfinite(c.latitude) || !std::isfinite(c.longitude) || std::abs(c.latitude) > 90 ||
      std::abs(c.longitude) > 180) {
    e = "location: coordinates out of range";
    return false;
  }
  if ((!l["label"].isNull() && !l["label"].is<const char *>()) ||
      (!l["timezone"].isNull() && !l["timezone"].is<const char *>())) {
    e = "location: label/timezone must be strings";
    return false;
  }
  c.label = l["label"] | "Weather";
  c.timezone = l["timezone"] | "auto";
  if (c.label.size() > 40 || c.timezone.size() > 64 || c.timezone.empty()) {
    e = "location: invalid label/timezone";
    return false;
  }
  if (c.timezone != "auto" && posixTimezone(c.timezone).empty()) {
    e = "location: unsupported IANA timezone";
    return false;
  }
  if (!settings.empty() &&
      (!doc(settings, t, "settings", e) || !settingsFrom(t.as<JsonVariantConst>(), c, e)))
    return false;
  out = c;
  e.clear();
  return true;
}
std::string configJson(const Config &c) {
  JsonDocument d;
  d["wifi_ssid"] = c.ssid;
  d["wifi_password"] = c.password;
  d["latitude"] = c.latitude;
  d["longitude"] = c.longitude;
  d["label"] = c.label;
  d["timezone"] = c.timezone;
  d["forecast_days"] = c.forecastDays;
  d["forecast_view"] = c.largeThree          ? "large_three"
                       : c.forecastDays == 3 ? "classic_three"
                                             : "classic_five";
  d["power_profile"] = c.battery ? "battery" : "usb";
  d["show_sun_times"] = c.sun;
  d["show_wind"] = c.wind;
  std::string s;
  serializeJson(d, s);
  return s;
}
bool restoreConfig(const std::string &raw, Config &out, std::string &e) {
  return parseConfig(raw, raw, raw, out, e);
}
std::string locationKey(const Config &c) {
  char key[96];
  snprintf(key, sizeof(key), "%.6f,%.6f,%s", c.latitude, c.longitude, c.timezone.c_str());
  return key;
}
std::string requestUrl(const Config &c) {
  char coordinates[96];
  snprintf(coordinates, sizeof(coordinates),
           "https://api.open-meteo.com/v1/forecast?latitude=%.6f&longitude=%.6f", c.latitude,
           c.longitude);
  std::string tz;
  for (unsigned char ch : c.timezone) {
    if (std::isalnum(ch) || ch == '/' || ch == '_' || ch == '-')
      tz += char(ch);
    else {
      char s[4];
      snprintf(s, 4, "%%%02X", ch);
      tz += s;
    }
  }
  return std::string(coordinates) +
         "&current=temperature_2m,apparent_temperature,weather_code,is_day,wind_speed_10m,wind_"
         "direction_10m&hourly=temperature_2m&daily=temperature_2m_max,temperature_2m_min,weather_"
         "code,precipitation_probability_max,uv_index_max,sunrise,sunset&past_days=1&forecast_days="
         "7&timezone=" +
         tz + "&timeformat=unixtime&temperature_unit=celsius&wind_speed_unit=kmh";
}
std::string posixTimezone(const std::string &name) {
  for (const auto &zone : kTimezones)
    if (name == zone.name)
      return zone.posix;
  if (name == "GMT" || name == "UTC")
    return "UTC0";
  return "";
}
bool parseWeather(const std::string &raw, int64_t fetched, Weather &out, std::string &e) {
  JsonDocument d;
  if (!doc(raw, d, "weather", e))
    return false;
  Weather w;
  w.fetched = fetched;
  if (d["error"] | false) {
    e = "Weather service rejected request";
    return false;
  }
  w.timezone = d["timezone"] | "";
  w.posix = posixTimezone(w.timezone);
  if (w.posix.empty()) {
    e = "Weather returned unsupported timezone";
    return false;
  }
  JsonObjectConst current = d["current"];
  w.current = number(current["temperature_2m"]);
  w.feels = number(current["apparent_temperature"]);
  w.currentTime = current["time"] | int64_t(0);
  w.code = current["weather_code"].is<int>() ? current["weather_code"].as<int>() : -1;
  w.isDay = (current["is_day"] | 1) != 0;
  w.wind = number(current["wind_speed_10m"]);
  w.direction = number(current["wind_direction_10m"]);
  if (!std::isfinite(w.current) || w.current < -100 || w.current > 70 || w.currentTime <= 0 ||
      fetched <= 0) {
    e = "Weather missing valid current temperature/time";
    return false;
  }
  if (d["current_units"]["temperature_2m"] != "°C" || d["hourly_units"]["temperature_2m"] != "°C") {
    e = "Unexpected weather units";
    return false;
  }
  JsonObjectConst hourly = d["hourly"];
  auto times = hourly["time"].as<JsonArrayConst>();
  auto temps = hourly["temperature_2m"].as<JsonArrayConst>();
  if (times.size() < 24 || times.size() > 256 || temps.size() != times.size()) {
    e = "Invalid hourly forecast";
    return false;
  }
  int64_t previous = 0;
  for (size_t i = 0; i < times.size(); ++i) {
    int64_t stamp = times[i] | int64_t(0);
    if (stamp <= previous || (previous && stamp - previous != Hour)) {
      e = "Non-hourly forecast timestamps";
      return false;
    }
    previous = stamp;
    float val = number(temps[i]);
    if (std::isfinite(val) && (val < -100 || val > 70))
      val = NAN;
    w.hours.push_back({stamp, val});
  }
  JsonObjectConst daily = d["daily"];
  auto dates = daily["time"].as<JsonArrayConst>();
  if (dates.size() < 2 || dates.size() > 10) {
    e = "Invalid daily forecast";
    return false;
  }
  for (const char *field : {"temperature_2m_max", "temperature_2m_min", "weather_code",
                            "precipitation_probability_max", "uv_index_max", "sunrise", "sunset"})
    if (daily[field].as<JsonArrayConst>().size() != dates.size()) {
      e = "Daily array lengths differ";
      return false;
    }
  const int offset = d["utc_offset_seconds"] | 0;
  for (size_t i = 0; i < dates.size(); ++i) {
    // Open-Meteo documents daily unixtime dates relative to utc_offset_seconds.
    time_t stamp = (dates[i] | int64_t(0)) + offset;
    tm local{};
    gmtime_r(&stamp, &local);
    char date[11];
    strftime(date, sizeof(date), "%Y-%m-%d", &local);
    Daily day;
    day.date = date;
    if (!validDate(day.date) || (i && day.date <= w.days.back().date)) {
      e = "Invalid daily dates";
      return false;
    }
    day.high = number(daily["temperature_2m_max"][i]);
    day.low = number(daily["temperature_2m_min"][i]);
    if (std::isfinite(day.high) && std::isfinite(day.low) && day.low > day.high) {
      e = "Inverted daily range";
      return false;
    }
    day.probability = number(daily["precipitation_probability_max"][i]);
    if (day.probability < 0 || day.probability > 100)
      day.probability = NAN;
    day.uv = number(daily["uv_index_max"][i]);
    if (day.uv < 0)
      day.uv = NAN;
    day.code = daily["weather_code"][i].is<int>() ? daily["weather_code"][i].as<int>() : -1;
    day.sunrise = daily["sunrise"][i] | int64_t(0);
    day.sunset = daily["sunset"][i] | int64_t(0);
    w.days.push_back(day);
  }
  out = std::move(w);
  e.clear();
  return true;
}
bool applyTimezone(const Weather &w) {
  if (w.posix.empty())
    return false;
  setenv("TZ", w.posix.c_str(), 1);
  tzset();
  return true;
}
std::string localTime(int64_t value, const char *format) {
  if (value <= 0)
    return "--";
  time_t stamp = value;
  tm date{};
  localtime_r(&stamp, &date);
  char s[64];
  strftime(s, sizeof(s), format, &date);
  return s;
}
std::string localDate(int64_t value) { return localTime(value, "%Y-%m-%d"); }
const Daily *today(const Weather &w, int64_t now) {
  const auto date = localDate(now);
  for (const auto &day : w.days)
    if (day.date == date)
      return &day;
  return nullptr;
}
std::vector<Hourly> dayHours(const Weather &w, const std::string &date) {
  std::vector<Hourly> result;
  for (const auto &h : w.hours)
    if (localDate(h.time) == date)
      result.push_back(h);
  return result;
}
std::vector<Daily> futureDays(const Weather &w, int64_t now, int count) {
  std::vector<Daily> r;
  if (now <= 0 || count <= 0)
    return r;
  time_t stamp = now;
  tm date{};
  localtime_r(&stamp, &date);
  // Step calendar dates at local noon, not by 86400-second durations. Always
  // reserve tomorrow's tile even when the provider is missing that day's data.
  date.tm_hour = 12;
  date.tm_min = date.tm_sec = 0;
  for (int i = 0; i < count; ++i) {
    ++date.tm_mday;
    date.tm_isdst = -1;
    stamp = mktime(&date);
    Daily next;
    next.date = localDate(stamp);
    for (const auto &day : w.days)
      if (day.date == next.date) {
        next = day;
        break;
      }
    r.push_back(next);
  }
  return r;
}
float overnightLow(const Weather &w, int64_t now) {
  time_t n = now;
  tm start{};
  localtime_r(&n, &start);
  if (start.tm_hour < 6)
    --start.tm_mday;
  start.tm_hour = 18;
  start.tm_min = start.tm_sec = 0;
  start.tm_isdst = -1;
  time_t a = mktime(&start);
  tm end = start;
  ++end.tm_mday;
  end.tm_hour = 6;
  end.tm_isdst = -1;
  time_t b = mktime(&end);
  float low = INFINITY;
  size_t got = 0;
  for (const auto &h : w.hours)
    if (h.time >= a && h.time < b) {
      if (!std::isfinite(h.temperature))
        return NAN;
      low = std::min(low, h.temperature);
      ++got;
    }
  return got == size_t((b - a) / Hour) && got > 0 ? low : NAN;
}
Grid gridBounds(float low, float high) {
  if (!std::isfinite(low) || !std::isfinite(high) || low > high)
    return {0, 0, false};
  low = std::floor(low / 5) * 5;
  high = std::ceil(high / 5) * 5;
  if (low == high) {
    low -= 5;
    high += 5;
  }
  return {low, high, true};
}
uint32_t retryDelay(unsigned failures) { return failures <= 1 ? 60 : failures == 2 ? 300 : 900; }
uint32_t retryAfter(const std::string &header, int64_t now) {
  if (header.empty())
    return Hour;
  if (header.find_first_not_of("0123456789") == std::string::npos) {
    unsigned long value = strtoul(header.c_str(), nullptr, 10);
    return uint32_t(std::min(value, 604800UL));
  }
  int day, year, hour, minute, second;
  char monthName[4] = {};
  if (sscanf(header.c_str(), "%*3s, %d %3s %d %d:%d:%d GMT", &day, monthName, &year, &hour, &minute,
             &second) == 6) {
    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *match = strstr(months, monthName);
    if (match && year >= 2020 && day >= 1 && day <= 31 && hour >= 0 && hour < 24 && minute >= 0 &&
        minute < 60 && second >= 0 && second < 60) {
      unsigned month = (match - months) / 3 + 1;
      int y = year - (month <= 2);
      int era = y / 400;
      unsigned yoe = y - era * 400;
      unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
      unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
      int64_t stamp =
          (int64_t(era) * 146097 + doe - 719468) * 86400 + hour * 3600 + minute * 60 + second;
      return uint32_t(std::max<int64_t>(0, std::min<int64_t>(604800, stamp - now)));
    }
  }
  return Hour;
}
int64_t advanceSchedule(int64_t due, int64_t now) {
  if (due <= 0)
    return now + Hour;
  return due <= now ? due + ((now - due) / Hour + 1) * Hour : due;
}
const char *weatherIcon(int code, bool day) {
  switch (code) {
  case 0:
    return day ? "clear-day" : "clear-night";
  case 1:
    return day ? "mainly-clear-day" : "mainly-clear-night";
  case 2:
    return day ? "partly-cloudy-day" : "partly-cloudy-night";
  case 3:
    return "overcast";
  case 45:
    return "fog";
  case 48:
    return "rime-fog";
  case 51:
  case 53:
  case 55:
    return "drizzle";
  case 56:
  case 57:
    return "freezing-drizzle";
  case 61:
  case 63:
    return "rain";
  case 65:
    return "heavy-rain";
  case 66:
  case 67:
    return "freezing-rain";
  case 71:
  case 73:
    return "snow";
  case 75:
    return "heavy-snow";
  case 77:
    return "snow-grains";
  case 80:
  case 81:
    return day ? "rain-showers-day" : "rain-showers-night";
  case 82:
    return "heavy-rain-showers";
  case 85:
  case 86:
    return "snow-showers";
  case 95:
    return "thunderstorm";
  case 96:
  case 99:
    return "hail-thunderstorm";
  case 97:
    return "heavy-thunderstorm";
  default:
    return "unknown";
  }
}
const char *weatherLabel(int code) {
  switch (code) {
  case 0:
    return "Clear";
  case 1:
    return "Mainly clear";
  case 2:
    return "Partly cloudy";
  case 3:
    return "Overcast";
  case 45:
    return "Fog";
  case 48:
    return "Rime fog";
  case 51:
  case 53:
  case 55:
    return "Drizzle";
  case 56:
  case 57:
    return "Freezing drizzle";
  case 61:
  case 63:
    return "Rain";
  case 65:
    return "Heavy rain";
  case 66:
  case 67:
    return "Freezing rain";
  case 71:
  case 73:
    return "Snow";
  case 75:
    return "Heavy snow";
  case 77:
    return "Snow grains";
  case 80:
  case 81:
  case 82:
    return "Rain showers";
  case 85:
  case 86:
    return "Snow showers";
  case 95:
  case 97:
    return "Thunderstorm";
  case 96:
  case 99:
    return "Hail storm";
  default:
    return "Unknown";
  }
}
} // namespace cpw
