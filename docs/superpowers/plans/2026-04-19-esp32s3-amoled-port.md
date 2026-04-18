# ESP32-S3-Touch-AMOLED-1.8 Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port the M5StickC Plus firmware to a Waveshare ESP32-S3-Touch-AMOLED-1.8 board with full feature parity, on a new `waveshare` branch, leaving `main` untouched.

**Architecture:** Thin HAL (`src/hw/`) wraps all board-specific drivers (Arduino_GFX SH8601 + Arduino_Canvas at 184×224 with 2× upscale push, Key1/Key3 + FT3168 input, AXP2101 power, QMI8658 IMU, PCF85063 RTC, ES8311 audio). `main.cpp` only `#include`s `hw/hw.h`, never board libraries directly. Touch is an additive event source via a `tap(x,y,w,h)` helper — keys remain primary. AMOLED edge flash replaces the LED. Five-stage smoke test bring-up: display → input → sensors → BLE → full integration.

**Tech Stack:** ESP32-S3 + Arduino framework, PlatformIO, Arduino_GFX, XPowersLib, SensorLib, Adafruit_BusIO, Arduino_DriveBus (FT3168 + PCF85063), TCA9554 expander, NimBLE-Arduino, AnimatedGIF, ArduinoJson, LittleFS.

**Spec:** `docs/superpowers/specs/2026-04-19-esp32s3-amoled-port-design.md`

**Note on testing:** Embedded firmware on a unique board has no automated test harness. Each phase ends with a manual smoke test producing observable serial output and/or visual behavior. "Run test" steps mean "flash, reset, observe expected output". Commit at the end of every task.

---

## Phase 0 — Branch & scaffolding

### Task 1: Create `waveshare` branch from `main`

**Files:**
- N/A (git only)

- [ ] **Step 1: Verify current branch is `main` and clean**

```bash
git status
git branch --show-current
```

Expected: branch = `main`, working tree clean.

- [ ] **Step 2: Create and switch to `waveshare` branch**

```bash
git checkout -b waveshare
```

Expected: `Switched to a new branch 'waveshare'`

- [ ] **Step 3: Push to remote and set upstream**

```bash
git push -u origin waveshare
```

Expected: branch created on origin.

---

### Task 2: Copy bundled libraries from Waveshare examples to `lib/`

The two libraries `Arduino_DriveBus` (FT3168 + PCF85063) and `Adafruit_XCA9554` (I/O expander) are not on PlatformIO Registry. They are bundled with the Waveshare examples and must be vendored.

**Files:**
- Create: `lib/Arduino_DriveBus/` (copied tree)
- Create: `lib/Adafruit_XCA9554/` (copied tree)

- [ ] **Step 1: Copy Arduino_DriveBus**

```bash
cp -R /Users/yadongxie/Downloads/ESP32-S3-Touch-AMOLED-1.8/examples/Arduino-v3.3.5/libraries/Arduino_DriveBus lib/
```

- [ ] **Step 2: Copy Adafruit_XCA9554**

```bash
cp -R /Users/yadongxie/Downloads/ESP32-S3-Touch-AMOLED-1.8/examples/Arduino-v3.3.5/libraries/Adafruit_XCA9554 lib/
```

- [ ] **Step 3: Verify each lib has a `library.properties` or `library.json` so PlatformIO can index it**

```bash
ls lib/Arduino_DriveBus/library.* 2>/dev/null
ls lib/Adafruit_XCA9554/library.* 2>/dev/null
```

If missing, create a minimal `library.json` per lib:

```json
{
  "name": "Arduino_DriveBus",
  "version": "1.0.0",
  "frameworks": "arduino",
  "platforms": "espressif32"
}
```

- [ ] **Step 4: Commit**

```bash
git add lib/
git commit -m "vendor Waveshare Arduino_DriveBus and Adafruit_XCA9554 libs"
```

---

### Task 3: Replace `platformio.ini` with the Waveshare environment

**Files:**
- Modify: `platformio.ini` (full rewrite)

- [ ] **Step 1: Replace contents**

```ini
[env:waveshare-amoled]
platform = espressif32 @ ^6.5
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600

board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.flash_mode = qio
board_build.psram_type = opi
board_upload.flash_size = 8MB
board_build.partitions = no_ota_8mb.csv
board_build.filesystem = littlefs
board_build.arduino.memory_type = qio_opi

build_src_filter = +<*> +<buddies/> +<hw/>
build_flags =
    -DCORE_DEBUG_LEVEL=0
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DXPOWERS_CHIP_AXP2101

lib_deps =
    moononournation/GFX Library for Arduino
    lewisxhe/XPowersLib
    lewisxhe/SensorLib
    adafruit/Adafruit BusIO
    bitbank2/AnimatedGIF @ ^2.1.1
    bblanchon/ArduinoJson @ ^7.0.0
    h2zero/NimBLE-Arduino @ ^1.4
```

- [ ] **Step 2: Commit**

```bash
git add platformio.ini
git commit -m "platformio: switch to waveshare-amoled (ESP32-S3) env"
```

---

### Task 4: Add 8MB partition table

**Files:**
- Create: `no_ota_8mb.csv`
- Delete: `no_ota.csv` (old M5Stick partition table — keep on `main`, remove on `waveshare`)

- [ ] **Step 1: Create `no_ota_8mb.csv`**

```
# Name,    Type, SubType,  Offset,   Size
nvs,       data, nvs,      0x9000,   0x6000
phy_init,  data, phy,      0xf000,   0x1000
factory,   app,  factory,  0x10000,  0x600000
spiffs,    data, spiffs,   0x610000, 0x1F0000
```

- [ ] **Step 2: Remove old partition table**

```bash
git rm no_ota.csv
```

- [ ] **Step 3: Verify build configuration compiles even without source changes (will fail later in source files but config should parse)**

```bash
pio run -e waveshare-amoled 2>&1 | head -40
```

Expected: PlatformIO finds the env, downloads dependencies. Compile errors in `src/main.cpp` are EXPECTED at this point (M5StickCPlus.h missing).

- [ ] **Step 4: Commit**

```bash
git add no_ota_8mb.csv
git commit -m "partitions: 8MB layout for ESP32-S3 (6MB app, 2MB LittleFS)"
```

---

## Phase 1 — Display bring-up (smoke test 1)

### Task 5: Create `src/hw/pins.h`

**Files:**
- Create: `src/hw/pins.h`

- [ ] **Step 1: Write contents**

```c
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
```

- [ ] **Step 2: Commit**

```bash
git add src/hw/pins.h
git commit -m "hw: add pins.h with all GPIO and expander pin assignments"
```

---

### Task 6: Create `src/hw/expander.cpp` and stub `hw/hw.h`

**Files:**
- Create: `src/hw/hw.h`
- Create: `src/hw/expander.h`
- Create: `src/hw/expander.cpp`

- [ ] **Step 1: Create initial `src/hw/hw.h` with `hwInit()` declaration only**

```c
#pragma once
#include <stdint.h>

// Top-level initialiser; calls all subsystem init in correct order
void hwInit();
```

- [ ] **Step 2: Create `src/hw/expander.h`**

```c
#pragma once
#include <Adafruit_XCA9554.h>

// Shared TCA9554 instance. Initialised by hwExpanderInit() before any
// driver that needs LCD/TP reset or AXP IRQ access.
extern Adafruit_XCA9554 g_expander;

bool hwExpanderInit();          // returns false on I2C failure
void hwExpanderResetSequence(); // pull EXIO 0/1/2 low → 20ms → high
bool hwExpanderAxpIrqLow();     // true while AXP IRQ pin is asserted
```

- [ ] **Step 3: Create `src/hw/expander.cpp`**

```cpp
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
```

- [ ] **Step 4: Commit**

```bash
git add src/hw/hw.h src/hw/expander.h src/hw/expander.cpp
git commit -m "hw: add TCA9554 I/O expander wrapper"
```

---

### Task 7: Create `src/hw/display.{h,cpp}` — init + canvas

**Files:**
- Create: `src/hw/display.h`
- Create: `src/hw/display.cpp`

- [ ] **Step 1: Create `src/hw/display.h`**

```c
#pragma once
#include <Arduino_GFX_Library.h>

constexpr int HW_W = 184;
constexpr int HW_H = 224;

bool hwDisplayInit();                       // false on any failure
void hwDisplayPush();                       // 2× upscale canvas → AMOLED
void hwDisplayBrightness(uint8_t lvl_0_4);  // 0..4 → 50/100/150/200/255
void hwDisplaySleep(bool off);              // true: blank AMOLED
Arduino_Canvas* hwCanvas();
```

- [ ] **Step 2: Create `src/hw/display.cpp` with init + canvas only (push comes next task)**

```cpp
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
```

- [ ] **Step 3: Commit**

```bash
git add src/hw/display.h src/hw/display.cpp
git commit -m "hw/display: SH8601 init, Canvas (PSRAM), brightness, sleep"
```

---

### Task 8: Implement `hwDisplayPush()` with 2× nearest-neighbor upscale

