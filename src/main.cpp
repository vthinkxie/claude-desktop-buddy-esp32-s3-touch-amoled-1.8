#include <Arduino.h>
#include <Wire.h>
#include "hw/pins.h"
#include "hw/expander.h"
#include "hw/display.h"
#include "hw/border.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== SMOKE 1: display ===");

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (!hwExpanderInit())     { Serial.println("FAIL expander"); while (1) delay(1000); }
  hwExpanderResetSequence();
  if (!hwDisplayInit())      { Serial.println("FAIL display");  while (1) delay(1000); }

  Serial.println("OK init");
}

void loop() {
  static uint32_t frame = 0;
  Arduino_Canvas* c = hwCanvas();
  c->fillScreen(0x0000);
  c->setTextColor(0xFFFF);
  c->setTextSize(3);
  c->setCursor(20, 80);
  c->print("HELLO");
  c->setTextSize(1);
  c->setCursor(20, 130);
  c->printf("frame %lu", frame++);

  // Toggle border alert every 60 frames so we see it works
  hwBorderAlert((frame / 60) % 2 == 0);

  uint32_t t0 = millis();
  hwDisplayPush();
  uint32_t dt = millis() - t0;
  if (frame % 30 == 0) Serial.printf("push %lums\n", dt);

  delay(16);
}
