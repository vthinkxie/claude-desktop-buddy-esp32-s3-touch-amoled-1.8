#include "hw/hw.h"
#include <Arduino.h>
#include <Wire.h>

static void die(const char* what) {
  Serial.printf("hwInit FAIL: %s\n", what);
  while (1) delay(1000);
}

#if BOARD_DISPLAY_RUNTIME_DETECT
// The Waveshare 1.8" AMOLED ships in two revisions that differ only in the
// bonded panel module: v1 = SH8601 display + FT3168 touch (I2C 0x38),
// v2 = CO5300 display + CST816 touch (I2C 0x15). Detect by which touch
// controller answers, then route the display and touch drivers accordingly.
// Must run AFTER hwExpanderResetSequence() (the touch chip is held in reset
// until EXIO TP_RESET goes high) and BEFORE hwDisplayInit()/hwInputInit().
static bool s_isCo5300 = false;

bool    hwBoardIsCo5300() { return s_isCo5300; }
uint8_t hwTouchAddr()     { return s_isCo5300 ? 0x15 : 0x38; }

static bool i2cPresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

static void detectBoardRev() {
  // FT3168 (0x38) exists only on v1; CST816 (0x15) only on v2. CST816 can take
  // up to a few hundred ms to answer after reset, so retry until one responds.
  s_isCo5300 = false;
  for (int i = 0; i < 25; i++) {
    if (i2cPresent(0x38)) { s_isCo5300 = false; break; }  // FT3168 → v1
    if (i2cPresent(0x15)) { s_isCo5300 = true;  break; }  // CST816 → v2
    delay(20);
  }
  Serial.printf("hwInit: board rev = %s\n",
                s_isCo5300 ? "v2 (CO5300 + CST816)" : "v1 (SH8601 + FT3168)");
}
#endif

void hwInit() {
  Serial.begin(115200);
  // USB-Serial-JTAG re-enumerates on reset and drops the host monitor for
  // ~1-2s, so the first boot lines are easily lost. To capture them, flash with
  // `pio run -t upload -t monitor` (one reset, monitor attached) rather than
  // opening a separate monitor. A longer delay here would help capture but
  // slows every boot, so keep it short.
  delay(500);
  Serial.println("\n=== claude-buddy waveshare boot ===");

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  if (!hwExpanderInit())  die("expander");
#if BOARD_LCD_RST_VIA_PMU
  // 2.16 has no LCD_RST GPIO; the panel is reset by power-cycling
  // AXP ALDO3. PMU must be initialised before the display.
  if (!hwPowerInit())     die("power");
  // ALDO3 power-cycle resets the panel (50 ms low between two highs).
  // s_pmu.enableALDO3() in powerInit left it enabled; toggle it here.
  hwPmuRef()->disableALDO3();
  delay(50);
  hwPmuRef()->enableALDO3();
  delay(50);
#endif
  // Toggles PIN_TP_RESET on all boards; PIN_LCD_RESET only on non-PMU
  // boards (gated inside the function via BOARD_LCD_RST_VIA_PMU).
  hwExpanderResetSequence();
#if BOARD_DISPLAY_RUNTIME_DETECT
  detectBoardRev();   // sets hwBoardIsCo5300()/hwTouchAddr() before display+input
#endif
  if (!hwDisplayInit())   die("display");
#if !BOARD_LCD_RST_VIA_PMU
  if (!hwPowerInit())     die("power");
#endif
  if (!hwInputInit())     die("input");
  if (!hwImuInit())       die("imu");
  if (!hwRtcInit())       die("rtc");
  if (!hwAudioInit())     die("audio");

  Serial.println("hwInit OK");
}