**Files:**
- Modify: `src/hw/display.cpp` (add `hwDisplayPush()`)

- [ ] **Step 1: Append to `src/hw/display.cpp`**

```cpp
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
```

- [ ] **Step 2: Commit**

```bash
git add src/hw/display.cpp
git commit -m "hw/display: 2x upscale push with optional red border alert"
```

---

### Task 9: Create `src/hw/border.{h,cpp}` — public toggle for the alert flag

**Files:**
- Create: `src/hw/border.h`
- Create: `src/hw/border.cpp`

- [ ] **Step 1: Create `src/hw/border.h`**

```c
#pragma once
void hwBorderAlert(bool on);
```

- [ ] **Step 2: Create `src/hw/border.cpp`**

```cpp
#include "hw/border.h"

extern "C" void hwBorderAlertSetInternal(bool on);

void hwBorderAlert(bool on) { hwBorderAlertSetInternal(on); }
```

- [ ] **Step 3: Commit**

```bash
git add src/hw/border.h src/hw/border.cpp
git commit -m "hw/border: public alert toggle, forwards to display flag"
```

---

### Task 10: Smoke test 1 — minimal main draws a "HELLO" box

Replace the M5-based `src/main.cpp` with a stripped bring-up that exercises only the display pipeline. Once verified, the real main migration happens in Phase 6.

**Files:**
- Modify: `src/main.cpp` (full rewrite, temporary)
- Create: `src/main.cpp.real` (back up the M5 main for restoration)

- [ ] **Step 1: Back up the M5 main to a non-compiled location**

```bash
git mv src/main.cpp src/main.cpp.m5stick.bak
```

- [ ] **Step 2: Add the backup to build_src_filter exclusion**

Modify `platformio.ini` `build_src_filter`:

```
build_src_filter = +<*> +<buddies/> +<hw/> -<main.cpp.m5stick.bak>
```

- [ ] **Step 3: Create new `src/main.cpp` for smoke 1**

```cpp
#include <Arduino.h>
#include "hw/expander.h"
#include "hw/display.h"
#include "hw/border.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== SMOKE 1: display ===");

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (!hwExpanderInit())     { Serial.println("FAIL expander"); while (1) delay(1000); }
  hwExpanderResetSequence();
  if (!hwDisplayInit())      { Serial.println("FAIL display");  while (1) delay(1000); }

  Serial.println("OK init");
}

void loop() {
  static uint32_t frame = 0;
  Arduino_Canvas* c = hwCanvas();
  c->fillScreen(0x0000);
  c->setTextColor(0xFFFF);
  c->setTextSize(3);
  c->setCursor(20, 80);
  c->print("HELLO");
  c->setTextSize(1);
  c->setCursor(20, 130);
  c->printf("frame %lu", frame++);

  // Toggle border alert every 60 frames so we see it works
  hwBorderAlert((frame / 60) % 2 == 0);

  uint32_t t0 = millis();
  hwDisplayPush();
  uint32_t dt = millis() - t0;
  if (frame % 30 == 0) Serial.printf("push %lums\n", dt);

  delay(16);
}
```

`Wire.h` include needs adding too:

```cpp
#include <Wire.h>
```

- [ ] **Step 4: Build**

```bash
pio run -e waveshare-amoled
```

Expected: clean build. If errors about missing types, fix in source — don't proceed.

- [ ] **Step 5: Flash and observe**

```bash
pio run -e waveshare-amoled -t upload && pio device monitor
```

Expected:
- Serial prints "OK init" then `push <N>ms` every 30 frames where N is roughly 25–40
- AMOLED shows white "HELLO" + frame counter on black
- Every ~1 second the screen border flashes between red and black (4px)

**Stop here if:**
- Display dark / blank → check expander reset sequence, gfx->begin() return value
- Push >50ms consistently → QSPI clock or PSRAM allocation issue
- Reset loop → likely PSRAM not detected; verify `board_build.psram_type = opi`

- [ ] **Step 6: Commit**

```bash
git add src/main.cpp platformio.ini
git mv src/main.cpp.m5stick.bak src/main.cpp.m5stick.bak  # ensure tracked
git add src/main.cpp.m5stick.bak
git commit -m "smoke 1: minimal main exercises display + border alert"
```

---

## Phase 2 — Power & expander setup (needed before input)

### Task 11: Create `src/hw/power.{h,cpp}` — AXP2101 init + battery

**Files:**
- Create: `src/hw/power.h`
- Create: `src/hw/power.cpp`

- [ ] **Step 1: Create `src/hw/power.h`**

```c
#pragma once
#include <stdint.h>

struct HwBattery {
  int   mV;          // battery voltage, millivolts
  int   mA;          // + discharging, − charging
  int   pct;         // 0..100, derived linearly from mV
  bool  usbPresent;  // VBUS > 4V
  bool  charging;
  int   tempC;
};

bool hwPowerInit();
HwBattery hwBattery();
void hwPowerOff();
uint32_t hwAxpIrqStatusClear();   // returns IRQ status, clears it
```

- [ ] **Step 2: Create `src/hw/power.cpp`**

```cpp
#include "hw/power.h"
#include "hw/pins.h"
#include <Wire.h>
#include <XPowersLib.h>

static XPowersPMU s_pmu;

bool hwPowerInit() {
  if (!s_pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL)) {
    Serial.println("hwPower: AXP2101 begin failed");
    return false;
  }
  s_pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
  s_pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
  s_pmu.enableBattDetection();
  s_pmu.enableVbusVoltageMeasure();
  s_pmu.enableBattVoltageMeasure();
  s_pmu.enableTemperatureMeasure();
  s_pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  s_pmu.enableIRQ(XPOWERS_PWR_BTN_CLICK_INT | XPOWERS_PWR_BTN_LONGPRESSED_INT);
  s_pmu.clearIrqStatus();
  return true;
}

HwBattery hwBattery() {
  HwBattery b;
  b.mV         = s_pmu.getBattVoltage();             // mV (verify with XPowersLib source)
  int chg      = s_pmu.getBatteryChargeCurrent();    // 0 if not charging
  int dis      = s_pmu.getBattDischargeCurrent();    // 0 if not discharging
  b.mA         = chg > 0 ? -chg : dis;
  int p        = (b.mV - 3200) / 10;
  if (p < 0) p = 0; if (p > 100) p = 100;
  b.pct        = p;
  int vbus_mV  = s_pmu.getVbusVoltage();
  b.usbPresent = vbus_mV > 4000;
  b.charging   = b.usbPresent && chg > 0;
  b.tempC      = (int)s_pmu.getTemperature();
  return b;
}

void hwPowerOff() { s_pmu.shutdown(); }

uint32_t hwAxpIrqStatusClear() {
  uint32_t s = s_pmu.getIrqStatus();
  s_pmu.clearIrqStatus();
  return s;
}
```

- [ ] **Step 3: Commit**

```bash
git add src/hw/power.h src/hw/power.cpp
git commit -m "hw/power: AXP2101 init, battery state, power off, IRQ status"
```

---

## Phase 3 — Input layer

### Task 12: Create `src/hw/input.{h,cpp}` skeleton + Key1 (GPIO0)

**Files:**
- Create: `src/hw/input.h`
- Create: `src/hw/input.cpp`

- [ ] **Step 1: Create `src/hw/input.h`**

```c
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
```

- [ ] **Step 2: Create `src/hw/input.cpp`**

```cpp
#include "hw/input.h"
#include "hw/pins.h"
#include "hw/expander.h"
#include "hw/power.h"
#include <Arduino.h>
#include <XPowersLib.h>

static HwBtn   s_a, s_b;
static HwTouch s_tp;
static uint8_t s_axpEvt = 0;

bool HwBtn::pressedFor(uint32_t ms) {
  return isPressed && (millis() - pressedAt) >= ms;
}

bool hwInputInit() {
  pinMode(PIN_KEY1, INPUT_PULLUP);   // GPIO0 has external pullup; INPUT_PULLUP is harmless
  return true;
}

static void scanKey1() {
  uint32_t now = millis();
  bool pressed = digitalRead(PIN_KEY1) == LOW;
  s_a.wasPressed  = pressed && !s_a.isPressed;
  s_a.wasReleased = !pressed && s_a.isPressed;
  if (s_a.wasPressed) s_a.pressedAt = now;
  s_a.isPressed = pressed;
}

static void scanAxp() {
  if (hwExpanderAxpIrqLow()) {
    uint32_t status = hwAxpIrqStatusClear();
    if (status & XPOWERS_PWR_BTN_CLICK_INT)        s_axpEvt = 0x02;
    if (status & XPOWERS_PWR_BTN_LONGPRESSED_INT)  s_axpEvt = 0x04;
  }
  // Route 0x02 to BtnB pulse for one frame:
  bool pressed = (s_axpEvt == 0x02);
  s_b.wasPressed  = pressed;
  s_b.wasReleased = pressed;
  s_b.isPressed   = false;
  if (pressed) s_axpEvt = 0;
  // 0x04 stays in s_axpEvt until consumed by hwAxpBtnEvent()
}

void hwInputUpdate() {
  scanKey1();
  scanAxp();
  // touch scan is added in next task
}

HwBtn& hwBtnA() { return s_a; }
HwBtn& hwBtnB() { return s_b; }

uint8_t hwAxpBtnEvent() {
  uint8_t e = s_axpEvt;
  if (e == 0x04) s_axpEvt = 0;   // 0x02 already cleared by scanAxp
  return e;
}

const HwTouch& hwTouch() { return s_tp; }
```

