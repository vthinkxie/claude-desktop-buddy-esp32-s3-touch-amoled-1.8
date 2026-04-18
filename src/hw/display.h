#pragma once
#include <Arduino_GFX_Library.h>

constexpr int HW_W = 184;
constexpr int HW_H = 224;

bool hwDisplayInit();                       // false on any failure
void hwDisplayPush();                       // 2× upscale canvas → AMOLED
void hwDisplayBrightness(uint8_t lvl_0_4);  // 0..4 → 50/100/150/200/255
void hwDisplaySleep(bool off);              // true: blank AMOLED
Arduino_Canvas* hwCanvas();
