#include "hw/input.h"
#include "hw/hw.h"
#include "hw/pins.h"
#include "hw/expander.h"
#include "hw/power.h"
#include <Arduino.h>
#include <Wire.h>

#if BOARD_TOUCH_CST92XX
  #include "TouchDrvCSTXXX.hpp"
#endif
// The FT-family path (non-CST92xx, i.e. the 1.8 board) talks to the touch
// controller over raw I2C. Both shipping revisions expose the identical
// FocalTech-style data layout (regs 0x02..0x06); only the I2C address differs
// (FT3168 @ 0x38 on v1, CST816 @ 0x15 on v2), so a single reader serves both.
// The live address is chosen at boot from the detected board revision.

static HwBtn   s_a, s_b;
static HwTouch s_tp;
static uint8_t s_axpEvt = 0;

#if BOARD_TOUCH_CST92XX
static TouchDrvCST92xx s_cst;
#else
static uint8_t s_touchAddr = 0x38;   // resolved at init: 0x38 FT3168 (v1) / 0x15 CST816 (v2)
#endif
static volatile bool                          s_tpIrqFlag = false;

static void IRAM_ATTR onTouchIrq() { s_tpIrqFlag = true; }

bool HwBtn::pressedFor(uint32_t ms) {
  return isPressed && (millis() - pressedAt) >= ms;
}

bool hwInputInit() {
  pinMode(PIN_KEY1, INPUT_PULLUP);   // GPIO0 has external pullup; INPUT_PULLUP is harmless
#if BOARD_HAS_KEY2
  pinMode(PIN_KEY2, INPUT_PULLUP);   // External R18 10K already pulls high; INPUT_PULLUP is harmless
#endif
#if BOARD_BTN_THIRD
  pinMode(PIN_KEY_BOOT, INPUT_PULLUP);   // External R8 10K already pulls high; INPUT_PULLUP is harmless
#endif

#if BOARD_TOUCH_CST92XX
  // CST92xx @ 0x5A via SensorLib. Reset is handled by hwExpanderResetSequence()
  // (TP_RST is shared with LCD_RESET on 1.75C), so pass rstPin=-1 to skip the
  // driver's internal reset — otherwise it would also re-reset the display.
  s_cst.setPins(-1, PIN_TP_INT);
  if (!s_cst.begin(Wire, 0x5A, PIN_I2C_SDA, PIN_I2C_SCL)) {
    Serial.println("hwInput: CST92xx init failed");
    return false;
  }
  Serial.printf("hwInput: CST92xx model=%s\n", s_cst.getModelName());
  s_cst.setMaxCoordinates(LCD_W_PHYS, LCD_H_PHYS);
  s_cst.setMirrorXY(true, true);
  pinMode(PIN_TP_INT, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_TP_INT), onTouchIrq, FALLING);
  return true;
#else
  // Address picked from the board revision detected in hwInit().
  s_touchAddr = hwTouchAddr();
  bool isCst816 = (s_touchAddr == 0x15);

  if (!isCst816) {
    // FT3168 power-mode register 0xA5 = 0x00: active scanning.
    // (CST816 reports by default; no equivalent setup needed.)
    Wire.beginTransmission(s_touchAddr);
    Wire.write(0xA5);
    Wire.write((uint8_t)0x00);
    Wire.endTransmission();
  }

  // Verify the controller answers (chip-id reg: FT3168 0xA0, CST816 0xA7).
  uint8_t idReg = isCst816 ? 0xA7 : 0xA0;
  Wire.beginTransmission(s_touchAddr);
  Wire.write(idReg);
  bool ok = (Wire.endTransmission(false) == 0) &&
            (Wire.requestFrom(s_touchAddr, (uint8_t)1) == 1);
  if (!ok) {
    Serial.printf("hwInput: touch init failed (addr 0x%02X)\n", s_touchAddr);
    return false;
  }
  Serial.printf("hwInput: %s ID=0x%02X (addr 0x%02X)\n",
                isCst816 ? "CST816" : "FT3168", Wire.read(), s_touchAddr);

  pinMode(PIN_TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TP_INT), onTouchIrq, FALLING);
  return true;