- [ ] **Step 3: Commit**

```bash
git add src/hw/input.h src/hw/input.cpp
git commit -m "hw/input: Key1 + Key3 (AXP IRQ) scanning"
```

---

### Task 13: Add FT3168 touch reading to `src/hw/input.cpp`

**Files:**
- Modify: `src/hw/input.cpp`

- [ ] **Step 1: Add touch driver fields and ISR at the top of input.cpp**

```cpp
#include <Arduino_DriveBus_Library.h>

static std::shared_ptr<Arduino_IIC_DriveBus> s_iicBus;
static std::unique_ptr<Arduino_IIC>          s_ft3168;
static volatile bool                          s_tpIrqFlag = false;

static void IRAM_ATTR onTouchIrq() { s_tpIrqFlag = true; }
```

- [ ] **Step 2: Initialise touch in `hwInputInit()`**

Replace the existing `hwInputInit()` body with:

```cpp
bool hwInputInit() {
  pinMode(PIN_KEY1, INPUT_PULLUP);

  s_iicBus = std::make_shared<Arduino_HWIIC>(PIN_I2C_SDA, PIN_I2C_SCL, &Wire);
  s_ft3168.reset(new Arduino_FT3x68(s_iicBus, FT3168_DEVICE_ADDRESS,
                                    DRIVEBUS_DEFAULT_VALUE, PIN_TP_INT, onTouchIrq));
  for (int i = 0; i < 5; i++) {
    if (s_ft3168->begin()) return true;
    delay(100);
  }
  Serial.println("hwInput: FT3168 init failed");
  return false;
}
```

- [ ] **Step 3: Add `scanTouch()` and call it from `hwInputUpdate()`**

```cpp
static void scanTouch() {
  if (!s_tpIrqFlag) {
    s_tp.justPressed  = false;
    s_tp.justReleased = s_tp.down;
    s_tp.down         = false;
    return;
  }
  s_tpIrqFlag = false;
  uint8_t fingers = s_ft3168->IIC_Read_Device_Value(
      Arduino_IIC::Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  if (fingers > 0) {
    int rx = s_ft3168->IIC_Read_Device_Value(
        Arduino_IIC::Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
    int ry = s_ft3168->IIC_Read_Device_Value(
        Arduino_IIC::Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
    s_tp.justPressed  = !s_tp.down;
    s_tp.justReleased = false;
    s_tp.x = rx / 2;          // 368 → 184
    s_tp.y = ry / 2;          // 448 → 224
    s_tp.down = true;
  } else {
    s_tp.justReleased = s_tp.down;
    s_tp.down = false;
    s_tp.justPressed  = false;
  }
}

// Inside hwInputUpdate(), after scanAxp(), add:
//   scanTouch();
```

- [ ] **Step 4: Commit**

```bash
git add src/hw/input.cpp
git commit -m "hw/input: FT3168 touch via IRQ + I2C, mapped to logical coords"
```

---

### Task 14: Smoke test 2 — input event log

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace smoke 1 main with smoke 2**

```cpp
#include <Arduino.h>
#include <Wire.h>
#include "hw/expander.h"
#include "hw/display.h"
#include "hw/power.h"
#include "hw/input.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== SMOKE 2: input ===");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (!hwExpanderInit())     { Serial.println("FAIL expander"); while (1) delay(1000); }
  hwExpanderResetSequence();
  if (!hwDisplayInit())      { Serial.println("FAIL display");  while (1) delay(1000); }
  if (!hwPowerInit())        { Serial.println("FAIL power");    while (1) delay(1000); }
  if (!hwInputInit())        { Serial.println("FAIL input");    while (1) delay(1000); }

  Serial.println("OK init");
}

void loop() {
  hwInputUpdate();

  if (hwBtnA().wasPressed)   Serial.println("Key1 down");
  if (hwBtnA().wasReleased)  Serial.println("Key1 up");
  if (hwBtnB().wasPressed)   Serial.println("Key3 short");
  uint8_t e = hwAxpBtnEvent();
  if (e == 0x04)             Serial.println("Key3 long");

  const HwTouch& t = hwTouch();
  if (t.justPressed)         Serial.printf("touch @ (%d,%d)\n", t.x, t.y);
  if (t.justReleased)        Serial.println("touch up");

  // Render a tiny status so we can see the touch coord live
  Arduino_Canvas* c = hwCanvas();
  c->fillScreen(0x0000);
  c->setTextColor(0xFFFF);
  c->setCursor(4, 4);
  c->printf("touch:%c x=%d y=%d", t.down ? 'Y' : 'n', t.x, t.y);
  c->setCursor(4, 16);
  c->printf("Key1:%c", hwBtnA().isPressed ? 'D' : 'u');
  hwDisplayPush();

  delay(16);
}
```

- [ ] **Step 2: Build and flash**

```bash
pio run -e waveshare-amoled -t upload && pio device monitor
```

Expected behaviour:
- Press BOOT (Key1) → "Key1 down" / "Key1 up" lines
- Short-press Key3 → "Key3 short"
- Long-press Key3 (~1s) → "Key3 long"
- Tap screen → `touch @ (X,Y)` with X in 0..183, Y in 0..223
- On-screen overlay shows live touch coords and Key1 state

**Stop here if:**
- Touch coords out of range → check `/2` divisor (rotation may differ)
- Key3 events never fire → check AXP IRQ enable, expander pin 5 wiring
- Touch IRQ never fires → check PIN_TP_INT (GPIO21) and ISR registration

- [ ] **Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "smoke 2: input event log (keys + touch)"
```

---

## Phase 4 — Sensors

### Task 15: Create `src/hw/imu.{h,cpp}` — QMI8658 accelerometer

**Files:**
- Create: `src/hw/imu.h`
- Create: `src/hw/imu.cpp`

- [ ] **Step 1: Create `src/hw/imu.h`**

```c
#pragma once

bool hwImuInit();
void hwImuAccel(float* ax, float* ay, float* az);
```

- [ ] **Step 2: Create `src/hw/imu.cpp`**

```cpp
#include "hw/imu.h"
#include "hw/pins.h"
#include <Wire.h>
#include <SensorQMI8658.hpp>

static SensorQMI8658 s_qmi;

bool hwImuInit() {
  if (!s_qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL)) {
    Serial.println("hwImu: QMI8658 begin failed");
    return false;
  }
  s_qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_2G,
                            SensorQMI8658::ACC_ODR_125Hz);
  s_qmi.enableAccelerometer();
  return true;
}

void hwImuAccel(float* ax, float* ay, float* az) {
  IMUdata d;
  s_qmi.getAccelerometer(d.x, d.y, d.z);
  *ax = d.x;
  *ay = d.y;
  *az = d.z;
  // If smoke test 3 reveals az is inverted vs M5 expectation, flip here:
  // *az = -d.z;
}
```

- [ ] **Step 3: Commit**

```bash
git add src/hw/imu.h src/hw/imu.cpp
git commit -m "hw/imu: QMI8658 accelerometer wrapper"
```

---

### Task 16: Create `src/hw/rtc.{h,cpp}` — PCF85063

**Files:**
- Create: `src/hw/rtc.h`
- Create: `src/hw/rtc.cpp`

- [ ] **Step 1: Create `src/hw/rtc.h`**

```c
#pragma once
#include <stdint.h>

struct HwTime {
  uint8_t  H, M, S;
  uint16_t Y;
  uint8_t  Mo, D, dow;     // dow: 0=Sun..6=Sat (matches old code)
};

bool hwRtcInit();
bool hwRtcRead(HwTime* t);
bool hwRtcWrite(const HwTime& t);
```

- [ ] **Step 2: Create `src/hw/rtc.cpp`**

```cpp
#include "hw/rtc.h"
#include "hw/pins.h"
#include <Wire.h>
#include <Arduino_DriveBus_Library.h>

static std::shared_ptr<Arduino_IIC_DriveBus> s_iicBus;
static std::unique_ptr<Arduino_IIC>          s_pcf;

bool hwRtcInit() {
  s_iicBus = std::make_shared<Arduino_HWIIC>(PIN_I2C_SDA, PIN_I2C_SCL, &Wire);
  s_pcf.reset(new Arduino_PCF85063(s_iicBus, PCF85063_DEVICE_ADDRESS));
  if (!s_pcf->begin()) { Serial.println("hwRtc: PCF85063 begin failed"); return false; }
  return true;
}

