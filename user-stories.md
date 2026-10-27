# CrowPanelWeather user stories

Initial draft, 2026-10-27. See [requirements](requirements.md) for definitions, proposed defaults, constraints and provider sources.
Must = first version; Optional = enhancement. Stories describe future acceptance, not completed software.

| ID | Priority | User story | Acceptance criteria | Requirements |
| --- | --- | --- | --- | --- |
| US01 | Must | As a user, I want to put my Wi-Fi name and password in a file called secrets so the display can connect without changing code. | Valid microSD configuration connects after startup; wrong/missing credentials show useful guidance; no password appears on screen or in logs; no firmware rebuild is required. | R01, R13 |
| US02 | Must | As a user, I want to paste coordinates into a location file so I can choose the forecast location. | Valid latitude/longitude drive the request; invalid ranges are rejected; changes apply after restart or Reload; old-location cache is not displayed as the new location. | R02, R12 |
| US03 | Must | As a user, I want weather data without registering for an API key so setup remains simple. | Open-Meteo supplies current, hourly and daily data through validated HTTPS; no API credential is needed for the intended noncommercial use; attribution appears. | R03 |
| US04 | Must | As a user, I want current and perceived temperatures, today's high and a value labeled Low at a glance so I can plan what to wear. | All four values appear on the home screen with labels and units; Low retains the documented overnight window; incomplete overnight data is unavailable rather than misleading; a large current-condition symbol is visibly larger than the outlook symbols. | R04 |
| US05 | Must | As a user, I want a temperature curve for today so I can see when it gets warmer or colder. | Hourly values cover the local day, with time/temperature axes and now marker; missing points remain gaps; horizontal dotted lines at every 5°C enclose the day's range using floor/ceiling bounds; negative values, flat temperatures and daylight-saving days render correctly. | R05 |
| US06 | Must | As a user, I want a three-to-five-day outlook so I can plan ahead. | Five future dates by default, excluding today; 3/4/5 selectable; every day includes high, low, precipitation percentage and recognizable condition icon; dates roll over in the location's timezone. | R06 |
| US07 | Must | As a user, I want today's UV index so I can plan sun protection. | Home screen labels the value UV max; absent UV is shown as unavailable; it is never confused with a current UV measurement. | R07 |
| US08 | Optional | As a user, I want sunrise and sunset times so I can plan daylight activities. | Optional view uses location-local times; no-sunrise/no-sunset conditions show a meaningful placeholder. | R08 |
| US09 | Optional | As a user, I want wind information so I can judge outdoor comfort. | Optional view shows speed with units and direction; missing values show unavailable. | R08 |
| US10 | Must | As a user, I want automatic hourly updates so the forecast stays current without interaction. | Fetch at startup and every 60 minutes; no interval setting is exposed; the schedule remains stable over repeated updates and manual refreshes. | R09 |
| US11 | Must | As a user, I want to request an update so I can check the latest available forecast. | Menu Refresh starts one fetch, reports success/failure, coalesces repeated presses and observes the manual cooldown. | R10 |
| US12 | Must | As a user, I want to operate the app with its physical controls so I need no touchscreen or keyboard. | Menu opens settings; up/down moves; press selects; Exit cancels/returns; Boot/Reset retain hardware roles; focus is visible and unconfirmed changes can be discarded. | R14 |
| US13 | Must | As a user, I want to see whether the forecast is old so I do not mistake retained e-paper content for fresh data. | Location and last successful update are visible; failures do not overwrite good data or timestamp; stale indicator appears once data is older than 120 minutes; no-cache state shows no fabricated weather. | R11, R12 |
| US14 | Must | As a user, I want the device to recover from internet outages so I do not have to restart it. | Bounded retries/backoff; last good data retained; a later valid response restores normal status; rate-limit and malformed-response cases do not crash or loop rapidly. | R03, R11, R12 |
| US15 | Must | As a user, I want to use microSD for setup and keep working after successful setup if the card is absent. | Startup validates files; internal last-known-good config/cache is usable without the card; first boot without config gives instructions; interrupted saves do not destroy the only usable copy. | R13 |
| US16 | Must | As a user, I want USB and battery profiles so I can use the display on a desk or away from a socket. | USB profile supports ordinary interaction; battery profile wakes on schedule and sleeps after updates; actual wake/control and handover behavior is verified and documented before release. | R15 |
| US17 | Must | As a user, I want a readable dashboard on the 400×300 display so I can understand the forecast quickly. | Black header with white location/date text, large temperature and current symbol, curve and three-/five-day outlooks fit without clipping; icons remain distinguishable in monochrome; optional details do not crowd out required data; unknown weather codes have a fallback. | R04–R08, R16 |

## Release acceptance scenarios

1. Fresh device + valid microSD configuration: connect, resolve location-local time, fetch, render all required data and persist a valid cache.
2. Missing secrets, invalid JSON, bad coordinates, missing card: actionable message, no secret leakage and no silent wrong-location fallback.
3. Wrong Wi-Fi, unavailable service, HTTP error, timeout, invalid certificate or malformed response: retain good frame/cache, bounded retries, visible status.
4. Location change and midnight/daylight-saving boundaries: correct dates, cache identity, day curve and overnight window.
5. Missing UV, unknown weather code, flat curve, negative temperatures, 0%/100% precipitation and fewer forecast days returned: readable truthful output; missing days shown unavailable, never invented.
6. Controls: menu navigation, confirmation, cancellation, inactivity return and repeated Refresh presses behave as specified.
7. Power: repeated scheduled cycles on both profiles, reboot with cached data, storage interruption, and measured active/sleep current on the real board.
8. Layout: review three- and five-day mockups at 400×300, then the physical panel. Verify the black header, enlarged current symbol, Low label and dotted 5°C grid; test 7–16°C, −7–3°C, exact-multiple bounds and a flat 10°C day.

## Deferred stories

Alternative weather providers, OTA updates, browser configuration, alerts and additional sensors require a later scope decision. Battery percentage needs a verified measurement path and calibration. A battery-runtime target must be chosen before runtime can become a pass/fail criterion.
