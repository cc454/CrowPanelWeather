#include "core.h"
#include "storage.h"
#include "network.h"
#include "screen.h"
#include <WiFi.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <memory>

using namespace cpw;
static Screen screen;
static Config config;
static Weather weather;
static UI ui;
static bool configured = false, hasWeather = false, redraw = true, staleShown = false;
static uint32_t nextScheduled = 0, retryAt = 0, lastManual = 0, lastAction = 0;
static bool retryPending = false, manualUsed = false;
RTC_DATA_ATTR static unsigned failures = 0;
RTC_DATA_ATTR static uint32_t rtcMagic = 0;
RTC_DATA_ATTR static int64_t rtcScheduled = 0, rtcRetry = 0;
RTC_DATA_ATTR static int64_t rtcLastManual = 0;
static constexpr uint32_t Magic = 0x43505731;
static constexpr uint32_t HourMs = 3600000;
static bool reached(uint32_t value) { return int32_t(millis() - value) >= 0; }
static int64_t clockNow() {
  auto n = time(nullptr);
  return n > 1700000000 ? n : hasWeather ? weather.fetched : 0;
}
static void message(const std::string &s) {
  ui.message = s;
  Serial.println(s.c_str());
  redraw = true;
}
static bool persist(const Config &candidate) {
  if (!configured) {
    message("Load microSD configuration first");
    return false;
  }
  if (!saveConfiguration(candidate)) {
    message("Settings save failed");
    return false;
  }
  config = candidate;
  return true;
}
static void fetch(bool manual) {
  if (!configured) {
    message("Configure microSD first");
    return;
  }
  if (fetchRunning()) {
    message("Update already running");
    return;
  }
  if (retryPending && !reached(retryAt)) {
    message("Retry cooldown active");
    return;
  }
  if (manual && manualUsed && millis() - lastManual < 60000) {
    message("Wait 60s before Refresh");
    return;
  }
  if (manual && time(nullptr) > 1700000000 && rtcLastManual > 0 &&
      time(nullptr) - rtcLastManual < 60) {
    message("Wait 60s before Refresh");
    return;
  }
  if (reached(nextScheduled))
    nextScheduled += ((millis() - nextScheduled) / HourMs + 1) * HourMs;
  if (!startFetch(config)) {
    message("Cannot start update task");
    return;
  }
  if (manual) {
    lastManual = millis();
    manualUsed = true;
    if (time(nullptr) > 1700000000)
      rtcLastManual = time(nullptr);
  }
  ui.busy = true;
  message("Updating");
}
static void reload() {
  if (fetchRunning()) {
    message("Wait for update to finish");
    return;
  }
  Config candidate;
  std::string error;
  if (!loadConfiguration(candidate, true, error)) {
    message(error);
    return;
  }
  bool changed = !configured || locationKey(candidate) != locationKey(config);
  config = candidate;
  configured = true;
  ui.selectedDay = 0;
  if (changed) {
    hasWeather = loadCache(config, weather);
    if (hasWeather)
      applyTimezone(weather);
    retryPending = false;
    failures = 0;
  }
  ui.view = View::Home;
  message("Configuration reloaded");
  if (changed)
    fetch(false);
}
static void key(int pin) {
  lastAction = millis();
  redraw = true;
  if (pin == 2) {
    ui.view = View::Menu;
    ui.focus = 0;
    return;
  }
  if (pin == 1) {
    ui.view =
        ui.view == View::ForecastChoice || ui.view == View::PowerChoice ? View::Menu : View::Home;
    return;
  }
  int movement = pin == 6 ? -1 : pin == 4 ? 1 : 0;
  if (ui.view == View::Home || ui.view == View::Details) {
    if (movement)
      ui.selectedDay = (ui.selectedDay + movement + config.forecastDays) % config.forecastDays;
    if (pin == 5)
      ui.view = View::Details;
  } else if (ui.view == View::Menu) {
    if (movement)
      ui.focus = (ui.focus + movement + 7) % 7;
    if (pin != 5)
      return;
    switch (ui.focus) {
    case 0:
      ui.view = View::Home;
      fetch(true);
      break;
    case 1:
      ui.pending = config.forecastDays;
      ui.view = View::ForecastChoice;
      break;
    case 2:
      ui.pending = config.battery ? 1 : 0;
      ui.view = View::PowerChoice;
      break;
    case 3: {
      auto c = config;
      c.sun = !c.sun;
      persist(c);
      break;
    }
    case 4: {
      auto c = config;
      c.wind = !c.wind;
      persist(c);
      break;
    }
    case 5:
      reload();
      break;
    case 6:
      ui.view = View::Status;
      break;
    }
  } else if (ui.view == View::ForecastChoice || ui.view == View::PowerChoice) {
    bool isForecast = ui.view == View::ForecastChoice;
    if (movement)
      ui.pending = isForecast ? (ui.pending == 3 ? 5 : 3) : !ui.pending;
    if (pin == 5) {
      auto c = config;
      if (isForecast)
        c.forecastDays = ui.pending;
      else
        c.battery = ui.pending;
      if (persist(c)) {
        ui.selectedDay = 0;
        ui.view = View::Home;
        message("Settings saved");
      }
    }
  }
}
struct Button {
  int pin;
  bool last = true, stable = true;
  uint32_t changed = 0;
};
static Button buttons[] = {{2}, {1}, {6}, {4}, {5}};
static void pollButtons() {
  for (auto &b : buttons) {
    bool level = digitalRead(b.pin);
    if (level != b.last) {
      b.last = level;
      b.changed = millis();
    }
    if (level != b.stable && millis() - b.changed >= 35) {
      b.stable = level;
      if (!level)
        key(b.pin);
    }
  }
}
void setup() {
  Serial.begin(115200);
  setenv("TZ", "UTC0", 1);
  tzset();
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis(GPIO_NUM_41);
  gpio_hold_dis(GPIO_NUM_7);
  rtc_gpio_deinit(GPIO_NUM_2);
  for (auto &b : buttons) {
    pinMode(b.pin, INPUT_PULLUP);
    b.last = b.stable = digitalRead(b.pin);
  }
  screen.begin();
  beginStorage();
  std::string setupMessage;
  configured = loadConfiguration(config, false, setupMessage);
  if (configured) {
    hasWeather = loadCache(config, weather);
    if (hasWeather)
      applyTimezone(weather);
  }
  ui.message = setupMessage;
  auto wake = esp_sleep_get_wakeup_cause();
  int64_t now = time(nullptr);
  bool resume = rtcMagic == Magic &&
                (wake == ESP_SLEEP_WAKEUP_TIMER || wake == ESP_SLEEP_WAKEUP_EXT0) &&
                now > 1700000000;
  nextScheduled =
      millis() +
      uint32_t(resume ? std::max<int64_t>(0, std::min<int64_t>(Hour, rtcScheduled - now)) * 1000
                      : HourMs);
  if (resume && rtcRetry > 0) {
    retryPending = true;
    retryAt =
        millis() + uint32_t(std::max<int64_t>(0, std::min<int64_t>(604800, rtcRetry - now)) * 1000);
  }
  if (wake == ESP_SLEEP_WAKEUP_EXT0) {
    ui.view = View::Menu;
    ui.focus = 0;
  }
  if (configured && wake != ESP_SLEEP_WAKEUP_EXT0) {
    if (!resume || reached(nextScheduled) || (retryPending && reached(retryAt)))
      fetch(false);
  }
  lastAction = millis();
}
void loop() {
  pollButtons();
  // A fetch already in flight covers an hourly deadline crossed during that
  // request. Advance the schedule without immediately issuing a duplicate fetch.
  if (fetchRunning() && reached(nextScheduled))
    nextScheduled += ((millis() - nextScheduled) / HourMs + 1) * HourMs;
  std::unique_ptr<FetchResult> result(pollFetch());
  if (result) {
    ui.busy = false;
    std::string error = result->error;
    Weather candidate;
    if (error.empty() && !parseWeather(result->raw, result->fetched, candidate, error))
      result->error = error;
    if (error.empty()) {
      weather = std::move(candidate);
      hasWeather = true;
      applyTimezone(weather);
      failures = 0;
      retryPending = false;
      bool cached = saveCache(config, result->raw, result->fetched);
      message(cached ? "" : "Cache save failed");
    } else {
      ++failures;
      retryPending = true;
      uint32_t secs = std::max(retryDelay(failures), result->retryAfter);
      retryAt = millis() + secs * 1000;
      message(error);
    }
    staleShown = false;
    lastAction = millis();
    redraw = true;
  }
  if (configured && !fetchRunning() && (!retryPending || reached(retryAt)) &&
      (reached(nextScheduled) || (retryPending && reached(retryAt))))
    fetch(false);
  if (hasWeather && !staleShown && clockNow() - weather.fetched > 7200) {
    staleShown = true;
    redraw = true;
  }
  static std::string lastDate;
  auto date = localDate(clockNow());
  if (hasWeather && date != lastDate) {
    lastDate = date;
    redraw = true;
  }
  if (ui.view != View::Home && millis() - lastAction > 30000) {
    ui.view = View::Home;
    redraw = true;
  }
  if (redraw) {
    screen.draw(config, hasWeather ? &weather : nullptr, ui, clockNow());
    redraw = false;
  }
  if (configured && config.battery && !fetchRunning() && ui.view == View::Home &&
      millis() - lastAction > 30000 && digitalRead(2)) {
    const uint32_t tick = millis();
    auto remaining = [tick](uint32_t deadline) {
      return int32_t(tick - deadline) >= 0 ? uint32_t(0) : deadline - tick;
    };
    uint32_t wait = retryPending ? remaining(retryAt) : remaining(nextScheduled);
    wait = std::max<uint32_t>(1000, wait);
    rtcScheduled = time(nullptr) + remaining(nextScheduled) / 1000;
    rtcRetry = retryPending ? time(nullptr) + remaining(retryAt) / 1000 : 0;
    rtcMagic = Magic;
    screen.sleep();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    digitalWrite(7, LOW);
    gpio_hold_en(GPIO_NUM_41);
    gpio_hold_en(GPIO_NUM_7);
    gpio_deep_sleep_hold_en();
    rtc_gpio_pullup_en(GPIO_NUM_2);
    rtc_gpio_pulldown_dis(GPIO_NUM_2);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, 0);
    esp_sleep_enable_timer_wakeup(uint64_t(wait) * 1000);
    esp_deep_sleep_start();
  }
  delay(10);
}
