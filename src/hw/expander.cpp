#include "hw/expander.h"
#include "hw/pins.h"
#include <Wire.h>
#include <Arduino.h>

Adafruit_XCA9554 g_expander;

bool hwExpanderInit() {
  if (!g_expander.begin(0x20)) return false;
  g_expander.pinMode(EXIO_LCD_RESET,  OUTPUT);
  g_expander.pinMode(EXIO_TP_RESET,   OUTPUT);
  g_expander.pinMode(EXIO_DSI_PWR_EN, OUTPUT);
  g_expander.pinMode(EXIO_AXP_IRQ,    INPUT);
  return true;
}

void hwExpanderResetSequence() {
  g_expander.digitalWrite(EXIO_LCD_RESET,  LOW);
  g_expander.digitalWrite(EXIO_TP_RESET,   LOW);
  g_expander.digitalWrite(EXIO_DSI_PWR_EN, LOW);
  delay(20);
  g_expander.digitalWrite(EXIO_LCD_RESET,  HIGH);
  g_expander.digitalWrite(EXIO_TP_RESET,   HIGH);
  g_expander.digitalWrite(EXIO_DSI_PWR_EN, HIGH);
  delay(20);
}

bool hwExpanderAxpIrqLow() {
  return g_expander.digitalRead(EXIO_AXP_IRQ) == 0;
}
