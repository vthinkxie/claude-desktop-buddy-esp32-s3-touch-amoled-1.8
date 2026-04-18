#include <Arduino.h>
#include <Wire.h>
#include "hw/pins.h"
#include "hw/expander.h"
#include "hw/display.h"
#include "hw/power.h"
#include "hw/input.h"
#include "hw/imu.h"
#include "hw/rtc.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== SMOKE 3: sensors ===");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (!hwExpanderInit())     { Serial.println("FAIL expander"); while (1) delay(1000); }
  hwExpanderResetSequence();
  if (!hwDisplayInit())      { Serial.println("FAIL display");  while (1) delay(1000); }
  if (!hwPowerInit())        { Serial.println("FAIL power");    while (1) delay(1000); }
  if (!hwInputInit())        { Serial.println("FAIL input");    while (1) delay(1000); }
  if (!hwImuInit())          { Serial.println("FAIL imu");      while (1) delay(1000); }
  if (!hwRtcInit())          { Serial.println("FAIL rtc");      while (1) delay(1000); }

  Serial.println("OK init");
}

void loop() {
  static uint32_t lastLog = 0;
  hwInputUpdate();

  if (millis() - lastLog > 500) {
    lastLog = millis();
    HwBattery b = hwBattery();
    float ax, ay, az; hwImuAccel(&ax, &ay, &az);
    HwTime tm; hwRtcRead(&tm);
    Serial.printf("bat %dmV %d%% usb=%d chg=%d temp=%dC | acc %.2f %.2f %.2f | rtc %04u-%02u-%02u %02u:%02u:%02u dow=%u\n",
                  b.mV, b.pct, b.usbPresent, b.charging, b.tempC,
                  ax, ay, az,
                  tm.Y, tm.Mo, tm.D, tm.H, tm.M, tm.S, tm.dow);
  }

  delay(50);
}
