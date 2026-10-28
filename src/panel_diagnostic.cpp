#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "panel.h"

static Panel panel;
static const char *revision = "GREEN";

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.printf("\nCrowPanel DISPLAY TEST v2: %s / software SPI\n", revision);
  Serial.println("No SD, Wi-Fi or saved configuration is used.");
  Serial.printf("Free heap: %u bytes\n", unsigned(ESP.getFreeHeap()));
  pinMode(2, INPUT_PULLUP);
  panel.begin();
  Serial.println("[test] Power enabled; preparing 400x300 test pattern");
  GFXcanvas1 frame(400, 300);
  if (!frame.getBuffer()) {
    Serial.println("[test] ERROR: cannot allocate image buffer");
    return;
  }
  frame.fillScreen(0);
  frame.drawRect(0, 0, 400, 300, 1);
  frame.fillRect(0, 0, 400, 55, 1);
  frame.setTextColor(0);
  frame.setTextSize(3);
  frame.setCursor(20, 16);
  frame.print("DISPLAY TEST");
  frame.setTextColor(1);
  frame.setCursor(20, 85);
  frame.print(revision);
  frame.setTextSize(2);
  frame.setCursor(20, 140);
  frame.print("400 x 300 / software SPI");
  frame.setCursor(20, 175);
  frame.print("No SD card required");
  frame.fillRect(20, 225, 80, 50, 1);
  frame.drawCircle(160, 250, 25, 1);
  frame.drawLine(220, 225, 375, 275, 1);
  bool ok = panel.show(frame.getBuffer());
  Serial.printf("[test] Driver returned %s. Does the screen show DISPLAY TEST?\n",
                ok ? "READY" : "ERROR");
  Serial.println("[test] READY alone does not confirm that the image appeared.");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 5000) {
    last = millis();
    Serial.printf("[test] Alive: %lu s, BUSY=%d, Menu=%d\n",
                  (unsigned long)(millis() / 1000), digitalRead(48), digitalRead(2));
  }
  delay(20);
}
