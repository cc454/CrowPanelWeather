#pragma once
#include <cstring>
#include "assets/include/icons_current.h"
#include "assets/include/icons_24.h"
struct IconAsset { const char* name; const uint8_t* current; int currentWidth; const uint8_t* forecast; };
static const IconAsset kIcons[] = {
  {"clear-day", cpw_assets::clear_day_current, cpw_assets::clear_day_current_width, cpw_assets::clear_day_24},
  {"clear-night", cpw_assets::clear_night_current, cpw_assets::clear_night_current_width, cpw_assets::clear_night_24},
  {"drizzle", cpw_assets::drizzle_current, cpw_assets::drizzle_current_width, cpw_assets::drizzle_24},
  {"fog", cpw_assets::fog_current, cpw_assets::fog_current_width, cpw_assets::fog_24},
  {"freezing-drizzle", cpw_assets::freezing_drizzle_current, cpw_assets::freezing_drizzle_current_width, cpw_assets::freezing_drizzle_24},
  {"freezing-rain", cpw_assets::freezing_rain_current, cpw_assets::freezing_rain_current_width, cpw_assets::freezing_rain_24},
  {"hail-thunderstorm", cpw_assets::hail_thunderstorm_current, cpw_assets::hail_thunderstorm_current_width, cpw_assets::hail_thunderstorm_24},
  {"heavy-rain", cpw_assets::heavy_rain_current, cpw_assets::heavy_rain_current_width, cpw_assets::heavy_rain_24},
  {"heavy-rain-showers", cpw_assets::heavy_rain_showers_current, cpw_assets::heavy_rain_showers_current_width, cpw_assets::heavy_rain_showers_24},
  {"heavy-snow", cpw_assets::heavy_snow_current, cpw_assets::heavy_snow_current_width, cpw_assets::heavy_snow_24},
  {"heavy-thunderstorm", cpw_assets::heavy_thunderstorm_current, cpw_assets::heavy_thunderstorm_current_width, cpw_assets::heavy_thunderstorm_24},
  {"mainly-clear-day", cpw_assets::mainly_clear_day_current, cpw_assets::mainly_clear_day_current_width, cpw_assets::mainly_clear_day_24},
  {"mainly-clear-night", cpw_assets::mainly_clear_night_current, cpw_assets::mainly_clear_night_current_width, cpw_assets::mainly_clear_night_24},
  {"overcast", cpw_assets::overcast_current, cpw_assets::overcast_current_width, cpw_assets::overcast_24},
  {"partly-cloudy-day", cpw_assets::partly_cloudy_day_current, cpw_assets::partly_cloudy_day_current_width, cpw_assets::partly_cloudy_day_24},
  {"partly-cloudy-night", cpw_assets::partly_cloudy_night_current, cpw_assets::partly_cloudy_night_current_width, cpw_assets::partly_cloudy_night_24},
  {"rain", cpw_assets::rain_current, cpw_assets::rain_current_width, cpw_assets::rain_24},
  {"rain-showers-day", cpw_assets::rain_showers_day_current, cpw_assets::rain_showers_day_current_width, cpw_assets::rain_showers_day_24},
  {"rain-showers-night", cpw_assets::rain_showers_night_current, cpw_assets::rain_showers_night_current_width, cpw_assets::rain_showers_night_24},
  {"rime-fog", cpw_assets::rime_fog_current, cpw_assets::rime_fog_current_width, cpw_assets::rime_fog_24},
  {"snow", cpw_assets::snow_current, cpw_assets::snow_current_width, cpw_assets::snow_24},
  {"snow-grains", cpw_assets::snow_grains_current, cpw_assets::snow_grains_current_width, cpw_assets::snow_grains_24},
  {"snow-showers", cpw_assets::snow_showers_current, cpw_assets::snow_showers_current_width, cpw_assets::snow_showers_24},
  {"thunderstorm", cpw_assets::thunderstorm_current, cpw_assets::thunderstorm_current_width, cpw_assets::thunderstorm_24},
  {"unknown", cpw_assets::unknown_current, cpw_assets::unknown_current_width, cpw_assets::unknown_24},
};
inline const IconAsset* findIcon(const char* name) {
  for (const auto& icon : kIcons) if (strcmp(name,icon.name)==0) return &icon;
  for (const auto& icon : kIcons) if (strcmp("unknown",icon.name)==0) return &icon;
  return &kIcons[0];
}
