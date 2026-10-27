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
  for (int days : {3, 5}) {
    config.forecastDays = days;
    screen.draw(config, &weather, ui, weather.currentTime);
    std::ofstream out("tests/build/classic-" + std::to_string(days) + ".pbm", std::ios::binary);
    out << "P4\n400 300\n";
    out.write((const char *)screen.buffer(), 15000);
  }
  for (auto view :
       {cpw::View::Menu, cpw::View::ForecastChoice, cpw::View::Status, cpw::View::Details}) {
    ui.view = view;
    ui.pending = 3;
    screen.draw(config, &weather, ui, weather.currentTime);
    std::ofstream out("tests/build/view-" + std::to_string(int(view)) + ".pbm", std::ios::binary);
    out << "P4\n400 300\n";
    out.write((const char *)screen.buffer(), 15000);
  }
  // Extreme temperatures must fit the hero region without changing icon proportions.
  weather.current = -42;
  ui.view = cpw::View::Home;
  screen.draw(config, &weather, ui, weather.currentTime);
  std::ofstream out("tests/build/negative.pbm", std::ios::binary);
  out << "P4\n400 300\n";
  out.write((const char *)screen.buffer(), 15000);
  std::cout << "Rendered both Classic views, menus/details, and negative temperature case\n";
}