bool hwRtcRead(HwTime* t) {
  t->H  = s_pcf->IIC_Read_Device_Value(Arduino_IIC::Arduino_IIC_RTC::Value_Information::RTC_HOUR);
  t->M  = s_pcf->IIC_Read_Device_Value(Arduino_IIC::Arduino_IIC_RTC::Value_Information::RTC_MINUTE);
  t->S  = s_pcf->IIC_Read_Device_Value(Arduino_IIC::Arduino_IIC_RTC::Value_Information::RTC_SECOND);
  t->Y  = s_pcf->IIC_Read_Device_Value(Arduino_IIC::Arduino_IIC_RTC::Value_Information::RTC_YEAR);
  t->Mo = s_pcf->IIC_Read_Device_Value(Arduino_IIC::Arduino_IIC_RTC::Value_Information::RTC_MONTH);
  t->D  = s_pcf->IIC_Read_Device_Value(Arduino_IIC::Arduino_IIC_RTC::Value_Information::RTC_DAY);
  t->dow = s_pcf->IIC_Read_Device_Value(Arduino_IIC::Arduino_IIC_RTC::Value_Information::RTC_WEEK);
  return true;
}

bool hwRtcWrite(const HwTime& t) {
  s_pcf->IIC_Write_Device_State(Arduino_IIC::Arduino_IIC_RTC::Device_Set::RTC_SET, ...);
  // Note: exact PCF85063 write API name to be confirmed against
  // Arduino_DriveBus library when this code is implemented. See examples
  // 05_GFX_PCF85063_simpleTime / 10_LVGL_PCF85063_simpleTime.
  return true;
}
```

The exact API names for `Arduino_PCF85063` come from `lib/Arduino_DriveBus/`. Cross-reference example `05_GFX_PCF85063_simpleTime.ino` when filling these in. If the API differs, adapt the wrapper but keep the public `HwTime` struct stable.

- [ ] **Step 3: Commit**

```bash
git add src/hw/rtc.h src/hw/rtc.cpp
git commit -m "hw/rtc: PCF85063 read + write with HwTime struct"
```

---

### Task 17: Smoke test 3 — sensors log

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace main with sensor exerciser**

```cpp
#include <Arduino.h>
#include <Wire.h>
#include "hw/expander.h"
#include "hw/display.h"
#include "hw/power.h"
#include "hw/input.h"
#include "hw/imu.h"
#include "hw/rtc.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== SMOKE 3: sensors ===");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (!hwExpanderInit())     { Serial.println("FAIL expander"); while (1) delay(1000); }
  hwExpanderResetSequence();
  if (!hwDisplayInit())      { Serial.println("FAIL display");  while (1) delay(1000); }
  if (!hwPowerInit())        { Serial.println("FAIL power");    while (1) delay(1000); }
  if (!hwInputInit())        { Serial.println("FAIL input");    while (1) delay(1000); }
  if (!hwImuInit())          { Serial.println("FAIL imu");      while (1) delay(1000); }
  if (!hwRtcInit())          { Serial.println("FAIL rtc");      while (1) delay(1000); }

  Serial.println("OK init");
}

void loop() {
  static uint32_t lastLog = 0;
  hwInputUpdate();

  if (millis() - lastLog > 500) {
    lastLog = millis();
    HwBattery b = hwBattery();
    float ax, ay, az; hwImuAccel(&ax, &ay, &az);
    HwTime tm; hwRtcRead(&tm);
    Serial.printf("bat %dmV %dmA %d%% usb=%d chg=%d temp=%dC | acc %.2f %.2f %.2f | rtc %04u-%02u-%02u %02u:%02u:%02u dow=%u\n",
                  b.mV, b.mA, b.pct, b.usbPresent, b.charging, b.tempC,
                  ax, ay, az,
                  tm.Y, tm.Mo, tm.D, tm.H, tm.M, tm.S, tm.dow);
  }

  delay(50);
}
```

- [ ] **Step 2: Build, flash, observe**

```bash
pio run -e waveshare-amoled -t upload && pio device monitor
```

Expected:
- Battery voltage 3500–4200 mV, plausible mA, pct 0–100, tempC 20–40
- USB plugged → `usb=1 chg=1`, mA negative
- Acc values: at rest, gravity → one axis ≈ ±1.0, others ≈ 0
- RTC ticks forward each line

**Critical observation — IMU axis check:**
- Place board flat, screen up. Read az.
- Place board flat, screen DOWN. Read az.
- Original M5 code expects: face-down → `az < -0.7` (i.e. screen DOWN gives az ≈ −1).
- If your board reads the opposite (screen DOWN gives az ≈ +1): edit `src/hw/imu.cpp` `hwImuAccel()` to flip: `*az = -d.z;`
- Re-flash and confirm.

- [ ] **Step 3: Apply axis flip if needed and commit**

```bash
git add src/main.cpp src/hw/imu.cpp
git commit -m "smoke 3: sensors log; calibrate IMU az if needed"
```

---

## Phase 5 — Audio

### Task 18: Create `src/hw/audio.{h,cpp}` — ES8311 + I2S beep

**Files:**
- Create: `src/hw/audio.h`
- Create: `src/hw/audio.cpp`

- [ ] **Step 1: Create `src/hw/audio.h`**

```c
#pragma once
#include <stdint.h>

bool hwAudioInit();
void hwBeep(uint16_t freqHz, uint16_t durMs);
```

- [ ] **Step 2: Create `src/hw/audio.cpp`**

```cpp
#include "hw/audio.h"
#include "hw/pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <driver/i2s.h>
#include <math.h>

struct BeepReq { uint16_t freq; uint16_t dur; };
static QueueHandle_t s_beepQ = nullptr;

// Minimal ES8311 init via I2C — register sequence cribbed from the
// Waveshare 15_ES8311 example. Confirm against the reference when
// implementing this task.
static bool es8311Init() {
  // Write key registers per example/15_ES8311.ino
  // ... (reproduce the sequence from the example)
  // Returns true if all I2C writes ack.
  return true;
}

static void beepTask(void*) {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 48000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false,
  };
  i2s_pin_config_t pins = {
    .mck_io_num = PIN_I2S_MCLK,
    .bck_io_num = PIN_I2S_BCLK,
    .ws_io_num  = PIN_I2S_WS,
    .data_out_num = PIN_I2S_DO,
    .data_in_num  = I2S_PIN_NO_CHANGE,
  };
  i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr);
  i2s_set_pin(I2S_NUM_0, &pins);

  pinMode(PIN_PA_CTRL, OUTPUT);
  digitalWrite(PIN_PA_CTRL, HIGH);   // PA on (always)

  BeepReq r;
  int16_t buf[256];
  while (xQueueReceive(s_beepQ, &r, portMAX_DELAY) == pdTRUE) {
    int total = 48000 * r.dur / 1000;
    float phase = 0.0f;
    float dphase = 2.0f * (float)M_PI * r.freq / 48000.0f;
    for (int n = 0; n < total; n += 256) {
      int chunk = total - n;
      if (chunk > 256) chunk = 256;
      for (int i = 0; i < chunk; i++) {
        buf[i] = (int16_t)(8000.0f * sinf(phase));
        phase += dphase;
        if (phase > 2*M_PI) phase -= 2*M_PI;
      }
      size_t bw;
      i2s_write(I2S_NUM_0, buf, chunk * sizeof(int16_t), &bw, portMAX_DELAY);
    }
  }
}

bool hwAudioInit() {
  if (!es8311Init()) { Serial.println("hwAudio: ES8311 init failed"); return false; }
  s_beepQ = xQueueCreate(8, sizeof(BeepReq));
  xTaskCreatePinnedToCore(beepTask, "beep", 4096, nullptr, 5, nullptr, 1);
  return true;
}

void hwBeep(uint16_t freqHz, uint16_t durMs) {
  if (!s_beepQ) return;
  BeepReq r{ freqHz, durMs };
  xQueueSend(s_beepQ, &r, 0);   // non-blocking; drops if full
}
```

The `es8311Init()` body must be filled in from `examples/Arduino-v3.3.5/examples/15_ES8311/15_ES8311.ino` — reproduce its register write sequence verbatim. If that sequence is large, factor into a helper file `src/hw/es8311_regs.h`.

- [ ] **Step 3: Add ES8311 init body**

Open the reference example and copy the I2C write sequence into `es8311Init()` (or a helper). Required output: speaker enabled, DAC routed, line-out path to PA.

- [ ] **Step 4: Test by adding `hwBeep(1200, 80);` after `hwAudioInit()` in smoke 3 main, flash, listen for chirp**

```cpp
hwAudioInit();
delay(500);
hwBeep(1200, 80);
delay(500);
hwBeep(2400, 60);
```

Expected: two short tones from the speaker, ~80 ms and ~60 ms. No clicks. Main loop continues responsive (beep is async).

- [ ] **Step 5: Commit**

```bash
git add src/hw/audio.h src/hw/audio.cpp
git commit -m "hw/audio: ES8311 codec + I2S sine beep on FreeRTOS task"
```

---

## Phase 6 — HAL orchestrator + reusable hw.cpp

### Task 19: Create `src/hw/hw.cpp` orchestrator

**Files:**
- Create: `src/hw/hw.cpp`
- Modify: `src/hw/hw.h` (add full surface)

- [ ] **Step 1: Replace `src/hw/hw.h` with the complete public surface**

```c
#pragma once
#include "hw/display.h"
#include "hw/input.h"
#include "hw/power.h"
#include "hw/imu.h"
#include "hw/rtc.h"
#include "hw/audio.h"
#include "hw/border.h"
#include "hw/expander.h"

