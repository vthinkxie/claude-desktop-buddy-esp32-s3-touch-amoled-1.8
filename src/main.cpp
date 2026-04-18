#include <Arduino.h>
#include <Wire.h>
#include "hw/pins.h"
#include "hw/expander.h"
#include "hw/display.h"
#include "hw/power.h"
#include "hw/input.h"
#include "hw/imu.h"
#include "hw/rtc.h"
#include "hw/audio.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== SMOKE T18: audio beep ===");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (!hwExpanderInit())     { Serial.println("FAIL expander"); while (1) delay(1000); }
  hwExpanderResetSequence();
  if (!hwDisplayInit())      { Serial.println("FAIL display");  while (1) delay(1000); }
  if (!hwPowerInit())        { Serial.println("FAIL power");    while (1) delay(1000); }
  if (!hwInputInit())        { Serial.println("FAIL input");    while (1) delay(1000); }
  if (!hwImuInit())          { Serial.println("FAIL imu");      while (1) delay(1000); }
  if (!hwRtcInit())          { Serial.println("FAIL rtc");      while (1) delay(1000); }
  if (!hwAudioInit())        { Serial.println("FAIL audio");    while (1) delay(1000); }

  Serial.println("OK init — beeping in 1s");
  delay(1000);
  Serial.println("beep 1: 1200 Hz / 80ms");
  hwBeep(1200, 80);
  delay(400);
  Serial.println("beep 2: 2400 Hz / 60ms");
  hwBeep(2400, 60);
  delay(400);
  Serial.println("beep 3: 800 Hz / 200ms");
  hwBeep(800, 200);
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last > 3000) {
    last = millis();
    Serial.println("loop beep");
    hwBeep(1500, 100);
  }
  delay(50);
}
