#include "core.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
using namespace cpw;
static int checks = 0;
#define CHECK(value)                                                                               \
  do {                                                                                             \
    if (!(value)) {                                                                                \
      std::cerr << "FAILED line " << __LINE__ << ": " << #value << std::endl;                      \
      return 1;                                                                                    \
    }                                                                                              \
    ++checks;                                                                                      \
  } while (0)
static std::string fixture() {
  JsonDocument d;
  d["timezone"] = "Etc/UTC";
  d["utc_offset_seconds"] = 0;
  d["current"]["temperature_2m"] = 14;
  d["current"]["apparent_temperature"] = 12;
  d["current"]["time"] = 1793095200;
  d["current"]["weather_code"] = 2;
  d["current"]["is_day"] = 1;
  d["current_units"]["temperature_2m"] = "°C";
  d["hourly_units"]["temperature_2m"] = "°C";
  // 26 October 2026 00:00 UTC, past + seven days.
  const int64_t base = 1792972800;
  for (int i = 0; i < 8 * 24; ++i) {
    d["hourly"]["time"].add(base + i * 3600);
    d["hourly"]["temperature_2m"].add(i % 24 < 6 ? 7 : 16);
  }
  for (int i = 0; i < 8; ++i) {
    d["daily"]["time"].add(base + i * 86400);
    d["daily"]["temperature_2m_max"].add(16);
    d["daily"]["temperature_2m_min"].add(7);
    d["daily"]["weather_code"].add(2);
    d["daily"]["precipitation_probability_max"].add(25);
    d["daily"]["uv_index_max"].add(2);
    d["daily"]["sunrise"].add(base + i * 86400 + 7 * 3600);
    d["daily"]["sunset"].add(base + i * 86400 + 17 * 3600);
  }
  std::string raw;
  serializeJson(d, raw);
  return raw;
}
int main(int argc, char **argv) {
  Config c;
  std::string e;
  const std::string secrets = R"({"wifi_ssid":"Example","wifi_password":"password"})";
  const std::string location =
      R"({"latitude":47.3769,"longitude":8.5417,"label":"Zurich","timezone":"auto"})";
  CHECK(parseConfig(secrets, location, "", c, e));
  CHECK(c.forecastDays == 5 && !c.battery);
  CHECK(!parseConfig(secrets, location, R"({"forecast_days":4})", c, e));
  CHECK(!parseConfig(secrets, R"({"latitude":91,"longitude":8})", "", c, e));
  CHECK(!parseConfig(secrets, R"({"latitude":"47","longitude":8})", "", c, e));
  CHECK(!parseConfig(R"({"wifi_ssid":"","wifi_password":"password"})", location, "", c, e));
  CHECK(!parseConfig(secrets, location, R"({"forecast_days":3.5})", c, e));
  CHECK(!parseConfig(secrets, location, R"({"show_wind":"true"})", c, e));
  CHECK(!parseConfig(secrets, location, R"({"power_profile":"solar"})", c, e));
  CHECK(parseConfig(secrets, location,
                    R"({"forecast_days":3,"power_profile":"battery","show_wind":true})", c, e));
  CHECK(c.forecastDays == 3 && c.battery && c.wind);
  Config restored;
  CHECK(restoreConfig(configJson(c), restored, e));
  CHECK(configJson(c) == configJson(restored));
  CHECK(requestUrl(c).find("forecast_days=7") != std::string::npos);
  CHECK(requestUrl(c).find("apikey") == std::string::npos);
  auto originalKey = locationKey(c);
  c.latitude += 1;
  CHECK(locationKey(c) != originalKey);
  auto grid = gridBounds(7, 16);
  CHECK(grid.valid && grid.low == 5 && grid.high == 20);
  grid = gridBounds(-7, 3);
  CHECK(grid.low == -10 && grid.high == 5);
  grid = gridBounds(10, 10);
  CHECK(grid.low == 5 && grid.high == 15);
  grid = gridBounds(5, 20);
  CHECK(grid.low == 5 && grid.high == 20);
  CHECK(!gridBounds(NAN, 12).valid);
  CHECK(!gridBounds(20, 5).valid);
  CHECK(retryDelay(1) == 60 && retryDelay(2) == 300 && retryDelay(3) == 900);
  CHECK(retryAfter("120", 1793095200) == 120);
  CHECK(retryAfter("Tue, 27 Oct 2026 12:00:00 GMT", 1793095200) == 7200);
  CHECK(retryAfter("invalid", 1793095200) == 3600);
  CHECK(retryAfter("0", 1793095200) == 0);
  CHECK(advanceSchedule(1000, 500) == 1000);
  CHECK(advanceSchedule(1000, 1001) == 4600);
  CHECK(advanceSchedule(1000, 9000) == 11800);
  for (int code : {0,  1,  2,  3,  45, 48, 51, 53, 55, 56, 57, 61, 63, 65, 66,
                   67, 71, 73, 75, 77, 80, 81, 82, 85, 86, 95, 96, 97, 99})
    CHECK(std::string(weatherIcon(code)) != "unknown");
  CHECK(std::string(weatherIcon(0, false)) == "clear-night");
  CHECK(std::string(weatherIcon(999)) == "unknown");
  Weather w;
  auto raw = fixture();
  CHECK(parseWeather(raw, 1793095200, w, e));
  CHECK(applyTimezone(w));
  const int64_t now = w.hours[24 + 10].time;
  CHECK(localDate(now) == "2026-10-27");
  CHECK(dayHours(w, "2026-10-27").size() == 24);
  CHECK(futureDays(w, now, 5).size() == 5);
  CHECK(futureDays(w, now, 3).size() == 3);
  CHECK(futureDays(w, now, 5)[0].date == "2026-10-28");
  CHECK(overnightLow(w, now) == 7);
  CHECK(today(w, now) && today(w, now)->high == 16);
  auto missing = w;
  missing.hours[48].temperature = NAN;
  CHECK(std::isnan(overnightLow(missing, now)));
  CHECK(overnightLow(w, w.hours[24 + 3].time) == 7);
  CHECK(std::isnan(overnightLow(w, w.hours.back().time)));
  JsonDocument d;
  deserializeJson(d, raw);
  d["daily"]["uv_index_max"][1] = nullptr;
  std::string modified;
  serializeJson(d, modified);
  Weather nullable;
  CHECK(parseWeather(modified, now, nullable, e));
  CHECK(std::isnan(nullable.days[1].uv));
  d["hourly"]["temperature_2m"].remove(0);
  modified.clear();
  serializeJson(d, modified);
  CHECK(!parseWeather(modified, now, nullable, e));
  deserializeJson(d, raw);
  d["current_units"]["temperature_2m"] = "°F";
  modified.clear();
  serializeJson(d, modified);
  CHECK(!parseWeather(modified, now, nullable, e));
  deserializeJson(d, raw);
  d["hourly"]["time"][1] = d["hourly"]["time"][0];
  modified.clear();
  serializeJson(d, modified);
  CHECK(!parseWeather(modified, now, nullable, e));
  CHECK(!parseWeather("{}", now, nullable, e));
  CHECK(!parseWeather(std::string(MaxResponse + 1, 'x'), now, nullable, e));
  // Europe/Zurich spring/fall day lengths and overnight windows exercise DST.
  Weather zurich;
  zurich.posix = posixTimezone("Europe/Zurich");
  CHECK(!zurich.posix.empty());
  if (argc > 1 && std::string(argv[1]) == "--dst") {
    CHECK(applyTimezone(zurich));
    for (auto pair :
         {std::make_pair(int64_t(1774738800), 23), std::make_pair(int64_t(1792879200), 25)}) {
      Weather hours;
      hours.posix = zurich.posix;
      for (int i = 0; i < 72; ++i)
        hours.hours.push_back({pair.first - 86400 + i * 3600, 10});
      auto date = localDate(pair.first + 12 * 3600);
      CHECK(dayHours(hours, date).size() == size_t(pair.second));
      CHECK(overnightLow(hours, pair.first + 3600) == 10);
      hours.hours[24].temperature = NAN;
      CHECK(std::isnan(overnightLow(hours, pair.first + 3600)));
    }
  }
  if (argc > 2 && std::string(argv[1]) == "--live-fixture") {
    std::ifstream f(argv[2]);
    std::string payload((std::istreambuf_iterator<char>(f)), {});
    Weather live;
    CHECK(parseWeather(payload, 1793095200, live, e));
    CHECK(live.hours.size() >= 168 && live.days.size() >= 6);
    CHECK(applyTimezone(live));
    std::cout << "Live fixture: " << live.timezone << ", " << live.hours.size() << " hours\n";
  }
  std::cout << checks << " checks passed\n";
}
