# CrowPanelWeather requirements

Status: implementation requirements, 2026-10-27. Initial firmware and installation instructions are now included; physical-board acceptance remains pending. See README.md for build/test status and limitations.
Hardware: Elecrow CrowPanel ESP32-S3 4.2-inch, monochrome 400×300; see [specs](specs). “Echo” is interpreted as Elecrow from the established project context.

## Goal and priorities

Provide a readable, unattended weather dashboard for one configured location, with simple file-based setup and physical-button navigation. Support both USB and battery use, as requested.

Must: Wi-Fi setup, coordinate configuration, keyless weather source, current and feels-like temperatures, today's high, tonight's low, today's temperature curve, a selectable three- or five-day outlook with high/low, precipitation probability and weather symbols, fixed 60-minute automatic updates plus manual refresh, UV information, and visible data freshness.
Optional: sunrise/sunset and wind. UV is treated as required because it was explicitly requested before also appearing in the optional list.

## Provider recommendation

Use Open-Meteo for version 1. Its free hosted API requires no key for noncommercial use; attribution is required. It supports the needed current, hourly and daily fields. See [service terms](https://open-meteo.com/) and [API documentation](https://open-meteo.com/en/docs).

| Provider | Access and fit | Scope |
| --- | --- | --- |
| Open-Meteo | No key for personal/noncommercial use; fits this project | Recommended first implementation |
| OpenWeather / OpenWeatherMap | Same service, requires an API key; complete feature coverage depends on product/subscription | Possible later adapter |
| Weather Underground / The Weather Company | Contributor API includes forecasts; access entitlement must be established | Consider only if user already has access |

OpenWeather's current [pricing](https://openweathermap.org/price) lists One Call 4.0 and marks 3.0 deprecated; do not adopt old reference-project endpoint instructions without verification. [Authentication](https://openweathermap.org/appid) requires a key. [Weather Underground access](https://www.wunderground.com/pws-api) is aimed at personal weather station contributors.

At one combined request per scheduled update, a 60-minute interval means 24 normal requests per day, excluding startup, retries and manual requests. Polling more frequently does not guarantee newly produced forecasts. Keep fetching/parsing separate from the display model, but implementing three providers is outside version 1.

## Functional requirements

| ID | Requirement and acceptance condition |
| --- | --- |
| R01 | Read SSID and password from a runtime file named exactly `secrets`; connect to a compatible 2.4 GHz Wi-Fi network without editing/rebuilding firmware. Missing or invalid credentials produce a useful setup message without displaying the password. |
| R02 | Read decimal latitude and longitude from a file named exactly `location`; accept ranges −90…90 and −180…180, reject missing/out-of-range values, never silently substitute another town. Optional label and timezone override are supported. |
| R03 | Fetch Open-Meteo over HTTPS with certificate validation. A fresh setup requires no weather account or API key. Handle time synchronization before validated HTTPS. |
| R04 | On the home screen show current temperature prominently, feels-like temperature, weather symbol, today's high and the overnight low labeled “Low”, with Celsius as the proposed default. Use a current-condition symbol (sun, rain, cloud, mixed conditions, etc.) alongside the large temperature. Its visible artwork height must match the rendered height of the temperature numerals (not the font's line box); keep its aspect ratio and align the symbol and numerals vertically. Both remain visibly larger than the daily outlook symbols. Label forecast-derived conditions rather than implying a physical sensor reading. |
| R05 | Show an hourly temperature curve for the configured location's current calendar day. Label time and °C axes, mark the current time, leave missing hours as gaps, and add horizontal dotted gridlines every 5°C. Round the day's minimum down to a multiple of 5°C and its maximum up to a multiple of 5°C so the full curve is enclosed; include and label both boundary lines. Use the whole local day's range, not the overnight Low metric. For a flat day exactly on a 5°C boundary, expand the range by 5°C on each side. Handle negative temperatures and unavailable values without clipping or a zero-range error. |
| R06 | Menu → Forecast view offers Classic three, Classic five and Large three. The outlook starts tomorrow in the location's timezone and reserves the next three or five consecutive local dates; today is never repeated and a missing date remains a dated unavailable tile. Default to Classic five. Up/down selects, press confirms and Exit cancels. Persist the confirmed choice across restart and redraw cached data without fetching or changing the hourly schedule. Each tile shows weekday/date, condition symbol, high/low in °C and precipitation probability in %. Large three uses larger dates, high/low values and precipitation percentages, with stacked high/low readings. |
| R07 | All three home layouts show today's precipitation probability labeled “Rain” with %. Use the provider's daily maximum precipitation probability; missing values show unavailable, never zero. Classic three/five additionally show today's maximum UV index labeled “UV max”. Large three omits UV and uses that third summary column for a larger Rain percentage alongside High and Low. A current UV reading may be added only if separately available and labeled. |
| R08 | Optional sunrise/sunset use location-local times and handle days without a sunrise/sunset. Optional wind shows speed with units and direction. |
| R09 | Fetch at startup and automatically every 60 minutes. The interval is fixed and is not configurable through files or menus. Manual Refresh (R10) provides an update between scheduled fetches and does not reset the hourly schedule. |
| R10 | Provide manual Refresh and Reload configuration menu actions. Coalesce repeated refresh requests and apply a proposed 60-second minimum between manual network fetches. |
| R11 | Always show location and last successful fetch date/time. Failed fetches retain the last good data, visibly indicate offline/error status and never advance the success timestamp. Mark data stale once older than 120 minutes (two hourly intervals). |
| R12 | Retain a validated cache across restarts. With no usable cache and no connection, show “No weather data” plus setup/error guidance. Never reuse another location's cached weather after coordinates change. |
| R13 | Reload configuration must remount and reread the microSD card, including a replacement not detected at boot. Files are root-level secrets (plural) and location, with precise filename/mount/JSON errors. Accept UTF-8 BOMs. Valid location changes update the label/coordinates and exclude old-location cache; changed credentials trigger reconnection. Invalid card data retains the last-known-good setup with a persistent visible warning, also available in Status / help alongside active coordinates. Successful reload clears the warning. |
| R14 | Menu, Exit and the up/down/press switch provide all ordinary interaction. Boot and Reset retain their hardware roles. No keyboard-style credential entry on the display. |
| R15 | Support explicit USB and battery operating profiles; USB remains responsive, battery profile sleeps between updates. Wake and button responsiveness during sleep require verification on the actual board before being promised. |
| R16 | Offer Classic three, Classic five and Large three on the 400×300 panel. Keep the black header, equal visible heights for current temperature and condition symbol, and today's dotted 5°C curve. All three show feels-like, High, Low, today's Rain %, provider attribution and freshness status; Classic layouts also show UV max. Large three uses approved option B: larger H/L labels and values left, condition icons right. It replaces UV with Rain and has no footer. Its single-line header has left-aligned location and right-aligned Open-Meteo, current date and parenthesized sync date/time; omit the current clock. Update/stale/error messages sit beneath Feels. All three omit forecast selection underlines. Failed fetches retain the successful sync timestamp. Classic layouts retain their footer. |
| R17 | Every header includes a Wi-Fi icon: connected at the last check, disconnected, or unknown before the first check. Sample at completion of existing weather-update attempts only; do not add polling or independent screen refreshes. Preserve the sample through battery sleep and show its timestamp in Status / help. The icon reports Wi-Fi association, not internet/API success. |

