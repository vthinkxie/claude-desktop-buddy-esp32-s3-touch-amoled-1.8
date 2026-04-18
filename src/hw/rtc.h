#pragma once
#include <stdint.h>

struct HwTime {
  uint8_t  H, M, S;
  uint16_t Y;
  uint8_t  Mo, D, dow;     // dow: 0=Sun..6=Sat (matches old code)
};

bool hwRtcInit();
bool hwRtcRead(HwTime* t);
bool hwRtcWrite(const HwTime& t);
