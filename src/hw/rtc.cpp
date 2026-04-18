#include "hw/rtc.h"
#include "hw/pins.h"
#include <Wire.h>
#include <SensorPCF85063.hpp>

static SensorPCF85063 s_pcf;

bool hwRtcInit() {
  if (!s_pcf.begin(Wire, PIN_I2C_SDA, PIN_I2C_SCL)) {
    Serial.println("hwRtc: PCF85063 begin failed");
    return false;
  }
  return true;
}

bool hwRtcRead(HwTime* t) {
  RTC_DateTime dt = s_pcf.getDateTime();
  t->H   = dt.getHour();
  t->M   = dt.getMinute();
  t->S   = dt.getSecond();
  t->Y   = dt.getYear();
  t->Mo  = dt.getMonth();
  t->D   = dt.getDay();
  t->dow = dt.getWeek();
  return true;
}

bool hwRtcWrite(const HwTime& t) {
  RTC_DateTime dt(t.Y, t.Mo, t.D, t.H, t.M, t.S, t.dow);
  s_pcf.setDateTime(dt);
  return true;
}