### Proposed configuration contract

Files are UTF-8 JSON on the microSD root; actual private files are never committed. Future firmware delivery must include sanitized `secrets.example`, `location.example`, `settings.example`, ignore rules, and clear copy-to-card instructions.

`secrets`:
```json
{"wifi_ssid":"YOUR_WIFI_NAME","wifi_password":"YOUR_WIFI_PASSWORD"}
```

`location` (example coordinates only, not a chosen user location):
```json
{"latitude":47.3769,"longitude":8.5417,"label":"Zurich","timezone":"auto"}
```

`settings`:
```json
{"forecast_days":5,"power_profile":"usb","show_sun_times":false,"show_wind":false}
```

Credentials are required; settings can fall back to documented defaults. `forecast_days` accepts only 3 or 5, selecting Classic three or Classic five respectively; other values are invalid. A confirmed menu choice is saved internally and takes precedence over the card value on later restarts. Explicit Reload configuration reapplies the validated card value and persists it; a reload with invalid settings retains the last-known-good configuration. Load all files at startup or explicit Reload configuration. Validate a complete candidate before replacing the last-known-good configuration; identify which file needs correction. Show the active power profile and fixed 60-minute interval in status. Do not attempt automatic USB/battery detection unless the hardware supports it reliably.

### Weather meanings — proposed decisions