#endif
}

static void scanKey1() {
  uint32_t now = millis();
#if BOARD_KEY1_ACTIVE_HIGH
  bool pressed = digitalRead(PIN_KEY1) == HIGH;
#else
  bool pressed = digitalRead(PIN_KEY1) == LOW;
#endif
  s_a.wasPressed  = pressed && !s_a.isPressed;
  s_a.wasReleased = !pressed && s_a.isPressed;
  if (s_a.wasPressed) s_a.pressedAt = now;
  s_a.isPressed = pressed;
}

#if BOARD_HAS_KEY2
static void scanKey2() {
  uint32_t now = millis();
  bool pressed = digitalRead(PIN_KEY2) == LOW;
  s_b.wasPressed  = pressed && !s_b.isPressed;
  s_b.wasReleased = !pressed && s_b.isPressed;
  if (s_b.wasPressed) s_b.pressedAt = now;
  s_b.isPressed = pressed;
}
#endif

#if BOARD_BTN_THIRD
// BOOT key (GPIO9 on 2.16) acts as a menu shortcut: a short tap synthesises
// BTN_A_LONG_PRESS, which main.cpp's existing handler treats as "open menu".
// main.cpp itself is unchanged.
static uint32_t s_bootPressedAt = 0;
static void scanBootKey() {
  bool pressed = digitalRead(PIN_KEY_BOOT) == LOW;
  if (pressed && !s_bootPressedAt) {
    s_bootPressedAt = millis();
  } else if (!pressed && s_bootPressedAt) {
    uint32_t held = millis() - s_bootPressedAt;
    s_bootPressedAt = 0;
    if (held > 30 && held < 1000) {
      // Synthesise a long-press of A so the menu opens.
      s_a.wasPressed  = true;
      s_a.wasReleased = true;
      s_a.pressedAt   = millis() - 1500;  // > LONG_PRESS_MS so isLongPress check passes
      s_a.isPressed   = false;
    }
  }
}
#endif

#if !BOARD_HAS_KEY2
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
#endif

