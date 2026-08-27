// src/boards/board_waveshare_esp32s3_touch_amoled_1_8_v2.h
#pragma once

// Waveshare ESP32-S3-Touch-AMOLED-1.8 V2 board.
// V2 hardware differs from the original 1.8:
//   - Display: CO5300 (not SH8601), 368×448, QSPI, col_offset1 = 16
//   - Touch:   CST816/CST820 @ 0x15 (not FT3168 @ 0x38)
// Pinout is otherwise identical to the original 1.8 (same QSPI, I2C, I2S).

#define LCD_W_PHYS  368
#define LCD_H_PHYS  448

// Logical canvas (half of physical — upscale done in hwDisplayPush).
#define BOARD_HW_W        184
#define BOARD_HW_H        224

#define BOARD_SAFE_INSET  8

// QSPI to CO5300 (same pins as original 1.8; reset via XCA9554 expander)
#define PIN_LCD_SDIO0  4
#define PIN_LCD_SDIO1  5
#define PIN_LCD_SDIO2  6
#define PIN_LCD_SDIO3  7
#define PIN_LCD_SCLK   11
#define PIN_LCD_CS     12
#define PIN_LCD_RESET  (-1)   // reset via XCA9554 expander, not direct GPIO
#define PIN_TP_RESET   (-1)   // reset via XCA9554 expander, not direct GPIO

// LCD/TP reset are driven via the XCA9554 expander (EXIO_LCD_RESET /
// EXIO_TP_RESET), not direct GPIOs. -1 (GFX_NOT_DEFINED) tells the display
// and touch drivers to skip their own direct-GPIO reset and rely on the
// hwExpanderResetSequence() pulse — matching the official V2 example.

// I2C bus (shared: XCA9554, AXP2101, CST816, ES8311)
#define PIN_I2C_SDA   15
#define PIN_I2C_SCL   14

// CST816 touch interrupt — direct to ESP32
#define PIN_TP_INT    21

// I2S to ES8311 codec
#define PIN_I2S_MCLK  16
#define PIN_I2S_BCLK  9
#define PIN_I2S_WS    45
#define PIN_I2S_DI    10
#define PIN_I2S_DO    8
#define PIN_PA_CTRL   46

// Buttons
#define PIN_KEY1      0   // GPIO0 BOOT key, active-low

// XCA9554 I2C GPIO expander (V2 uses it for LCD/TP reset like original 1.8)
#define BOARD_HAS_TCA9554  1
#define EXIO_LCD_RESET     0
#define EXIO_TP_RESET      1
#define EXIO_DSI_PWR_EN    2
#define EXIO_AXP_IRQ       5

// Display: CO5300 (V2). col_offset1 = 16 per Waveshare V2 example.
#define BOARD_DISPLAY_CO5300     1
#define BOARD_CO5300_COL_OFFSET  16

// Letterbox one-shot blit: CO5300 can't tolerate per-row draws.
// 184×224 canvas → DEST 368×448 (exact 2×) fills the full 368×448 panel.
#define BOARD_DISPLAY_LETTERBOX  1
#define BOARD_DISPLAY_DEST_W     368
#define BOARD_DISPLAY_DEST_H     448

// Touch: CST816/CST820 @ 0x15 via SensorLib TouchDrvCST816
#define BOARD_TOUCH_CST816  1

#define BOARD_BTN_SWAP_AB  0

// External PCF85063 RTC @ 0x51 (same as original 1.8)
#define BOARD_HAS_PCF85063  1

// Capability flags
#define BOARD_HAS_PSRAM            1
#define BOARD_DISPLAY_OFFSET_X     0
#define BOARD_DISPLAY_OFFSET_Y     0
#define BOARD_DISPLAY_SCALE        1   // letterbox path uses DEST_W/H math
#define BOARD_HAS_PA_CTRL          1
#define BOARD_HAS_AXP2101          1
#define BOARD_LCD_RST_VIA_PMU      0
#define BOARD_AXP_PWRON_4S_OFF     0
#define BOARD_AXP_ENABLE_AUX_LDOS  0
#define BOARD_BTN_THIRD            0
#define BOARD_KEY1_ACTIVE_HIGH     0
#define BOARD_HAS_KEY2             0
#define BOARD_DISPLAY_PUSH_STREAMED 0
#define BOARD_DISPLAY_ROTATION     0
#define BOARD_CO5300_MADCTL        0   // 0 = use lib's setRotation MADCTL unchanged
#define BOARD_DISPLAY_SH8601_VENDOR_INIT 0

// Credits-page hardware identification
#define BOARD_MODEL_LINE1  "Waveshare ESP32-S3"
#define BOARD_MODEL_LINE2  "Touch AMOLED 1.8 V2"
