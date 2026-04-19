#pragma once
#include <Arduino_GFX_Library.h>

constexpr int HW_W = 184;
constexpr int HW_H = 224;

// Logical-canvas safe-draw region for rounded AMOLED bezel.
// Symmetric inset; finalized after on-device calibration (DEBUG_SAFE_BOX).
constexpr int SAFE_INSET = 8;
constexpr int SAFE_L = SAFE_INSET;
constexpr int SAFE_T = SAFE_INSET;
constexpr int SAFE_R = HW_W - SAFE_INSET;        // 176
constexpr int SAFE_B = HW_H - SAFE_INSET;        // 216
constexpr int SAFE_W = HW_W - 2 * SAFE_INSET;    // 168
constexpr int SAFE_H = HW_H - 2 * SAFE_INSET;    // 208

bool hwDisplayInit();                       // false on any failure
void hwDisplayPush();                       // 2× upscale canvas → AMOLED
void hwDisplayBrightness(uint8_t lvl_0_4);  // 0..4 → 50/100/150/200/255
void hwDisplaySleep(bool off);              // true: blank AMOLED
Arduino_Canvas* hwCanvas();