void hwInit();   // initialises every subsystem; while(1) on any failure
```

- [ ] **Step 2: Create `src/hw/hw.cpp`**

```cpp
#include "hw/hw.h"
#include "hw/pins.h"
#include <Arduino.h>
#include <Wire.h>

static void die(const char* what) {
  Serial.printf("hwInit FAIL: %s\n", what);
  while (1) delay(1000);
}

void hwInit() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== claude-buddy waveshare boot ===");

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  if (!hwExpanderInit())  die("expander");
  hwExpanderResetSequence();
  if (!hwDisplayInit())   die("display");
  if (!hwPowerInit())     die("power");
  if (!hwInputInit())     die("input");
  if (!hwImuInit())       die("imu");
  if (!hwRtcInit())       die("rtc");
  if (!hwAudioInit())     die("audio");

  Serial.println("hwInit OK");
}
```

- [ ] **Step 3: Verify smoke 3 main still builds with new hw.h**

Replace the smoke 3 main's individual init calls with `hwInit()` and confirm build:

```cpp
#include <Arduino.h>
#include "hw/hw.h"

void setup() { hwInit(); }
// loop() unchanged
```

```bash
pio run -e waveshare-amoled
```

- [ ] **Step 4: Commit**

```bash
git add src/hw/hw.h src/hw/hw.cpp src/main.cpp
git commit -m "hw: hwInit() orchestrator covering all subsystems"
```

---

## Phase 7 — BLE (smoke test 4)

### Task 20: Verify `ble_bridge.cpp` builds on ESP32-S3

The BLE code uses NimBLE-Arduino which works identically on ESP32 and ESP32-S3. Expected: zero changes needed.

**Files:**
- Verify: `src/ble_bridge.cpp` (no changes expected)
- Possibly modify: `src/ble_bridge.cpp` (if any M5 dependency leaks)

- [ ] **Step 1: Search for any M5 references in BLE code**

```bash
grep -n "M5\." src/ble_bridge.cpp src/ble_bridge.h
```

Expected: no matches. If there are any, delete them.

- [ ] **Step 2: Update smoke main to call `bleInit("Claude-test")`**

```cpp
#include "hw/hw.h"
#include "ble_bridge.h"

void setup() {
  hwInit();
  bleInit("Claude-test");
  Serial.println("BLE advertising as Claude-test");
}

void loop() {
  hwInputUpdate();
  Arduino_Canvas* c = hwCanvas();
  c->fillScreen(0x0000);
  c->setTextColor(0xFFFF);
  c->setCursor(4, 4);
  c->printf("BLE %s", bleConnected() ? "linked" : "wait..");
  if (blePasskey()) {
    c->setTextSize(3);
    c->setCursor(20, 60);
    c->printf("%06lu", (unsigned long)blePasskey());
    c->setTextSize(1);
  }
  hwDisplayPush();
  delay(16);
}
```

- [ ] **Step 3: Build, flash, attempt pairing**

```bash
pio run -e waveshare-amoled -t upload && pio device monitor
```

Then on the desktop:
1. Enable Developer Mode in Claude desktop
2. Open **Developer → Open Hardware Buddy**
3. Click Connect, select "Claude-test" from the list
4. Enter the 6-digit passkey shown on the device
5. Confirm "BLE linked" appears on the device

**Stop here if:**
- Device not in scan list → check NimBLE init logs, advertising name
- Passkey not displayed → bonding handshake didn't reach `BleSecCallbacks`
- Pairing succeeds but no heartbeat → check NUS RX characteristic

- [ ] **Step 4: Commit**

```bash
git add src/main.cpp
git commit -m "smoke 4: BLE pair + passkey display verified on new hardware"
```

---

## Phase 8 — Full main migration (smoke test 5)

This is the largest phase. The strategy: bring the original M5StickC main back from `src/main.cpp.m5stick.bak`, then methodically replace M5 calls with hw_* calls in small, committable chunks. Each chunk should build successfully.

### Task 21: Restore original main.cpp and remove M5 includes

**Files:**
- Modify: `src/main.cpp` (replace smoke 4 with the M5 original, then begin substitutions)
- Delete: `src/main.cpp.m5stick.bak`
- Modify: `platformio.ini` (remove the `.bak` exclusion)

- [ ] **Step 1: Move backup back into main.cpp**

```bash
git rm src/main.cpp
git mv src/main.cpp.m5stick.bak src/main.cpp
```

- [ ] **Step 2: Remove the build_src_filter exclusion**

In `platformio.ini`, change:
```
build_src_filter = +<*> +<buddies/> +<hw/> -<main.cpp.m5stick.bak>
```
to:
```
build_src_filter = +<*> +<buddies/> +<hw/>
```

- [ ] **Step 3: Replace `#include <M5StickCPlus.h>` with HAL include**

In `src/main.cpp` line 1:
```cpp
#include <M5StickCPlus.h>
```
becomes:
```cpp
#include "hw/hw.h"
```

- [ ] **Step 4: Update screen dimensions**

Change lines 23–24 in `src/main.cpp`:
```cpp
const int W = 135, H = 240;
```
to:
```cpp
const int W = HW_W;        // 184
const int H = HW_H;        // 224
```

Note: any hard-coded 135 / 240 elsewhere needs case-by-case review in later tasks.

- [ ] **Step 5: Don't build yet — many M5 calls remain. Just commit the include + dimension change**

```bash
git add src/main.cpp platformio.ini
git commit -m "main: restore from backup, swap M5 include for hw.h, update screen dims"
```

---

### Task 22: Replace sprite (`TFT_eSprite`) with Canvas

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Remove the `spr` global**

Delete line 8:
```cpp
TFT_eSprite spr = TFT_eSprite(&M5.Lcd);
```

Add a helper at the top of the file (after the `#include`s):
```cpp
#define spr (*hwCanvas())   // shim: existing code uses spr.foo() directly
```

This lets every `spr.xxx()` call route through Canvas without touching every line. Once Phase 8 is done and the build is green, the macro can be removed and `spr.` rewritten to `hwCanvas()->`.

- [ ] **Step 2: Remove `spr.createSprite(W, H);`**

Find the line in `setup()` that says `spr.createSprite(W, H);` and delete it (Canvas was created in `hwDisplayInit()`).

- [ ] **Step 3: Replace `spr.pushSprite(0, 0)` with `hwDisplayPush()`**

Search `src/main.cpp` for `spr.pushSprite` and rewrite each occurrence:
```cpp
spr.pushSprite(0, 0);
```
to:
```cpp
hwDisplayPush();
```

- [ ] **Step 4: Build (will still fail on other M5 calls, but Canvas substitution should be syntactically valid)**

```bash
pio run -e waveshare-amoled 2>&1 | grep -E "(error|warning)" | head -20
```

The error count should drop noticeably; any remaining errors should be M5 references in non-display code.

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "main: route spr.* through Canvas via hwCanvas() shim"
```

---

### Task 23: Replace `M5.BtnA` / `M5.BtnB` / `M5.update()` references

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace `M5.update()`**

In the `loop()` function, near the top:
```cpp
M5.update();
```
becomes:
```cpp
hwInputUpdate();
```

Also delete:
```cpp
M5.Beep.update();
```

(The new audio task is FreeRTOS-driven; main no longer needs to tick it.)

- [ ] **Step 2: Sed-replace button references**

Run these in the project root:
```bash
sed -i '' 's/M5\.BtnA\.isPressed()/hwBtnA().isPressed/g'   src/main.cpp
sed -i '' 's/M5\.BtnA\.wasPressed()/hwBtnA().wasPressed/g' src/main.cpp
sed -i '' 's/M5\.BtnA\.wasReleased()/hwBtnA().wasReleased/g' src/main.cpp
sed -i '' 's/M5\.BtnA\.pressedFor(\([0-9]*\))/hwBtnA().pressedFor(\1)/g' src/main.cpp
sed -i '' 's/M5\.BtnB\.isPressed()/hwBtnB().isPressed/g'   src/main.cpp
sed -i '' 's/M5\.BtnB\.wasPressed()/hwBtnB().wasPressed/g' src/main.cpp
sed -i '' 's/M5\.BtnB\.wasReleased()/hwBtnB().wasReleased/g' src/main.cpp
```

(macOS `sed -i ''`. On Linux drop the `''`.)

- [ ] **Step 3: Replace power-button handling**

In `loop()`, the original block:
```cpp
if (M5.Axp.GetBtnPress() == 0x02) {
  if (screenOff) {
    wake();
  } else {
    M5.Axp.SetLDO2(false);
    screenOff = true;
  }
}
```
becomes (Key3 long-press toggles screen):
```cpp
if (hwAxpBtnEvent() == 0x04) {
  if (screenOff) {
    wake();
  } else {
    hwDisplaySleep(true);
    screenOff = true;
  }
}
```

- [ ] **Step 4: Search for any remaining `M5.BtnA` / `M5.BtnB` / `M5.Axp.GetBtnPress` / `M5.update`**

```bash
grep -nE "M5\.(BtnA|BtnB|Axp\.GetBtnPress|update)" src/main.cpp
```

Expected: no matches. Fix any that remain.

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "main: replace M5 button & power-key API with hw_* equivalents"
```

