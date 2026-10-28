#pragma once
#include "core.h"
#include "panel.h"
#include <Adafruit_GFX.h>
namespace cpw {
enum class View { Home, Menu, ForecastChoice, PowerChoice, Details, Status };
enum class WifiState { Unknown, Connected, Disconnected };
struct UI {
  View view = View::Home;
  int focus = 0, selectedDay = 0, pending = 1;
  bool busy = false;
  std::string message;
  std::string configIssue;
  WifiState wifi = WifiState::Unknown;
  int64_t wifiCheckedAt = 0;
};
class Screen {
public:
  Screen() : frame(400, 300) {}
  void begin() { panel.begin(); }
  bool draw(const Config &c, const Weather *w, const UI &ui, int64_t now);
  void sleep() { panel.sleep(); }
  const uint8_t *buffer() { return frame.getBuffer(); }

private:
  GFXcanvas1 frame;
  Panel panel;
  void text(int x, int y, const std::string &s, int scale = 1, bool white = false,
            int maxWidth = 390);
  void centered(int x, int y, const std::string &s, int scale = 1);
  void header(const Config &c, const Weather *w, const UI &ui, int64_t now);
  void wifiIcon(int x, WifiState state);
  void home(const Config &c, const Weather &w, const UI &ui, int64_t now);
  void hero(const Weather &w);
  void largeHome(const Config &c, const Weather &w, const UI &ui, int64_t now);
  int readable(int x, int y, const std::string &s, bool bold = false, bool white = false,
               int maxWidth = 390);
  void largeValue(int x, int y, float value, bool degree = false);
  void graph(const Weather &w, int64_t now);
};
} // namespace cpw
