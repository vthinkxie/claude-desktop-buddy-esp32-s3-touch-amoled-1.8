#include "hw/power.h"
#include "hw/pins.h"
#include <Wire.h>
#include <XPowersLib.h>

static XPowersPMU s_pmu;

bool hwPowerInit() {
  if (!s_pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL)) {
    Serial.println("hwPower: AXP2101 begin failed");
    return false;
  }
  s_pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
  s_pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
  s_pmu.enableBattDetection();
  s_pmu.enableVbusVoltageMeasure();
  s_pmu.enableBattVoltageMeasure();
  s_pmu.enableTemperatureMeasure();
  s_pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  s_pmu.enableIRQ(XPOWERS_PWR_BTN_CLICK_INT | XPOWERS_PWR_BTN_LONGPRESSED_INT);
  s_pmu.clearIrqStatus();
  return true;
}

HwBattery hwBattery() {
  HwBattery b;
  b.mV         = s_pmu.getBattVoltage();
  // AXP2101 (via XPowersLib) does not expose actual battery current,
  // only set charge target. We report 0 mA — info screen shows ±/charging
  // state via hb.charging instead. mA stays in struct for ABI compat.
  b.mA         = 0;
  int p        = (b.mV - 3200) / 10;
  if (p < 0) p = 0; if (p > 100) p = 100;
  b.pct        = p;
  int vbus_mV  = s_pmu.getVbusVoltage();
  b.usbPresent = vbus_mV > 4000;
  b.charging   = s_pmu.isCharging();
  b.tempC      = (int)s_pmu.getTemperature();
  return b;
}

void hwPowerOff() { s_pmu.shutdown(); }

uint32_t hwAxpIrqStatusClear() {
  uint32_t s = s_pmu.getIrqStatus();
  s_pmu.clearIrqStatus();
  return s;
}
