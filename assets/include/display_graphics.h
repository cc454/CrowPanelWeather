// SPDX-License-Identifier: MIT
#pragma once
#include <math.h>
#include <stdint.h>
namespace cpw_assets {
struct TemperatureGrid { float low; float high; bool valid; };
inline TemperatureGrid temperatureGrid(float minimum, float maximum) {
  if (!isfinite(minimum) || !isfinite(maximum) || minimum > maximum)
    return {0, 0, false};
  float low = floorf(minimum / 5.0f) * 5.0f;
  float high = ceilf(maximum / 5.0f) * 5.0f;
  if (low == high) { low -= 5; high += 5; }
  return {low, high, true};
}
// Coordinates describe the graph's inner plot area, excluding labels.
// Caller draws °C labels at each tick and location/date text in white.
template<class Display>
void drawTemperatureGrid(Display& display, int16_t x, int16_t y,
                         int16_t width, int16_t height,
                         TemperatureGrid grid, uint16_t ink) {
  if (!grid.valid || width < 2 || height < 2 || grid.high <= grid.low) return;
  for (float tick = grid.low; tick <= grid.high; tick += 5.0f) {
    int16_t row = y + lroundf((grid.high-tick) * (height-1) / (grid.high-grid.low));
    for (int16_t col = 0; col < width; col += 4)
      display.drawPixel(x+col, row, ink);
  }
}
template<class Display>
void drawClassicHeader(Display& display, uint16_t ink) {
  display.fillRect(0, 0, 400, 28, ink);
}
} // namespace cpw_assets
