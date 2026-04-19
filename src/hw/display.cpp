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
  s_canvas = new Arduino_Canvas(HW_W, HW_H, s_gfx);
  // canvas->begin() internally calls gfx->begin() which calls bus init.
  // Calling them separately would double-init the SPI bus → ESP_ERR_INVALID_STATE.
  if (!s_canvas->begin()) { Serial.println("hwDisplay: canvas begin failed"); return false; }
  s_gfx->setBrightness(0);   // black first frame to avoid white flash
  delay(20);
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
    uint16_t* row = src + y * HW_W;
    for (int x = 0; x < HW_W; x++) {
      uint16_t c = row[x];
      s_lineBuf[x*2]     = c;
      s_lineBuf[x*2 + 1] = c;
    }
    s_gfx->draw16bitRGBBitmap(0, y * 2,     s_lineBuf, LCD_W_PHYS, 1);
    s_gfx->draw16bitRGBBitmap(0, y * 2 + 1, s_lineBuf, LCD_W_PHYS, 1);
  }

  // Attention indicator: small red pill centered at the top of the
  // panel. Inset well below the rounded bezel corners. Less intrusive
  // than a full frame; reads as a "notification dot".
  if (s_borderAlertOn) {
    const int BAR_W = 200;
    const int BAR_H = 8;
    const int BAR_Y = 18;
    const int BAR_X = (LCD_W_PHYS - BAR_W) / 2;
    s_gfx->fillRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, BAR_H / 2, BORDER_RED);
  }
}
