# Third-party notices

Application source: MIT unless stated otherwise. Original artwork: CC0-1.0. Dependencies retain their own licenses.

- Adafruit GFX 1.11.11: rendering, compact font and bundled FreeSansBold24pt7b data. https://github.com/adafruit/Adafruit-GFX-Library — BSD; see `licenses/Adafruit-GFX.txt`. FreeSans originates from GNU FreeFont; upstream font licensing/embedding terms: https://www.gnu.org/software/freefont/license.html. Font data is obtained through the unmodified dependency rather than copied into this repository.
- Adafruit BusIO 1.17.0: https://github.com/adafruit/Adafruit_BusIO — MIT; `licenses/Adafruit-BusIO.txt`.
- ArduinoJson 7.3.1: https://github.com/bblanchon/ArduinoJson — MIT; `licenses/ArduinoJson.txt`.
- Arduino-ESP32 / ESP-IDF: runtime, Wi-Fi, TLS, NVS, filesystem and sleep. https://github.com/espressif/arduino-esp32 — LGPL-2.1 and component licenses; https://github.com/espressif/esp-idf — Apache-2.0 and component licenses. These are installed by PlatformIO.
- nayarsystems/posix_tz_db: generated timezone table. https://github.com/nayarsystems/posix_tz_db — MIT; `licenses/posix_tz_db.txt`.
- Open-Meteo: https://open-meteo.com/ and https://open-meteo.com/en/docs — weather data attribution, CC BY 4.0.

The original transport uses hardware register/command information from Elecrow's green-sticker example:

- https://github.com/Elecrow-RD/CrowPanel-ESP32-4.2-E-paper-HMI-Display-with-400-300/tree/master/example/arduino_A_green_circular_sticker_on_the_back/Examples/4.2_Example7_Global_Refresh

The renderer and transport were written for this project. Source from the GPL-licensed echo-cool and viixxiiv apps was not incorporated; they remain design/hardware references.