static void scanTouch() {
  // Poll when IRQ fires OR when a finger was down last frame — both FT3168
  // and CST92xx only reliably IRQ on state edges, so a drag wouldn't advance
  // x/y without this.
  bool shouldPoll = s_tpIrqFlag || s_tp.down;
  s_tpIrqFlag = false;

  if (!shouldPoll) {
    s_tp.justPressed  = false;
    s_tp.justReleased = false;
    return;
  }

#if BOARD_TOUCH_CST92XX
  int16_t x[2] = {0}, y[2] = {0};
  uint8_t n = s_cst.getPoint(x, y, s_cst.getSupportTouchPoint());
  if (n > 0) {
    s_tp.justPressed  = !s_tp.down;
    s_tp.justReleased = false;
    // Mirror of hwDisplayPush's letterbox scale: reverse (physical → canvas).
    // Use BOARD_* (raw macros from board header) since display.h's constexpr
    // wrappers aren't visible here.
    #if BOARD_DISPLAY_LETTERBOX
      constexpr int OFF_X  = (LCD_W_PHYS - BOARD_DISPLAY_DEST_W) / 2;
      constexpr int OFF_Y  = (LCD_H_PHYS - BOARD_DISPLAY_DEST_H) / 2;
      int dx = x[0] - OFF_X;
      int dy = y[0] - OFF_Y;
      int tx = (dx * BOARD_HW_W) / BOARD_DISPLAY_DEST_W;
      int ty = (dy * BOARD_HW_H) / BOARD_DISPLAY_DEST_H;
      if (tx < 0) tx = 0; else if (tx >= BOARD_HW_W) tx = BOARD_HW_W - 1;
      if (ty < 0) ty = 0; else if (ty >= BOARD_HW_H) ty = BOARD_HW_H - 1;
      s_tp.x = tx;
      s_tp.y = ty;
    #else
      // Non-letterbox: physical → canvas via OFFSET subtract + scale downscale.
      // OFFSET is 0 on 1.8 (full-fill), 148/128 on 2.16 (centred 184×224 in 480×480).
      int dx = x[0] - BOARD_DISPLAY_OFFSET_X;
      int dy = y[0] - BOARD_DISPLAY_OFFSET_Y;
      int tx = dx / BOARD_DISPLAY_SCALE;
      int ty = dy / BOARD_DISPLAY_SCALE;
      if (tx < 0) tx = 0; else if (tx >= BOARD_HW_W) tx = BOARD_HW_W - 1;
      if (ty < 0) ty = 0; else if (ty >= BOARD_HW_H) ty = BOARD_HW_H - 1;
      s_tp.x = tx;
      s_tp.y = ty;
    #endif
    s_tp.down = true;
  } else {
    s_tp.justReleased = s_tp.down;
    s_tp.down = false;
    s_tp.justPressed  = false;
  }
#else
  // FocalTech-style layout shared by FT3168 and CST816:
  //   reg 0x02 low nibble = finger count; 0x03/0x04 = X hi/lo; 0x05/0x06 = Y hi/lo.
  uint8_t fingers = 0;
  int rx = 0, ry = 0;
  Wire.beginTransmission(s_touchAddr);
  Wire.write((uint8_t)0x02);
  if (Wire.endTransmission(false) == 0 &&
      Wire.requestFrom(s_touchAddr, (uint8_t)5) == 5) {
    fingers     = Wire.read() & 0x0F;
    uint8_t xH  = Wire.read();
    uint8_t xL  = Wire.read();
    uint8_t yH  = Wire.read();
    uint8_t yL  = Wire.read();
    rx = ((int)(xH & 0x0F) << 8) | xL;
    ry = ((int)(yH & 0x0F) << 8) | yL;
  }
  if (fingers > 0 && fingers <= 5) {
    s_tp.justPressed  = !s_tp.down;
    s_tp.justReleased = false;
    int dx = rx - BOARD_DISPLAY_OFFSET_X;
    int dy = ry - BOARD_DISPLAY_OFFSET_Y;
    int tx = dx / BOARD_DISPLAY_SCALE;
    int ty = dy / BOARD_DISPLAY_SCALE;
    if (tx < 0) tx = 0; else if (tx >= BOARD_HW_W) tx = BOARD_HW_W - 1;
    if (ty < 0) ty = 0; else if (ty >= BOARD_HW_H) ty = BOARD_HW_H - 1;
    s_tp.x = tx;
    s_tp.y = ty;
    s_tp.down = true;
  } else {
    s_tp.justReleased = s_tp.down;
    s_tp.down = false;
    s_tp.justPressed  = false;
  }
#endif
}

void hwInputUpdate() {
  scanKey1();
#if BOARD_HAS_KEY2
  scanKey2();
#else
  scanAxp();
#endif
#if BOARD_BTN_THIRD
  scanBootKey();
#endif
  scanTouch();
}

#if BOARD_BTN_SWAP_AB
HwBtn& hwBtnA() { return s_b; }
HwBtn& hwBtnB() { return s_a; }
#else
HwBtn& hwBtnA() { return s_a; }
HwBtn& hwBtnB() { return s_b; }
#endif

uint8_t hwAxpBtnEvent() {
  uint8_t e = s_axpEvt;
  if (e == 0x04) s_axpEvt = 0;   // 0x02 already cleared by scanAxp
  return e;
}

const HwTouch& hwTouch() { return s_tp; }
bool hwTouchIrqPending() { return s_tpIrqFlag; }