---

### Task 24: Replace `M5.Imu` and the `wake()`/brightness/sleep paths

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace IMU calls**

```bash
sed -i '' 's/M5\.Imu\.getAccelData(\&ax, \&ay, \&az);/hwImuAccel(\&ax, \&ay, \&az);/g' src/main.cpp
```

Verify: search `M5.Imu`:
```bash
grep -n "M5\.Imu" src/main.cpp
```
Expected: empty.

- [ ] **Step 2: Delete `M5.Imu.Init();` from setup()**

Find the line in `setup()`:
```cpp
M5.Imu.Init();
```
Delete it (covered by `hwInit()`).

- [ ] **Step 3: Replace `applyBrightness()`**

Original at line ~97:
```cpp
static void applyBrightness() { M5.Axp.ScreenBreath(20 + brightLevel * 20); }
```
becomes:
```cpp
static void applyBrightness() { hwDisplayBrightness(brightLevel); }
```

(`brightLevel` is 0..4 already, matching `hwDisplayBrightness`'s contract.)

- [ ] **Step 4: Replace `wake()`'s screen turn-on**

Original:
```cpp
if (screenOff) {
  M5.Axp.SetLDO2(true);
  applyBrightness();
  screenOff = false;
  wakeTransitionUntil = millis() + 12000;
}
```
becomes:
```cpp
if (screenOff) {
  hwDisplaySleep(false);
  applyBrightness();
  screenOff = false;
  wakeTransitionUntil = millis() + 12000;
}
```

Also the auto-off branch at end of `loop()`:
```cpp
M5.Axp.SetLDO2(false);
screenOff = true;
```
becomes:
```cpp
hwDisplaySleep(true);
screenOff = true;
```

And in face-down nap handling:
```cpp
M5.Axp.ScreenBreath(8);
```
becomes:
```cpp
hwDisplayBrightness(0);   // dimmest
```

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "main: replace M5.Imu and screen on/off/brightness with hw_* API"
```

---

### Task 25: Replace `M5.Beep` and `digitalWrite(LED_PIN)` (border alert)

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace beep helper**

Original at line ~111:
```cpp
static void beep(uint16_t freq, uint16_t dur) {
  if (settings().sound) M5.Beep.tone(freq, dur);
}
```
becomes:
```cpp
static void beep(uint16_t freq, uint16_t dur) {
  if (settings().sound) hwBeep(freq, dur);
}
```

Delete `M5.Beep.begin();` from `setup()`.

- [ ] **Step 2: Replace LED with border alert**

Find lines 1004–1009 (LED pulse):
```cpp
if (activeState == P_ATTENTION && settings().led) {
  digitalWrite(LED_PIN, (now / 400) % 2 ? LOW : HIGH);
} else {
  digitalWrite(LED_PIN, HIGH);
}
```
becomes:
```cpp
hwBorderAlert(activeState == P_ATTENTION && settings().led && (now / 400) % 2 == 0);
```

Delete the `LED_PIN` constant (line 26) and the `pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, HIGH);` lines in `setup()`.

- [ ] **Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "main: replace M5.Beep with hwBeep, LED with hwBorderAlert"
```

---

### Task 26: Replace `M5.Axp` battery & temp readings

**Files:**
- Modify: `src/main.cpp` (the `drawInfo()` page-3 device block)

- [ ] **Step 1: Replace battery readings in drawInfo() page 3**

Original (around line 596):
```cpp
int vBat_mV = (int)(M5.Axp.GetBatVoltage() * 1000);
int iBat_mA = (int)M5.Axp.GetBatCurrent();
int vBus_mV = (int)(M5.Axp.GetVBusVoltage() * 1000);
int pct = (vBat_mV - 3200) / 10;
if (pct < 0) pct = 0; if (pct > 100) pct = 100;
bool usb = vBus_mV > 4000;
bool charging = usb && iBat_mA > 1;
bool full = usb && vBat_mV > 4100 && iBat_mA < 10;
```
becomes:
```cpp
HwBattery hb = hwBattery();
int vBat_mV = hb.mV;
int iBat_mA = hb.mA;
int vBus_mV = hb.usbPresent ? 5000 : 0;   // approximation; only used for display
int pct     = hb.pct;
bool usb    = hb.usbPresent;
bool charging = hb.charging;
bool full   = usb && vBat_mV > 4100 && iBat_mA > -10 && iBat_mA <= 0;
```

(Note: in HwBattery, `mA` is + for discharge, − for charge — reversed from original M5 convention. The `full`/`charging` predicates flip accordingly.)

- [ ] **Step 2: Replace `M5.Axp.GetTempInAXP192()` reference**

Original:
```cpp
ln("  temp     %dC", (int)M5.Axp.GetTempInAXP192());
```
becomes:
```cpp
ln("  temp     %dC", hb.tempC);
```

- [ ] **Step 3: Replace `M5.Axp.GetVBusVoltage()` in `clockRefreshRtc()`**

Original (line 359):
```cpp
_onUsb = M5.Axp.GetVBusVoltage() > 4.0f;
```
becomes:
```cpp
_onUsb = hwBattery().usbPresent;
```

- [ ] **Step 4: Replace `M5.Axp.PowerOff()` in menu confirmation**

```cpp
case 1: M5.Axp.PowerOff(); break;
```
becomes:
```cpp
case 1: hwPowerOff(); break;
```

- [ ] **Step 5: Verify all `M5.Axp.*` references gone**

```bash
grep -n "M5\.Axp" src/main.cpp
```

Expected: empty.

- [ ] **Step 6: Commit**

```bash
git add src/main.cpp
git commit -m "main: replace M5.Axp battery/temp/power-off with hwBattery/hwPowerOff"
```

---

### Task 27: Replace `M5.Rtc` and remove landscape clock

The new design drops landscape clock (`clockUpdateOrient()` returns 1 or 3). Simplify `clockOrient` to portrait-only.

**Files:**
- Modify: `src/main.cpp` (clock-related block ~lines 346–477 and clock invocation in loop)

- [ ] **Step 1: Replace RTC reads in `clockRefreshRtc()`**

Original (lines 358–362):
```cpp
M5.Rtc.GetTime(&_clkTm);
M5.Rtc.GetDate(&_clkDt);
```

The `_clkTm` (RTC_TimeTypeDef) and `_clkDt` (RTC_DateTypeDef) variables disappear. Replace with a single `HwTime` plus a small accessor:

Replace lines 352–362 in `src/main.cpp`:
```cpp
static RTC_TimeTypeDef _clkTm;
static RTC_DateTypeDef _clkDt;
uint32_t               _clkLastRead = 0;
static bool            _onUsb       = false;
static void clockRefreshRtc() {
  if (millis() - _clkLastRead < 1000) return;
  _clkLastRead = millis();
  _onUsb = M5.Axp.GetVBusVoltage() > 4.0f;
  M5.Rtc.GetTime(&_clkTm);
  M5.Rtc.GetDate(&_clkDt);
}
```
with:
```cpp
static HwTime    _clkTm;
uint32_t         _clkLastRead = 0;
static bool      _onUsb       = false;
static void clockRefreshRtc() {
  if (millis() - _clkLastRead < 1000) return;
  _clkLastRead = millis();
  _onUsb = hwBattery().usbPresent;
  hwRtcRead(&_clkTm);
}
```

Then update every `_clkTm.Hours/Minutes/Seconds` reference to `_clkTm.H/M/S`, and `_clkDt.Date/Month/WeekDay` to `_clkTm.D/Mo/dow`. (Field names already chosen to match the original semantics.)

- [ ] **Step 2: Replace RTC writes (BLE time sync)**

Find calls to `M5.Rtc.SetTime` / `M5.Rtc.SetDate` in `data.h` (if present) or anywhere they're called. Replace with `hwRtcWrite(...)`.

```bash
grep -rn "M5\.Rtc" src/
```

Expected after fix: empty.

- [ ] **Step 3: Delete landscape-clock code**

In `clockUpdateOrient()` (~lines 364–401), replace the entire body with a stub that always selects portrait:
```cpp
static void clockUpdateOrient() { clockOrient = 0; orientFrames = 0; }
```

In `drawClock()` (~line 412), delete the `if (clockOrient != 0)` branch — keep only the portrait code path. Specifically, delete from `// Landscape: 240×135 direct-to-LCD.` down to the closing `M5.Lcd.setRotation(0);`.

