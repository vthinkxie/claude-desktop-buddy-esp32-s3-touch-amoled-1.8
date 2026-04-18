#include "hw/display.h"
#include "hw/pins.h"
#include <Arduino.h>

static Arduino_DataBus*  s_bus    = nullptr;
static Arduino_SH8601*   s_gfx    = nullptr;
static Arduino_Canvas*   s_canvas = nullptr;

static const uint8_t BRIGHT_LUT[5] = { 50, 100, 150, 200, 255 };

bool hwDisplayInit() {
  s_bus = new Arduino_ESP32QSPI(
    PIN_LCD_CS, PIN_LCD_SCLK, PIN_LCD_SDIO0, PIN_LCD_SDIO1,
    PIN_LCD_SDIO2, PIN_LCD_SDIO3);
  s_gfx = new Arduino_SH8601(s_bus, GFX_NOT_DEFINED, 0, LCD_W_PHYS, LCD_H_PHYS);
  if (!s_gfx->begin()) { Serial.println("hwDisplay: gfx begin failed"); return false; }
  s_gfx->setBrightness(0);   // black to avoid white flash
  s_canvas = new Arduino_Canvas(HW_W, HW_H, s_gfx);
  if (!s_canvas->begin()) { Serial.println("hwDisplay: canvas begin failed"); return false; }
  s_gfx->setBrightness(150);  // default mid-brightness; main may override later
  return true;
}

Arduino_Canvas* hwCanvas() { return s_canvas; }

void hwDisplayBrightness(uint8_t lvl) {
  if (lvl > 4) lvl = 4;
  s_gfx->setBrightness(BRIGHT_LUT[lvl]);
}

void hwDisplaySleep(bool off) {
  if (off) {
    s_gfx->setBrightness(0);
    s_gfx->displayOff();
  } else {
    s_gfx->displayOn();
    hwDisplayBrightness(2);   // mid; caller usually re-applies stored level
  }
}

static uint16_t s_lineBuf[LCD_W_PHYS];   // one physical row, internal RAM
static bool     s_borderAlertOn = false;

extern "C" void hwBorderAlertSetInternal(bool on);  // forward decl from border.cpp

void hwBorderAlertSetInternal(bool on) { s_borderAlertOn = on; }

static const uint16_t BORDER_RED = 0xF800;

void hwDisplayPush() {
  uint16_t* src = (uint16_t*)s_canvas->getFramebuffer();
  for (int y = 0; y < HW_H; y++) {
    bool isBorderRow = s_borderAlertOn && (y < 4 || y >= HW_H - 4);

    if (isBorderRow) {
      for (int i = 0; i < LCD_W_PHYS; i++) s_lineBuf[i] = BORDER_RED;
    } else {
      uint16_t* row = src + y * HW_W;
      for (int x = 0; x < HW_W; x++) {
        uint16_t c = row[x];
        s_lineBuf[x*2]     = c;
        s_lineBuf[x*2 + 1] = c;
      }
      if (s_borderAlertOn) {
        // Left/right 8 physical pixels = 4 logical pixels of red border
        for (int i = 0; i < 8; i++) {
          s_lineBuf[i] = BORDER_RED;
          s_lineBuf[LCD_W_PHYS - 1 - i] = BORDER_RED;
        }
      }
    }

    s_gfx->draw16bitRGBBitmap(0, y * 2,     s_lineBuf, LCD_W_PHYS, 1);
    s_gfx->draw16bitRGBBitmap(0, y * 2 + 1, s_lineBuf, LCD_W_PHYS, 1);
  }
}
