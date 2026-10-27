#include "network.h"
#include "weather_ca.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <memory>

namespace cpw {
static QueueHandle_t results = nullptr;
static bool running = false;
static void fetchTask(void *argument) {
  std::unique_ptr<Config> c(static_cast<Config *>(argument));
  auto *result = new FetchResult();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if (WiFi.status() != WL_CONNECTED)
    WiFi.begin(c->ssid.c_str(), c->password.c_str());
  uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 25000)
    vTaskDelay(pdMS_TO_TICKS(100));
  if (WiFi.status() != WL_CONNECTED)
    result->error = "Wi-Fi connection failed";
  else {
    configTzTime(getenv("TZ") ? getenv("TZ") : "UTC0", "pool.ntp.org", "time.google.com");
    started = millis();
    while (time(nullptr) < 1700000000 && millis() - started < 15000)
      vTaskDelay(pdMS_TO_TICKS(100));
    if (time(nullptr) < 1700000000)
      result->error = "Time sync failed; TLS not attempted";
    else {
      WiFiClientSecure client;
      client.setCACert(kWeatherCA);
      client.setHandshakeTimeout(12);
      HTTPClient http;
      http.setConnectTimeout(10000);
      http.setTimeout(12000);
      http.useHTTP10(true);
      const char *headers[] = {"Retry-After"};
      http.collectHeaders(headers, 1);
      if (!http.begin(client, requestUrl(*c).c_str()))
        result->error = "Cannot start HTTPS request";
      else {
        http.addHeader("Accept-Encoding", "identity");
        int code = http.GET();
        if (code != HTTP_CODE_OK) {
          result->error = code < 0 ? "HTTPS / DNS / certificate connection failed"
                                   : "Weather HTTP " + std::to_string(code);
          if (code == 429 || code == 503)
            result->retryAfter = retryAfter(http.header("Retry-After").c_str(), time(nullptr));
        } else if (http.getSize() > int(MaxResponse))
          result->error = "Weather response too large";
        else {
          WiFiClient *stream = http.getStreamPtr();
          int remaining = http.getSize();
          result->raw.reserve(24000);
          uint32_t idle = millis();
          while ((http.connected() || stream->available()) && remaining != 0) {
            int available = stream->available();
            if (available) {
              char buf[512];
              size_t limit = std::min(size_t(available), sizeof(buf));
              if (remaining > 0)
                limit = std::min(limit, size_t(remaining));
              size_t n = stream->readBytes(buf, limit);
              result->raw.append(buf, n);
              if (remaining > 0)
                remaining -= n;
              idle = millis();
              if (result->raw.size() > MaxResponse) {
                result->error = "Weather response too large";
                break;
              }
            } else {
              if (millis() - idle > 12000) {
                result->error = "Weather read timed out";
                break;
              }
              vTaskDelay(pdMS_TO_TICKS(10));
            }
          }
          if (remaining > 0 && result->error.empty())
            result->error = "Incomplete weather response";
          if (result->raw.empty() && result->error.empty())
            result->error = "Empty weather response";
          if (result->error.empty())
            result->fetched = time(nullptr);
        }
        http.end();
      }
    }
  }
  if (!result->error.empty())
    result->raw.clear();
  xQueueSend(results, &result, portMAX_DELAY);
  vTaskDelete(nullptr);
}
bool startFetch(const Config &c) {
  if (running)
    return false;
  if (!results)
    results = xQueueCreate(1, sizeof(FetchResult *));
  if (!results)
    return false;
  Config *snapshot = new Config(c);
  running = true;
  if (xTaskCreate(fetchTask, "weather-fetch", 16384, snapshot, 1, nullptr) != pdPASS) {
    delete snapshot;
    running = false;
    return false;
  }
  return true;
}
bool fetchRunning() { return running; }
FetchResult *pollFetch() {
  FetchResult *r = nullptr;
  if (results && xQueueReceive(results, &r, 0) == pdTRUE) {
    running = false;
    return r;
  }
  return nullptr;
}
} // namespace cpw
