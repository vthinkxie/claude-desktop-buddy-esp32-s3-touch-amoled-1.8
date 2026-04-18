#pragma once

// LCD (QSPI to SH8601 AMOLED)
#define PIN_LCD_SDIO0 4
#define PIN_LCD_SDIO1 5
#define PIN_LCD_SDIO2 6
#define PIN_LCD_SDIO3 7
#define PIN_LCD_SCLK  11
#define PIN_LCD_CS    12
#define LCD_W_PHYS    368
#define LCD_H_PHYS    448

// I2C bus (shared: TCA9554, AXP2101, QMI8658, PCF85063, ES8311, FT3168)
#define PIN_I2C_SDA   15
#define PIN_I2C_SCL   14

// FT3168 touch interrupt (direct to ESP32, not via expander)
#define PIN_TP_INT    21

// I2S to ES8311 codec + speaker
#define PIN_I2S_MCLK  16
#define PIN_I2S_BCLK  9
#define PIN_I2S_WS    45
#define PIN_I2S_DI    10
#define PIN_I2S_DO    8
#define PIN_PA_CTRL   46

// Buttons
#define PIN_KEY1      0   // GPIO0 BOOT key, active-low

// SD card (unused in this port)
// CLK=2 CMD=1 DAT=3 — left undefined here

// TCA9554 expander pin assignments (per schematic)
#define EXIO_LCD_RESET  0
#define EXIO_TP_RESET   1
#define EXIO_DSI_PWR_EN 2
#define EXIO_AXP_IRQ    5