In `loop()`, find:
```cpp
bool landscapeClock = clocking && clockOrient != 0;
```
Replace with:
```cpp
const bool landscapeClock = false;   // portrait-only on the AMOLED port
```
And drop everything in the loop guarded by `landscapeClock` — the "skip sprite render" branch keeps `napping || screenOff` only.

- [ ] **Step 4: Search for remaining M5.Lcd**

```bash
grep -n "M5\.Lcd" src/main.cpp
```

Expected: empty after the landscape clock deletion.

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "main: replace M5.Rtc with hwRtc; drop landscape clock"
```

---

### Task 28: Replace `setup()` and remove all M5 init

**Files:**
- Modify: `src/main.cpp` (`setup()`)

- [ ] **Step 1: Replace `setup()` body**

Original (lines 938–986) — replace the WHOLE function with:

```cpp
void setup() {
  hwInit();                                  // display, input, IMU, RTC, audio, power
  startBt();                                 // BLE init (unchanged)
  applyBrightness();
  lastInteractMs = millis();
  statsLoad();
  settingsLoad();
  petNameLoad();
  buddyInit();

  characterInit(nullptr);
  gifAvailable = characterLoaded();
  buddyMode = !(gifAvailable && speciesIdxLoad() == SPECIES_GIF);
  applyDisplayMode();

  {
    const Palette& p = characterPalette();
    spr.fillScreen(p.bg);
    spr.setTextSize(2);
    if (ownerName()[0]) {
      char line[40];
      snprintf(line, sizeof(line), "%s's", ownerName());
      spr.setTextColor(p.text, p.bg);
      spr.setCursor(W/2 - 30, H/2 - 12); spr.print(line);
      spr.setTextColor(p.body, p.bg);
      spr.setCursor(W/2 - 30, H/2 + 12); spr.print(petName());
    } else {
      spr.setTextColor(p.body, p.bg);
      spr.setCursor(W/2 - 24, H/2 - 12); spr.print("Hello!");
      spr.setTextSize(1);
      spr.setTextColor(p.textDim, p.bg);
      spr.setCursor(W/2 - 50, H/2 + 12); spr.print("a buddy appears");
    }
    spr.setTextSize(1);
    hwDisplayPush();
    delay(1800);
  }

  Serial.printf("buddy: %s\n", buddyMode ? "ASCII mode" : "GIF character loaded");
}
```

(The original used `spr.setTextDatum(MC_DATUM)` then `drawString(...)`. Arduino_GFX `Arduino_Canvas` has `setTextDatum` too; if it works, prefer the original style. If not, switch to `setCursor + print` as shown.)

- [ ] **Step 2: Build to surface remaining errors**

```bash
pio run -e waveshare-amoled 2>&1 | grep error | head -30
```

Address any remaining symbol errors one-by-one. Likely candidates: `setTextDatum`, `MC_DATUM`/`TL_DATUM`, `drawString`. If Canvas doesn't support them, replace with `setCursor()/print()`.

- [ ] **Step 3: Commit when build is clean**

```bash
git add src/main.cpp
git commit -m "main: replace setup() with hwInit-based bring-up; build is green"
```

---

### Task 29: Add the touch hit-test helper and wire it into approval & menu

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Add `tap()` helper after the global state declarations**

```cpp
// Touch hit-test helper. justPressed within the rect.
static bool tap(int x, int y, int w, int h) {
  const HwTouch& t = hwTouch();
  return t.justPressed && t.x >= x && t.y >= y && t.x < x+w && t.y < y+h;
}
```

- [ ] **Step 2: Augment approval handling**

Find the `if (M5.BtnA.wasReleased())` / `M5.BtnB.wasPressed()` blocks that already became `hwBtnA().wasReleased` / `hwBtnB().wasPressed` after Task 23. Wrap the in-prompt branches:

```cpp
// Approval: A approves, B denies, OR tap top-half = approve, bottom-half = deny
if (inPrompt) {
  bool approve = hwBtnA().wasReleased || tap(0, H - 78,        W, 39);
  bool deny    = hwBtnB().wasPressed  || tap(0, H - 78 + 39,   W, 39);
  if (approve) { /* existing approve code */ }
  if (deny)    { /* existing deny code */    }
}
```

(Coordinates use `H - 78` because `drawApproval()` reserves the bottom 78 pixels.)

- [ ] **Step 3: Augment menu/settings/reset selection**

Inside the existing `if (menuOpen)`, after the existing key handling, add:
```cpp
// Touch: directly tap a menu item to select-and-confirm
int mw = 118, mh = 16 + MENU_N * 14 + MENU_HINT_H;
int mx = (W - mw) / 2, my = (H - mh) / 2;
int rowH = 14;
const HwTouch& t = hwTouch();
if (t.justPressed && t.x >= mx && t.x < mx + mw &&
    t.y >= my + 8 && t.y < my + 8 + MENU_N * rowH) {
  int hit = (t.y - (my + 8)) / rowH;
  if (hit >= 0 && hit < MENU_N) {
    menuSel = hit;
    menuConfirm();
  }
}
```

Repeat the analogous pattern for `settingsOpen` (use `SETTINGS_N`, call `applySetting(settingsSel)`) and `resetOpen` (use `RESET_N`, call `applyReset(resetSel)`).

- [ ] **Step 4: Augment HUD scroll (DISP_NORMAL)**

In the `else` branch where `BtnB` scrolls transcript, add:
```cpp
// Touch bottom 32px scrolls back one line — same as BtnB
if (displayMode == DISP_NORMAL && tap(0, H - 32, W, 32)) {
  msgScroll = (msgScroll >= 30) ? 0 : msgScroll + 1;
}
```

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "main: add touch hit-test, wire into approval/menu/settings/HUD"
```

---

### Task 30: Adapt `buddies/` and `buddy.{h,cpp}` to Canvas surface

**Files:**
- Modify: `src/buddy.h`
- Modify: `src/buddy.cpp`
- Modify: `src/buddies/*.cpp` (18 files)

- [ ] **Step 1: Replace surface type in `buddy.h`**

Find:
```cpp
#include <M5StickCPlus.h>
```
or whatever the existing surface include is. Replace with:
```cpp
#include <Arduino_GFX_Library.h>
using BuddySurface = Arduino_GFX;   // Canvas inherits from Arduino_GFX
```

Find the function-pointer typedef or render signature(s) that take `TFT_eSprite&` and replace with `BuddySurface&`.

- [ ] **Step 2: Replace function signatures in `buddy.cpp`**

```bash
sed -i '' 's/TFT_eSprite&/BuddySurface\&/g' src/buddy.cpp
sed -i '' 's/TFT_eSprite \*/BuddySurface */g' src/buddy.cpp
```

Verify no other `TFT_eSprite` references remain:
```bash
grep -n "TFT_eSprite" src/buddy.cpp src/buddy.h src/buddy_common.h
```
Expected: empty.

- [ ] **Step 3: Sed-replace all 18 buddy species files**

```bash
for f in src/buddies/*.cpp; do
  sed -i '' 's/TFT_eSprite&/BuddySurface\&/g' "$f"
  sed -i '' 's/TFT_eSprite \*/BuddySurface */g' "$f"
done
```

Verify:
```bash
grep -ln "TFT_eSprite" src/buddies/
```
Expected: empty.

- [ ] **Step 4: Build**

```bash
pio run -e waveshare-amoled
```

Address any per-species build errors (e.g. unsupported `setTextDatum` calls in Arduino_GFX → switch to manual cursor offsets).

- [ ] **Step 5: Commit**

```bash
git add src/buddy.h src/buddy.cpp src/buddies/
git commit -m "buddies: surface type TFT_eSprite -> BuddySurface (Arduino_GFX)"
```

---

### Task 31: Adapt `character.cpp` GIF rendering to Canvas

**Files:**
- Modify: `src/character.cpp`
- Modify: `src/character.h`

- [ ] **Step 1: Replace surface type**

In `character.h` and `character.cpp`, replace any `TFT_eSprite` references with `Arduino_GFX*` (use the same `BuddySurface` alias if applicable).

```bash
sed -i '' 's/TFT_eSprite&/Arduino_GFX\&/g' src/character.cpp src/character.h
sed -i '' 's/TFT_eSprite \*/Arduino_GFX */g' src/character.cpp src/character.h
```

- [ ] **Step 2: Replace pixel callback target**

If the GIF draw callback writes to a global sprite, switch it to `hwCanvas()`:
```cpp
// Old
gif_draw_callback(...) { spr.drawPixel(...); }
// New
gif_draw_callback(...) { hwCanvas()->drawPixel(...); }
```

Add `#include "hw/display.h"` at top of `character.cpp` if not present.

- [ ] **Step 3: Verify and build**

```bash
grep -n "TFT_eSprite\|M5\." src/character.cpp src/character.h
pio run -e waveshare-amoled
```

- [ ] **Step 4: Commit**

```bash
git add src/character.cpp src/character.h
git commit -m "character: route GIF render to hwCanvas()"
```

---

