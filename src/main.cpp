#include <Arduino.h>
#include <Wire.h>
#include "hw/pins.h"
#include "hw/expander.h"
#include "hw/display.h"
#include "hw/power.h"
#include "hw/input.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== SMOKE 2: input ===");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (!hwExpanderInit())     { Serial.println("FAIL expander"); while (1) delay(1000); }
  hwExpanderResetSequence();
  if (!hwDisplayInit())      { Serial.println("FAIL display");  while (1) delay(1000); }
  if (!hwPowerInit())        { Serial.println("FAIL power");    while (1) delay(1000); }
  if (!hwInputInit())        { Serial.println("FAIL input");    while (1) delay(1000); }

  Serial.println("OK init");
}

void loop() {
  hwInputUpdate();

  if (hwBtnA().wasPressed)   Serial.println("Key1 down");
  if (hwBtnA().wasReleased)  Serial.println("Key1 up");
  if (hwBtnB().wasPressed)   Serial.println("Key3 short");
  uint8_t e = hwAxpBtnEvent();
  if (e == 0x04)             Serial.println("Key3 long");

  const HwTouch& t = hwTouch();
  if (t.justPressed)         Serial.printf("touch @ (%d,%d)\n", t.x, t.y);
  if (t.justReleased)        Serial.println("touch up");

  // Render a tiny status so we can see the touch coord live
  Arduino_Canvas* c = hwCanvas();
  c->fillScreen(0x0000);
  c->setTextColor(0xFFFF);
  c->setCursor(4, 4);
  c->printf("touch:%c x=%d y=%d", t.down ? 'Y' : 'n', t.x, t.y);
  c->setCursor(4, 16);
  c->printf("Key1:%c", hwBtnA().isPressed ? 'D' : 'u');
  hwDisplayPush();

  delay(16);
}