- Current: provider's current conditions and associated timestamp.
- Today: local calendar date at the configured coordinates.
- Today's high: provider daily maximum for today.
- Low (home-screen label; replaces “Tonight” only): minimum hourly temperature from local 18:00 today up to, but excluding, 06:00 tomorrow. Before 06:00, use the night that began yesterday. Label the date/window in details. If hours are missing, show unavailable rather than presenting an incomplete minimum as complete. Fetch the previous day's hourly data when needed.
- Outlook high/low: daily calendar-day maximum/minimum, distinct from the overnight metric.
- Precipitation %: daily maximum hourly probability, labeled “precip. chance”; it is neither rainfall amount nor a sum of hourly percentages.
- Symbols: consistent mapping for clear, partly cloudy, cloudy, fog, drizzle, rain, freezing precipitation, snow, showers and thunderstorms; unknown codes use an explicit fallback.
- Time: resolve timezone from coordinates, including daylight-saving transitions. Do not use the device owner's timezone when the weather location differs. Today's chart supports 23/25-hour local days.

Open-Meteo field plan: current `temperature_2m`, `apparent_temperature`, `weather_code`; hourly `temperature_2m`; daily `temperature_2m_max`, `temperature_2m_min`, `weather_code`, `precipitation_probability_max`, `uv_index_max`. Optional daily `sunrise`, `sunset`, current `wind_speed_10m`, `wind_direction_10m`. Request enough hourly/daily coverage for tonight and five future dates, with timezone resolution. Validate these against the API during implementation.

## Screen and controls

Selected layouts: Classic three, Classic five and Large three at 400×300. All use a black header, equally tall current temperature/symbol, today's curve and Rain %. Classic layouts retain UV max and a footer. Large three uses approved option B with temperatures/larger H/L labels left and icons right. Its single header line shows location left and provider/current date/sync date-time right, with no current clock or footer. Rain replaces UV; status messages appear under Feels. Forecast tiles have no underline. Classic five remains the default.

| Control | Proposed behavior |
| --- | --- |
| Menu | Open menu |
| Switch up/down | Move focus, change selected value, or select forecast day |
| Switch press | Select/confirm; open selected day details |
| Exit | Cancel pending edits/back; return from menu to dashboard |
| Reset | Hardware restart, preserve saved configuration |
| Boot | Firmware download/startup role; no weather-app action |

Menus include Refresh, Forecast view (Classic three / Classic five), power profile, optional fields, Reload configuration, and status/help. Highlight focus, debounce inputs, avoid multiple simultaneous refreshes, and return to home after a proposed 30 seconds of inactivity, discarding unconfirmed edits. Menu navigation should use cached data rather than fetching on every press.

## Reliability, privacy and display quality

