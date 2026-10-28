#include "screen.h"
#include "icon_table.h"
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#ifdef _WIN32
static tm *localtime_r(const time_t *value, tm *out) {
  return localtime_s(out, value) == 0 ? out : nullptr;
}
#endif

namespace cpw {
static std::string value(float n, bool degree = false) {
  if (!std::isfinite(n))
    return "--";
  return std::to_string(int(std::round(n))) + (degree ? std::string(1, char(248)) : "");
}
void Screen::text(int x, int y, const std::string &raw, int scale, bool white, int maxWidth) {
  std::string s;
  for (unsigned char ch : raw)
    s += ((ch >= 32 && ch < 127) || ch == 248) ? char(ch) : '?';
  size_t fit = std::max(0, maxWidth / (6 * scale));
  if (s.size() > fit) {
    s = s.substr(0, fit);
    if (fit > 2)
      s.replace(fit - 2, 2, "..");
  }
  frame.setFont(nullptr);
  frame.setTextSize(scale);
  frame.setTextColor(white ? 0 : 1);
  frame.setTextWrap(false);
  frame.cp437(true);
  frame.setCursor(x, y);
  frame.print(s.c_str());
}
void Screen::centered(int x, int y, const std::string &s, int scale) {
  text(x - int(s.size()) * 3 * scale, y, s, scale, false, 400);
}
void Screen::header(const Config &c, int64_t now) {
  frame.fillRect(0, 0, 400, 28, 1);
  text(10, 10, c.label, 1, true, 174);
  auto right = localTime(now, "%a %d %b %H:%M");
  text(390 - int(right.size()) * 6, 10, right, 1, true, 200);
}
void Screen::hero(const Weather &w) {
  text(12, 36, weatherLabel(w.code), 1, false, 188);
  GFXcanvas1 numerals(180, 60);
  numerals.setFont(&FreeSansBold24pt7b);
  numerals.setTextColor(1);
  numerals.setTextWrap(false);
  std::string digits = value(w.current);
  int16_t bx, by;
  uint16_t bw, bh;
  numerals.getTextBounds(digits.c_str(), 0, 0, &bx, &by, &bw, &bh);
  numerals.setCursor(-bx, -by);
  numerals.print(digits.c_str());
  float scale = std::min(48.0f / bh, 86.0f / bw);
  int width = std::round(bw * scale), height = std::round(bh * scale);
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      if (numerals.getPixel(std::min(int(bw) - 1, int(x / scale)),
                            std::min(int(bh) - 1, int(y / scale))))
        frame.drawPixel(12 + x, 58 + y, 1);
  frame.drawCircle(15 + width, 60, 2, 1);
  auto asset = findIcon(weatherIcon(w.code, w.isDay));
  // Normalized sources have exactly 48 ink rows, independent of canvas padding.
  int iconWidth = std::round(asset->currentWidth * height / 48.0f);
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < iconWidth; ++x) {
      int sx = x * asset->currentWidth / iconWidth, sy = y * 48 / height;
      if (pgm_read_byte(asset->current + sy * ((asset->currentWidth + 7) / 8) + sx / 8) &
          (0x80 >> (sx % 8)))
        frame.drawPixel(104 + x, 58 + y, 1);
    }
}
void Screen::graph(const Weather &w, int64_t now) {
  text(214, 36, "TODAY  C");
  auto hours = dayHours(w, localDate(now));
  float low = INFINITY, high = -INFINITY;
  for (auto h : hours)
    if (std::isfinite(h.temperature)) {
      low = std::min(low, h.temperature);
      high = std::max(high, h.temperature);
    }
  auto d = today(w, now);
  if (d) {
    if (std::isfinite(d->low))
      low = std::min(low, d->low);
    if (std::isfinite(d->high))
      high = std::max(high, d->high);
  }
  auto grid = gridBounds(low, high);
  if (!grid.valid || hours.empty()) {
    text(224, 98, "Unavailable");
    return;
  }
  const int left = 240, right = 385, top = 56, bottom = 163;
  auto y = [&](float t) {
    return bottom - int(std::round((t - grid.low) / (grid.high - grid.low) * (bottom - top)));
  };
  int lastLabel = -100;
  for (float tick = grid.high; tick >= grid.low; tick -= 5) {
    int row = y(tick);
    for (int x = left; x <= right; x += 4)
      frame.drawPixel(x, row, 1);
    // All gridlines remain; skip colliding interior text on unusually wide ranges.
    if (row - lastLabel >= 9 || tick == grid.low) {
      auto s = value(tick);
      text(left - 5 - int(s.size()) * 6, row - 3, s);
      lastLabel = row;
    }
  }
  time_t n = now;
  tm midnight{};
  localtime_r(&n, &midnight);
  midnight.tm_hour = midnight.tm_min = midnight.tm_sec = 0;
  midnight.tm_isdst = -1;
  int64_t start = mktime(&midnight);
  ++midnight.tm_mday;
  midnight.tm_isdst = -1;
  int64_t end = mktime(&midnight);
  auto x = [&](int64_t stamp) {
    return left + int((stamp - start) * double(right - left) / (end - start));
  };
  for (size_t i = 1; i < hours.size(); ++i)
    if (hours[i].time - hours[i - 1].time == Hour && std::isfinite(hours[i].temperature) &&
        std::isfinite(hours[i - 1].temperature))
      frame.drawLine(x(hours[i - 1].time), y(hours[i - 1].temperature), x(hours[i].time),
                     y(hours[i].temperature), 1);
  int nx = std::max(left, std::min(right, x(now)));
  for (int yy = top; yy < bottom; yy += 5)
    frame.drawFastVLine(nx, yy, 2, 1);
  // Local clock ticks are positioned by epoch, so DST days retain their real length.
  for (int hour : {0, 6, 12, 18, 24}) {
    tm tick{};
    time_t st = start;
    localtime_r(&st, &tick);
    tick.tm_hour = hour;
    tick.tm_isdst = -1;
    auto stamp = mktime(&tick);
    char label[4];
    snprintf(label, sizeof(label), "%02d", hour);
    int tx = std::min(right - 8, std::max(left, x(stamp) - 6));
    text(tx, 173, label);
  }
}
void Screen::home(const Config &c, const Weather &w, const UI &ui, int64_t now) {
  if (c.largeThree) {
    largeHome(c, w, ui, now);
    return;
  }
  hero(w);
  text(12, 120, "Feels " + value(w.feels, true), 2, false, 188);
  auto day = today(w, now);
  text(12, 145,
       "High " + value(day ? day->high : NAN, true) + "  Low " + value(overnightLow(w, now), true));
  text(12, 167, "UV max " + value(day ? day->uv : NAN));
  text(110, 167, "Rain " + value(day ? day->probability : NAN) + "%");
  frame.drawFastVLine(203, 37, 141, 1);
  graph(w, now);
  frame.drawFastHLine(10, 188, 380, 1);
  auto days = futureDays(w, now, c.forecastDays);
  for (int i = 0; i < c.forecastDays; ++i) {
    int x = 10 + (i * 380) / c.forecastDays, width = 380 / c.forecastDays, center = x + width / 2;
    if (i)
      frame.drawFastVLine(x, 196, 81, 1);
    if (i >= int(days.size())) {
      centered(center, 201, "--");
      centered(center, 247, "No data");
      continue;
    }
    const auto &dd = days[i];
    tm date{};
    int yr, mo, da;
    sscanf(dd.date.c_str(), "%d-%d-%d", &yr, &mo, &da);
    date.tm_year = yr - 1900;
    date.tm_mon = mo - 1;
    date.tm_mday = da;
    date.tm_hour = 12;
    date.tm_isdst = -1;
    time_t epoch = mktime(&date);
    centered(center, 198, localTime(epoch, "%a %d"));
    auto icon = findIcon(weatherIcon(dd.code));
    frame.drawBitmap(center - 12, 214, icon->forecast, 24, 24, 1);
    centered(center, 246, value(dd.high, true) + " / " + value(dd.low, true));
    centered(center, 264, value(dd.probability) + "%");
  }
  frame.drawFastHLine(0, 282, 400, 1);
  std::string status = ui.message;
  if (status.empty() && now - w.fetched > 2 * Hour)
    status = "STALE";
  if (ui.busy)
    status = "Updating";
  text(5, 289, "Weather: Open-Meteo " + localTime(w.fetched, "%d/%m %H:%M"), 1, false, 210);
  text(222, 289, status.empty() ? "Hourly  Menu" : status, 1, false, 175);
}
int Screen::readable(int x, int y, const std::string &s, bool bold, bool white, int maxWidth) {
  frame.setFont(bold ? &FreeSansBold12pt7b : &FreeSans9pt7b);
  frame.setTextSize(1);
  frame.setTextColor(white ? 0 : 1);
  frame.setTextWrap(false);
  std::string fitted = s;
  int16_t bx, by;
  uint16_t bw, bh;
  while (true) {
    frame.getTextBounds(fitted.c_str(), 0, 0, &bx, &by, &bw, &bh);
    if (bw <= maxWidth || fitted.empty())
      break;
    fitted.pop_back();
  }
  frame.setCursor(x - bx, y - by);
  frame.print(fitted.c_str());
  return bw;
}
void Screen::largeValue(int x, int y, float n, bool degree) {
  int width = readable(x, y, value(n), true);
  if (degree && std::isfinite(n))
    frame.drawCircle(x + width + 5, y + 3, 2, 1);
}
void Screen::largeHome(const Config &c, const Weather &w, const UI &ui, int64_t now) {
  // Two header rows replace the footer; last sync changes only after success.
  frame.fillRect(0, 0, 400, 32, 1);
  readable(10, 4, c.label, false, true, 165);
  text(214, 7, localTime(now, "%a %d %b %H:%M"), 1, true, 181);
  std::string status = ui.busy ? "Updating" : ui.message;
  if (status.empty() && now - w.fetched > 2 * Hour)
    status = "STALE";
  text(10, 22, status.empty() ? "Open-Meteo" : status, 1, true, 180);
  text(214, 22, "Sync " + localTime(w.fetched, "%d/%m %H:%M"), 1, true, 181);
  hero(w);
  text(12, 119, "Feels " + value(w.feels, true), 2, false, 126);
  auto day = today(w, now);
  readable(12, 148, "High");
  readable(75, 148, "Low");
  readable(141, 148, "Rain", false, false, 62);
  largeValue(12, 166, day ? day->high : NAN, true);
  largeValue(75, 166, overnightLow(w, now), true);
  readable(136, 166, value(day ? day->probability : NAN) + "%", true, false, 65);
  frame.drawFastVLine(207, 37, 147, 1);
  graph(w, now);
  frame.drawFastHLine(10, 191, 380, 1);
  const auto days = futureDays(w, now, 3);
  for (int i = 0; i < 3; ++i) {
    int x = 10 + i * 127;
    if (i)
      frame.drawFastVLine(x, 199, 96, 1);
    const auto &d = days[i];
    int yr, mo, da;
    tm date{};
    sscanf(d.date.c_str(), "%d-%d-%d", &yr, &mo, &da);
    date.tm_year = yr - 1900;
    date.tm_mon = mo - 1;
    date.tm_mday = da;
    date.tm_hour = 12;
    date.tm_isdst = -1;
    const auto label = localTime(mktime(&date), "%a %d");
    readable(x + 25, 200, label, false, false, 100);
    auto asset = findIcon(weatherIcon(d.code));
    frame.drawBitmap(x + 3, 228, asset->largeForecast, 48, 48, 1);
    text(x + 49, 225, "H");
    largeValue(x + 60, 221, d.high, true);
    text(x + 49, 254, "L");
    largeValue(x + 60, 250, d.low, true);
    readable(x + 45, 279, value(d.probability) + "%", true, false, 76);
  }
}
bool Screen::draw(const Config &c, const Weather *w, const UI &ui, int64_t now) {
  frame.fillScreen(0);
  header(c, now);
  if (ui.view == View::Home && w)
    home(c, *w, ui, now);
  else if (ui.view == View::Home) {
    text(12, 60, "No weather data", 2);
    text(12, 100, ui.message, 1, false, 376);
    text(12, 135, "Add secrets and location to microSD.");
    text(12, 155, "Menu > Reload configuration");
  } else if (ui.view == View::Menu) {
    text(12, 42, "MENU", 2);
    const char *rows[] = {"Refresh",      "Forecast view",        "Power profile", "Sun times",
                          "Wind details", "Reload configuration", "Status / help"};
    for (int i = 0; i < 7; ++i) {
      int y = 72 + i * 27;
      bool selected = ui.focus == i;
      if (selected)
        frame.fillRect(8, y - 4, 384, 24, 1);
      std::string label = rows[i];
      if (i == 1)
        label += c.largeThree          ? "  Large three"
                 : c.forecastDays == 3 ? "  Classic three"
                                       : "  Classic five";
      if (i == 2)
        label += c.battery ? "  Battery" : "  USB";
      if (i == 3)
        label += c.sun ? "  On" : "  Off";
      if (i == 4)
        label += c.wind ? "  On" : "  Off";
      text(15, y, label, 1, selected);
    }
  } else if (ui.view == View::ForecastChoice || ui.view == View::PowerChoice) {
    bool forecast = ui.view == View::ForecastChoice;
    text(12, 52, forecast ? "FORECAST VIEW" : "POWER PROFILE", 2);
    for (int i = 0; i < (forecast ? 3 : 2); ++i) {
      bool selected = ui.pending == i;
      int y = 94 + i * 42;
      if (selected)
        frame.fillRect(10, y - 6, 380, 32, 1);
      text(20, y,
           forecast ? (i == 2 ? "Large three"
                       : i    ? "Classic five"
                              : "Classic three")
                    : (i ? "Battery (sleep)" : "USB (awake)"),
           2, selected);
    }
    text(12, 224, "Up/down selects. Press confirms.");
    text(12, 246, "Exit cancels.");
  } else if (ui.view == View::Details) {
    auto ds = w ? futureDays(*w, now, c.forecastDays) : std::vector<Daily>{};
    if (ui.selectedDay < int(ds.size())) {
      auto &d = ds[ui.selectedDay];
      text(12, 48, d.date, 2);
      text(12, 81, weatherLabel(d.code), 2);
      text(12, 112, "High " + value(d.high, true) + "   Low " + value(d.low, true), 2);
      text(12, 144, "Precipitation chance " + value(d.probability) + "%");
      text(12, 166, "UV max " + value(d.uv));
      if (c.sun) {
        text(12, 190, "Sunrise " + localTime(d.sunrise));
        text(12, 210, "Sunset  " + localTime(d.sunset));
      }
    } else
      text(12, 70, "Forecast unavailable");
    if (c.wind && w)
      text(12, 237, "Current wind " + value(w->wind) + " km/h  " + value(w->direction) + " deg");
    text(12, 270, "Exit returns. Up/down changes day.");
  } else {
    text(12, 45, "STATUS / HELP", 2);
    text(12, 77, "Weather: Open-Meteo (model data)");
    text(12, 98, "open-meteo.com  CC BY 4.0");
    text(12, 119, "Automatic refresh: 60 minutes");
    text(12, 140, std::string("Power: ") + (c.battery ? "battery" : "USB"));
    text(12, 161, "Last success: " + (w ? localTime(w->fetched, "%d/%m %H:%M") : "none"));
    text(12, 182, "Low: overnight 18:00 to 06:00");
    text(12, 203, w ? w->timezone : "Timezone pending");
    text(12, 225, ui.message, 1, false, 376);
    text(12, 263, "Exit returns. Boot/Reset: hardware.");
  }
  bool ok = panel.show(frame.getBuffer());
  if (!ok)
    Serial.println("Display busy timeout: check panel build revision");
  return ok;
}
} // namespace cpw
