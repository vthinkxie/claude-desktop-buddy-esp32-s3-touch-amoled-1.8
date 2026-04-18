#pragma once
#include <stdint.h>

struct HwBattery {
  int   mV;          // battery voltage, millivolts
  int   mA;          // + discharging, − charging
  int   pct;         // 0..100, derived linearly from mV
  bool  usbPresent;  // VBUS > 4V
  bool  charging;
  int   tempC;
};

bool hwPowerInit();
HwBattery hwBattery();
void hwPowerOff();
uint32_t hwAxpIrqStatusClear();   // returns IRQ status, clears it
