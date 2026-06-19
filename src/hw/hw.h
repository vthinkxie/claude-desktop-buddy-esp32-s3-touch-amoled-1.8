#pragma once
#include "hw/display.h"
#include "hw/input.h"
#include "hw/power.h"
#include "hw/imu.h"
#include "hw/rtc.h"
#include "hw/audio.h"
#include "hw/border.h"
#include "hw/expander.h"
#include "hw/pins.h"

void hwInit();   // initialises every subsystem; while(1) on any failure

#if BOARD_DISPLAY_RUNTIME_DETECT
// Runtime panel/touch revision detection (1.8 board ships v1 SH8601+FT3168 or
// v2 CO5300+CST816). Valid only after hwInit() has probed the I2C bus.
bool    hwBoardIsCo5300();   // true on the v2 (CO5300 + CST816) panel
uint8_t hwTouchAddr();       // 0x38 (FT3168, v1) or 0x15 (CST816, v2)
#endif
