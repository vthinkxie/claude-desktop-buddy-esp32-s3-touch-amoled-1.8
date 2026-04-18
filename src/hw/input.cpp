#include "hw/input.h"
#include "hw/pins.h"
#include "hw/expander.h"
#include "hw/power.h"
#include <Arduino.h>
#include <Arduino_DriveBus_Library.h>

static HwBtn   s_a, s_b;
static HwTouch s_tp;
static uint8_t s_axpEvt = 0;

static std::shared_ptr<Arduino_IIC_DriveBus> s_iicBus;
static std::unique_ptr<Arduino_IIC>          s_ft3168;
static volatile bool                          s_tpIrqFlag = false;

static void IRAM_ATTR onTouchIrq() { s_tpIrqFlag = true; }

bool HwBtn::pressedFor(uint32_t ms) {
  return isPressed && (millis() - pressedAt) >= ms;
}

bool hwInputInit() {
  pinMode(PIN_KEY1, INPUT_PULLUP);   // GPIO0 has external pullup; INPUT_PULLUP is harmless

  s_iicBus = std::make_shared<Arduino_HWIIC>(PIN_I2C_SDA, PIN_I2C_SCL, &Wire);
  s_ft3168.reset(new Arduino_FT3x68(s_iicBus, FT3168_DEVICE_ADDRESS,
                                    DRIVEBUS_DEFAULT_VALUE, PIN_TP_INT, onTouchIrq));
  for (int i = 0; i < 5; i++) {
    if (s_ft3168->begin()) return true;
    delay(100);
  }
  Serial.println("hwInput: FT3168 init failed");
  return false;
}

static void scanKey1() {
  uint32_t now = millis();
  bool pressed = digitalRead(PIN_KEY1) == LOW;
  s_a.wasPressed  = pressed && !s_a.isPressed;
  s_a.wasReleased = !pressed && s_a.isPressed;
  if (s_a.wasPressed) s_a.pressedAt = now;
  s_a.isPressed = pressed;
}

static void scanAxp() {
  if (hwExpanderAxpIrqLow()) {
    if (hwAxpPekeyShortPress()) s_axpEvt = 0x02;
    if (hwAxpPekeyLongPress())  s_axpEvt = 0x04;
  }
  // Route 0x02 to BtnB pulse for one frame:
  bool pressed = (s_axpEvt == 0x02);
  s_b.wasPressed  = pressed;
  s_b.wasReleased = pressed;
  s_b.isPressed   = false;
  if (pressed) s_axpEvt = 0;
  // 0x04 stays in s_axpEvt until consumed by hwAxpBtnEvent()
}

static void scanTouch() {
  if (!s_tpIrqFlag) {
    s_tp.justPressed  = false;
    s_tp.justReleased = s_tp.down;
    s_tp.down         = false;
    return;
  }
  s_tpIrqFlag = false;
  uint8_t fingers = (uint8_t)s_ft3168->IIC_Read_Device_Value(
      Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  if (fingers > 0) {
    int rx = (int)s_ft3168->IIC_Read_Device_Value(
        Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
    int ry = (int)s_ft3168->IIC_Read_Device_Value(
        Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
    s_tp.justPressed  = !s_tp.down;
    s_tp.justReleased = false;
    s_tp.x = rx / 2;   // 368 → 184
    s_tp.y = ry / 2;   // 448 → 224
    s_tp.down = true;
  } else {
    s_tp.justReleased = s_tp.down;
    s_tp.down = false;
    s_tp.justPressed  = false;
  }
}

void hwInputUpdate() {
  scanKey1();
  scanAxp();
  scanTouch();
}

HwBtn& hwBtnA() { return s_a; }
HwBtn& hwBtnB() { return s_b; }

uint8_t hwAxpBtnEvent() {
  uint8_t e = s_axpEvt;
  if (e == 0x04) s_axpEvt = 0;   // 0x02 already cleared by scanAxp
  return e;
}

const HwTouch& hwTouch() { return s_tp; }
