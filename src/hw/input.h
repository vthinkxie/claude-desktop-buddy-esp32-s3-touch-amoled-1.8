#pragma once
#include <stdint.h>

struct HwBtn {
  bool isPressed;
  bool wasPressed;
  bool wasReleased;
  uint32_t pressedAt;
  bool pressedFor(uint32_t ms);
};

struct HwTouch {
  bool down;
  int16_t x, y;
  bool justPressed;
  bool justReleased;
};

bool hwInputInit();
void hwInputUpdate();

HwBtn& hwBtnA();          // Key1 (GPIO0)
HwBtn& hwBtnB();          // Key3 short-press (AXP IRQ 0x02)
uint8_t hwAxpBtnEvent();  // 0 / 0x02 / 0x04 — caller consumes 0x04

const HwTouch& hwTouch();
