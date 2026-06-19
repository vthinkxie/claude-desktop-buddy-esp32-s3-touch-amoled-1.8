// src/boards/board_waveshare_esp32s3_touch_amoled_1_8.h
#pragma once

// Display physical resolution (SH8601 AMOLED, 2× upscaled from canvas).
// Keep name LCD_W_PHYS / LCD_H_PHYS — display.cpp uses these directly via pins.h.
#define LCD_W_PHYS  368
#define LCD_H_PHYS  448

// Logical canvas (half of physical — upscale done in hwDisplayPush).
// BOARD_ prefix required here: display.h wraps these as constexpr int HW_W = BOARD_HW_W;
// without the prefix the preprocessor would expand "constexpr int HW_W = HW_W" to "constexpr int 184 = 184".
#define BOARD_HW_W        184
#define BOARD_HW_H        224

// Safe draw area: symmetric inset from bezel corners (calibrated with DEBUG_SAFE_BOX)
#define BOARD_SAFE_INSET  8

// QSPI to SH8601
#define PIN_LCD_SDIO0  4
#define PIN_LCD_SDIO1  5
#define PIN_LCD_SDIO2  6
#define PIN_LCD_SDIO3  7
#define PIN_LCD_SCLK   11
#define PIN_LCD_CS     12
// LCD_RESET and TP_RESET are driven via TCA9554 expander (see EXIO_* below)

// I2C bus (shared: TCA9554, AXP2101, FT3168, ES8311)
#define PIN_I2C_SDA   15
#define PIN_I2C_SCL   14

// FT3168 touch interrupt — direct to ESP32
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

// TCA9554 I2C GPIO expander pin assignments
#define BOARD_HAS_TCA9554  1
#define EXIO_LCD_RESET     0
#define EXIO_TP_RESET      1
#define EXIO_DSI_PWR_EN    2
#define EXIO_AXP_IRQ       5

// Display: SH8601 (v1) or CO5300 (v2), chosen at runtime — see below.
// Canvas is upscaled 2× to physical.
#define BOARD_DISPLAY_CO5300     0   // compile-time default; overridden by runtime detect
#define BOARD_DISPLAY_LETTERBOX  0

// This board ships in two hardware revisions that differ only in the bonded
// panel module: v1 = SH8601 + FT3168 (0x38), v2 = CO5300 + CST816 (0x15).
// hwInit() probes the I2C bus and selects the right display + touch driver at
// boot, so one binary drives both. display.cpp/input.cpp compile both paths.
#define BOARD_DISPLAY_RUNTIME_DETECT  1

// Touch: FT3168 @ 0x38 via Arduino_DriveBus (Arduino_FT3x68)
#define BOARD_TOUCH_CST92XX  0

#define BOARD_BTN_SWAP_AB  0

// External PCF85063 RTC @ 0x51 on the I2C bus
#define BOARD_HAS_PCF85063  1

// New flags introduced by 2.16 port. Defaults preserve current 1.8 behavior.
#define BOARD_HAS_PSRAM            1
#define BOARD_DISPLAY_OFFSET_X     0
#define BOARD_DISPLAY_OFFSET_Y     0
#define BOARD_DISPLAY_SCALE        2
#define BOARD_HAS_PA_CTRL          1
#define BOARD_HAS_AXP2101          1
#define BOARD_LCD_RST_VIA_PMU      0
#define BOARD_AXP_PWRON_4S_OFF     0
#define BOARD_BTN_THIRD            0
#define BOARD_KEY1_ACTIVE_HIGH     0
#define BOARD_HAS_KEY2             0

// Credits-page hardware identification (two short lines).
#define BOARD_MODEL_LINE1  "Waveshare ESP32-S3"
#define BOARD_MODEL_LINE2  "Touch AMOLED 1.8"

#define BOARD_DISPLAY_SH8601_VENDOR_INIT  0
#define BOARD_AXP_ENABLE_AUX_LDOS  0
#define BOARD_DISPLAY_PUSH_STREAMED  0   // per-row 2× upscale works fine on S3 240 MHz
#define BOARD_CO5300_COL_OFFSET    16  // v2 CO5300: 368-wide area starts at GRAM col 16 (verified on panel)
#define BOARD_DISPLAY_ROTATION     0
#define BOARD_CO5300_MADCTL        0   // 0 = use lib's setRotation MADCTL unchanged