- Preserve a complete valid frame during network failures; validate data before updating the display.
- Distinguish Wi-Fi, DNS/connection, time-sync, HTTP/rate-limit and invalid-response failures. Use bounded timeouts and backoff; honor server retry guidance and avoid retry storms. Proposed retry delays: 1, 5, then 15 minutes, capped at the normal interval.
- Do not log credentials or API keys, display them, or publish them in example files. Removable-card storage is plaintext and is not encrypted credential protection.
- E-paper refresh policy must suit the identified panel revision; verify supported partial refresh and periodic full refresh on hardware. No second-by-second animations or clock redraws.
- Test with a real 400×300 frame: no clipping, distinguishable symbols, readable negative and two-digit temperatures, dates, units and percentages. Validate both Classic layouts with the symbol matching the temperature numerals' visible height and the 5°C dotted graph grid. Verify clear, rain, cloud, mixed and snow artwork at that shared height, preserving aspect ratio and avoiding overlap. Five columns and the curve must coexist before the layout is accepted.
- Show “Weather: Open-Meteo” on the dashboard and provide the full attribution link in help/documentation.
- Bound response/cache sizes to available memory and recover gracefully from malformed files, missing values, interrupted writes and missing storage.
- Firmware and the standalone screen test support only the identified green-sticker panel. Keep its initialization sequence and fixed GPIOs; other 4.2-inch controllers are outside scope.

## Battery and USB requirements

User selected both power sources. [Elecrow's wiki](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-paper_4.2-inch_HMI_Display.html) specifies a 3.7 V lithium battery, SH1.0 2-pin connector and onboard charging. It does not specify a minimum Ah/mAh capacity. The product page's 2.2–3.7 V wording is insufficient to establish charging limits.

Before selecting a battery, verify the actual board revision, charger specification/current, permitted cell charge voltage, connector polarity and battery charge/discharge ratings. A capacity alone does not establish compatibility. A nominal 3.7 V rechargeable lithium cell typically reaches a higher voltage at full charge; confirm this against the actual charging circuit rather than treating 3.7 V as a proven absolute maximum.

Minimum capacity and runtime remain undetermined until current is measured. Estimate capacity in Ah as average battery current in A × desired hours, with allowance for usable capacity and losses. Average current must include Wi-Fi, display refresh, idle/sleep and storage. E-paper image retention does not imply zero consumption for the whole board.

Acceptance: scheduled wake/fetch/render/sleep works from battery; USB operation is stable; buttons and wake behavior are documented; no unverified battery percentage or runtime claim. Seamless USB/battery handover and in-use charging behavior remain hardware-validation items. Battery-runtime target is still to be chosen.

## References and boundaries

- [echo-cool/e-paper-weather](https://github.com/echo-cool/e-paper-weather): directly relevant CrowPanel 4.2-inch implementation using Open-Meteo; inspiration for driver integration and dense weather layout. Its always-on/web setup does not replace this project's file-based setup and battery goals.
- [viixxiiv/Crowpanel-Weather-ESP](https://github.com/viixxiiv/Crowpanel-Weather-ESP): second user reference, confirmed by name. Its README contains a 4.2-inch layout and older OpenWeatherMap instructions; treat those as historical, not current API guidance. Repository metadata's Weather Underground description differs from its README.
- Before reusing source code, fonts or icons, inspect applicable licenses and preserve required notices.

Version 1 excludes OTA, web setup, touch interaction, air quality, alerts, indoor sensors and multiple locations. These may be proposed later rather than inherited automatically from references.

Graph examples: 7–16°C uses gridlines at 5, 10, 15 and 20°C; −7–3°C uses −10, −5, 0 and 5°C. Exact multiples remain bounds unless the entire day is flat. Missing hours stay gaps; unavailable ranges show an unavailable chart rather than invented ticks.

Review decisions still open: exact overnight definition (the requested Low rename does not change the existing draft definition), final pixel sizing within the selected Classic layouts, battery runtime target and validated hardware/power behavior. Proposed defaults above make the draft actionable without claiming user approval.
