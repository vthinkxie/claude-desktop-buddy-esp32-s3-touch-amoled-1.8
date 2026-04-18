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