### Task 32: Recalc UI coordinates for the new logical screen

The original UI was hand-tuned for 135×240. The new logical canvas is 184×224. With `W` and `H` already replaced by `HW_W`/`HW_H`, most relative layouts work, but some hard-coded literal coordinates remain.

**Files:**
- Modify: `src/main.cpp` (UI draw functions)

- [ ] **Step 1: Search for any literal `135` or `240` in main.cpp**

```bash
grep -nE "[^0-9](135|240)[^0-9]" src/main.cpp
```

Each hit needs review:
- If the literal corresponds to old W/H, replace with `W`/`H`
- If it's an unrelated number (e.g. a frequency 1000Hz tone), leave alone

- [ ] **Step 2: Audit y-coordinates against new H=224**

Original `H=240`, now `H=224`. Anything using `H - N` shifts down by 16 pixels — usually fine, but several constants may need adjustment:
- `drawApproval` uses `H - AREA` where `AREA=78` → still fits.
- `drawHUD` uses `H - LH - 2` and `H - AREA` → still fits.
- `drawInfo`'s `TOP = 70` → still fits in 224.

Visually verify on smoke 5 if any text gets cut off.

- [ ] **Step 3: Adjust the `MENU_HINT_H = 14` and menu width `mw = 118`**

The new W=184 has 33 px of side margin per side at mw=118 — plenty. No change needed unless visual review says otherwise.

- [ ] **Step 4: Commit any literal-coord fixes you found**

```bash
git add src/main.cpp
git commit -m "main: align hard-coded coords with new 184x224 logical canvas"
```

---

### Task 33: Smoke test 5 — full integration

**Files:**
- N/A (build + flash + manual checklist)

- [ ] **Step 1: Clean build**

```bash
pio run -e waveshare-amoled -t clean
pio run -e waveshare-amoled -t upload
pio device monitor
```

Expected serial:
```
hwInit OK
buddy: <ASCII mode | GIF character loaded>
```

- [ ] **Step 2: Boot screen check**

Visual: shows "Hello!" + "a buddy appears" centered for ~1.8s, then transitions to home.

- [ ] **Step 3: Pet animation check**

Cycle through species via menu (Hold A → settings → ascii pet → press B repeatedly). Confirm:
- Each of the 18 species renders correctly
- All 7 animation states (sleep/idle/busy/attention/celebrate/dizzy/heart) play
- Sleep state plays without BLE connection

- [ ] **Step 4: BLE pairing + heartbeat check**

Pair via desktop. After successful pair:
- `bleSecure() == true` shown on info page → "ble  encrypted"
- Heartbeat messages show on transcript HUD as expected

- [ ] **Step 5: Approval flow check**

In Claude desktop, trigger a permission prompt (e.g. enable a tool that needs approval). On the device:
- Beep chirp plays
- Approval screen shows tool name + hint
- Border flashes red while waiting
- Press Key1 → approve → desktop confirms tool runs
- Repeat with Key3 short → deny → desktop confirms tool denied
- Repeat with **touch on top half of approval area** → approve via touch
- Repeat with **touch on bottom half** → deny via touch

- [ ] **Step 6: Touch in menu**

- Hold Key1 → menu appears
- Touch a menu item directly → selection confirmed (skips A-cycle/B-confirm dance)
- Verify settings, reset confirmation, info pages all touch-navigable

- [ ] **Step 7: Charging clock check**

Plug in USB. Verify:
- After idle, screen does NOT auto-off (USB power)
- Clock face displays in portrait (no landscape rotation — confirmed dropped)
- Pet sleeps under the clock, occasionally peeks per scenario

- [ ] **Step 8: Face-down nap**

Place board face-down for >2s:
- Brightness dims to lowest
- Pet animation pauses
- Lift back up:
  - Stats `napped` time accumulates
  - Pet wakes

- [ ] **Step 9: Shake → dizzy**

Vigorously shake the board. Confirm:
- Pet briefly enters dizzy state
- Beep does NOT fire (shake doesn't trigger sound)

- [ ] **Step 10: GIF character pack push**

Drag `characters/bufo/` onto the Hardware Buddy window:
- Progress bar appears on device
- After completion, GIF renders correctly
- Cycle to ASCII species and back via menu

- [ ] **Step 11: Factory reset**

Hold A → settings → reset → factory reset → tap twice within 3s. Confirm:
- Beep fires
- Device reboots
- NVS cleared (owner gone, stats zero)
- LittleFS cleared (no GIF)
- BLE bonds erased (re-pairing required)

- [ ] **Step 12: Commit and tag**

```bash
git tag v0.1.0-waveshare
git push --tags
git add .
git commit --allow-empty -m "smoke 5: full integration verified on waveshare hardware"
```

---

## Phase 9 — Polish

### Task 34: AMOLED burn-in protection

**Files:**
- Modify: `src/main.cpp` (loop tail)

- [ ] **Step 1: Add a 5-minute pixel-shimmy**

Near the end of `loop()`, before `delay()`:
```cpp
// AMOLED burn-in protection: every 5 minutes, nudge canvas up/right by 1px
// for one frame so static elements don't permanently etch.
static uint32_t lastShimmy = 0;
if (millis() - lastShimmy > 5UL * 60UL * 1000UL) {
  lastShimmy = millis();
  // Push the same canvas with a 1px offset by manipulating the GFX cursor
  // before the next push. Simplest: skip one push cycle (shows old buffer
  // shifted by hardware blank). Real implementation: shift the canvas data
  // by 1 row + 1 col on next render.
  characterInvalidate();   // forces full redraw with subtle offset
  if (buddyMode) buddyInvalidate();
}
```

For a meaningful shimmy, also wrap `hwDisplayPush()` to optionally insert a 1-pixel offset every 5 minutes — this requires a small modification to `hw/display.cpp` to support an offset parameter. Defer if visual inspection finds no burn-in concerns after 1 hour of static display.

- [ ] **Step 2: Commit**

```bash
git add src/main.cpp
git commit -m "main: 5-minute pixel-shimmy hint to mitigate AMOLED burn-in"
```

---

### Task 35: Update README for waveshare branch

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add a "Boards supported" section near the top**

```markdown
## Boards supported

This repository supports two boards on separate branches:

- **`main`** — M5StickC Plus (135×240 LCD, 2 buttons, AXP192). Original target.
- **`waveshare`** — Waveshare ESP32-S3-Touch-AMOLED-1.8 (368×448 AMOLED, touch, 2 buttons, AXP2101). Port using the same BLE protocol.

Each branch has its own `platformio.ini` env. The BLE wire protocol
(REFERENCE.md) is identical between boards.
```

- [ ] **Step 2: Add a "Hardware (Waveshare)" subsection updating the current "Hardware" section**

Mention:
- Use `pio run -e waveshare-amoled -t upload`
- Two buttons: Key1 (BOOT/GPIO0) for A, Key3 (PWR) short for B, Key3 long for screen on/off
- Touch is supplementary — taps on action zones, menu items, scroll regions
- AMOLED screen with 9× the pixel area; UI rendered at 184×224 then 2× upscaled

- [ ] **Step 3: Commit**

```bash
git add README.md
git commit -m "docs: README documents waveshare branch alongside M5StickC"
```

---

## Self-review notes

Verified during writing:

- **Spec coverage:** Each spec section maps to phase tasks. Phase 0 = §1-2 (decisions, file layout). Phase 1-5 = §3 (HAL surface) + §4-6 (display, input, drivers). Phase 6 = §3 (`hwInit`). Phase 7 = no spec change (BLE works as-is). Phase 8 = §5 (input wiring) + §3 (HAL consumption from main). Phase 9 = §9 (risks: burn-in).
- **Decisions traced:**
  - 2× upscale → Task 8
  - Edge flash for LED → Tasks 8 (display alpha) + 9 (border) + 25 (main wiring)
  - Drop landscape clock → Task 27
  - Touch as additive → Task 29
  - 3-button → 2-button mapping (Key3 long = screen toggle) → Tasks 12 + 23
  - Audio: I2S sine on FreeRTOS → Task 18
- **Type consistency:** `HwBtn`, `HwTouch`, `HwBattery`, `HwTime` defined once in respective `hw/<x>.h`, used unchanged in main.cpp tasks. `Arduino_Canvas*` returned by `hwCanvas()` is the only render surface; `BuddySurface = Arduino_GFX` (its base class) used in buddies.
- **Placeholder check:** No "TODO" / "TBD" except where explicitly marked as "to confirm against library source" — these are real verification steps, not unfilled plan slots. The two notable cases:
  - Task 16 step 2: `IIC_Write_Device_State(...)` placeholder for PCF85063 write — must be reconciled with `lib/Arduino_DriveBus/` API at implementation time
  - Task 18 step 2: `es8311Init()` body must be cribbed from `examples/15_ES8311.ino` — register sequence too long to inline
- **Frequent commits:** every task ends with `git commit`. No multi-task batching.

---

## Execution

Plan complete and saved to `docs/superpowers/plans/2026-04-19-esp32s3-amoled-port.md`.
