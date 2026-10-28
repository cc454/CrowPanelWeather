#include "screen.h"
#include <fstream>
#include <iterator>
#include <iostream>
void Panel::begin() {}
bool Panel::show(const uint8_t *) { return true; }
void Panel::sleep() {}
int main(int argc, char **argv) {
  if (argc != 2)
    return 1;
  std::ifstream fixture(argv[1]);
  std::string raw((std::istreambuf_iterator<char>(fixture)), {});
  cpw::Weather weather;
  std::string error;
  if (!cpw::parseWeather(raw, 1793095200, weather, error)) {
    std::cerr << error;
    return 1;
  }
  weather.fetched = weather.currentTime;
  cpw::applyTimezone(weather);
  cpw::Screen screen;
  cpw::Config config;
  config.label = "Zurich";
  cpw::UI ui;
  ui.wifi = cpw::WifiState::Connected;
  ui.wifiCheckedAt = weather.currentTime;
  for (int days : {3, 5}) {
    config.forecastDays = days;
    screen.draw(config, &weather, ui, weather.currentTime);
    std::ofstream out("tests/build/classic-" + std::to_string(days) + ".pbm", std::ios::binary);
    out << "P4\n400 300\n";
    out.write((const char *)screen.buffer(), 15000);
  }
  config.forecastDays = 3;
  config.largeThree = true;
  screen.draw(config, &weather, ui, weather.currentTime);
  {
    std::ofstream out("tests/build/large-3.pbm", std::ios::binary);
    out << "P4\n400 300\n";
    out.write((const char *)screen.buffer(), 15000);
  }
  for (auto view :
       {cpw::View::Menu, cpw::View::ForecastChoice, cpw::View::Status, cpw::View::Details}) {
    ui.view = view;
    ui.pending = 2;
    screen.draw(config, &weather, ui, weather.currentTime);
    std::ofstream out("tests/build/view-" + std::to_string(int(view)) + ".pbm", std::ios::binary);
    out << "P4\n400 300\n";
    out.write((const char *)screen.buffer(), 15000);
  }
  for (auto state : {cpw::WifiState::Unknown, cpw::WifiState::Disconnected}) {
    ui.view = cpw::View::Home;
    ui.wifi = state;
    screen.draw(config, &weather, ui, weather.currentTime);
    std::ofstream out("tests/build/wifi-" + std::to_string(int(state)) + ".pbm", std::ios::binary);
    out << "P4\n400 300\n";
    out.write((const char *)screen.buffer(), 15000);
  }
  ui.wifi = cpw::WifiState::Connected;
  // Header must reserve space for provider/date/sync with a long location.
  ui.view = cpw::View::Home;
  config.label = "Long location name near the weather board";
  ui.message = "HTTPS / DNS / certificate connection failed";
  screen.draw(config, &weather, ui, weather.currentTime + 3 * cpw::Hour);
  {
    std::ofstream out("tests/build/large-3-header-status.pbm", std::ios::binary);
    out << "P4\n400 300\n";
    out.write((const char *)screen.buffer(), 15000);
  }
  config.label = "Zurich";
  ui.message.clear();
  ui.configIssue = "Using saved setup; microSD: missing /location in card root";
  screen.draw(config, &weather, ui, weather.currentTime);
  {
    std::ofstream out("tests/build/config-warning.pbm", std::ios::binary);
    out << "P4\n400 300\n";
    out.write((const char *)screen.buffer(), 15000);
  }
  ui.configIssue.clear();
  // Extreme temperatures must fit the hero region without changing icon proportions.
  weather.current = -42;
  ui.view = cpw::View::Home;
  screen.draw(config, &weather, ui, weather.currentTime);
  std::ofstream out("tests/build/negative.pbm", std::ios::binary);
  out << "P4\n400 300\n";
  out.write((const char *)screen.buffer(), 15000);
  weather.days[1].probability = 100;
  weather.days[2].high = -100;
  weather.days[2].low = -100;
  screen.draw(config, &weather, ui, weather.currentTime);
  std::ofstream edge("tests/build/large-3-extremes.pbm", std::ios::binary);
  edge << "P4\n400 300\n";
  edge.write((const char *)screen.buffer(), 15000);
  std::cout << "Rendered all three views, menus/details, and extreme temperatures/rain\n";
}
