# ESP32-S3-Touch-AMOLED-1.8 移植设计

**日期**:2026-04-19
**分支**:`waveshare`(待创建)
**目标硬件**:Waveshare ESP32-S3-Touch-AMOLED-1.8(368×448 AMOLED + FT3168 触摸 + AXP2101 + QMI8658 + PCF85063 + ES8311)
**范围**:与当前 M5StickC Plus 版本**全功能等价**移植

## 1. 决策汇总

| 项 | 选择 |
|---|---|
| 范围 | 全功能等价(18 个 ASCII 物种 / GIF / 菜单 / 设置 / info 6 页 / 宠物状态 / 充电时钟 / 摇晃 / 面朝下 / BLE 配对 / 角色包推送) |
| 屏幕 | 368×448 AMOLED(确认值,非 README 的 488) |
| 输入 | Key1 (GPIO0) → BtnA;Key3 短按 (AXP IRQ) → BtnB;Key3 长按 (AXP IRQ) → 切屏(替代 M5 PWR 短按);FT3168 触屏作为辅助事件源 |
| 显示框架 | Arduino_GFX + Canvas,逻辑 184×224,push 时 2× nearest-neighbor 上屏 |
| 仓库 | 新分支 `waveshare`,`main` 保持 M5StickC 版 |
| 抽象方式 | 薄 HAL(`src/hw/`),main.cpp 不直接 include 板级库 |
| 声音 | ES8311 + I2S 播 sine,FreeRTOS 任务非阻塞 |
| 注意 LED | AMOLED 屏 4px 红色边框闪烁(无独立 LED) |
| 横屏时钟 | 删除(368×448 接近正方形,横屏意义不大) |
| RTC | PCF85063 接上,充电时钟功能保留 |
| SD / 麦克风 | 不接,保持范围聚焦 |

## 2. 架构与文件布局

### 分支策略

- `main`:冻结在当前 M5StickC 版本,只接 bug 修复
- `waveshare`:全部移植工作。不 rebase 回 main(两板的 main.cpp 差异过大)
- 公共改动(BLE 协议、stats 算法)在 main 改一次,cherry-pick 到 waveshare

### `waveshare` 分支的 `src/`

```
src/
  main.cpp              改 — 替换 M5 调用为 hw_*(),坐标 W=184/H=224
  ble_bridge.{cpp,h}    不动 — NimBLE 在 ESP32-S3 一样可用
  data.h                不动 — JSON 协议无关硬件
  stats.h               不动 — NVS 在 ESP32-S3 一样
  buddy.{cpp,h}         小改 — surface 类型从 TFT_eSprite 换为 Arduino_GFX
  buddy_common.h        不动
  buddies/              小改 — 18 个文件函数签名换 surface 类型,sed 替换 spr.
  character.{cpp,h}     小改 — GIF 解码不变,渲染目标改为 hwCanvas()
  xfer.h                不动
  hw/                   新增 — 所有板级驱动封装
    hw.h                总入口,声明所有 hw_* 函数
    display.cpp         Arduino_GFX + Canvas + 2× push
    input.cpp           Key1/Key3 扫描、AXP IRQ 读取、FT3168 触摸
    power.cpp           AXP2101 电池/充电/关机
    imu.cpp             QMI8658 加速度
    rtc.cpp             PCF85063 时钟
    audio.cpp           ES8311 + I2S sine 蜂鸣
    expander.cpp        TCA9554 复用引脚
    border.cpp          AMOLED 边框闪红
    pins.h              所有 GPIO 常量
```

### 抽象边界

`main.cpp` 不能 `#include` `Arduino_GFX_Library.h`、`XPowersLib.h`、`SensorLib`。只能 `#include "hw/hw.h"`。

- 板级换代只动 `hw/`
- main.cpp 看上去仍像 M5Stick 版,只是 API 名字变了
- `buddies/`、`character.cpp`、`ble_bridge.cpp` 通过 `Arduino_GFX*` 抽象基类(Canvas 继承自此)接收绘图目标

### 第三方库依赖

```
moononournation/GFX Library for Arduino
lewisxhe/XPowersLib
lewisxhe/SensorLib
adafruit/Adafruit BusIO
bitbank2/AnimatedGIF @ ^2.1.1
bblanchon/ArduinoJson @ ^7.0.0
h2zero/NimBLE-Arduino @ ^1.4
```

例程附带的库(不在 PIO registry):
- `lib/Arduino_DriveBus/`(FT3168 + PCF85063 + I2C bus 抽象)
- `lib/Adafruit_XCA9554/`(I/O 扩展器)

来源:`/Users/yadongxie/Downloads/ESP32-S3-Touch-AMOLED-1.8/examples/Arduino-v3.3.5/libraries/`,版权信息保留。

## 3. HAL 接口(`src/hw/hw.h`)

main.cpp 唯一会调用的板级 API。完整签名:

```cpp
// 显示
constexpr int HW_W = 184;
constexpr int HW_H = 224;
void hwDisplayInit();
void hwDisplayPush();
void hwDisplayBrightness(uint8_t lvl_0_4);   // 0..4 → 50/100/150/200/255
void hwDisplaySleep(bool off);
Arduino_Canvas* hwCanvas();

// 输入(按键 + 触摸)
struct HwBtn {
  bool isPressed;
  bool wasPressed;     // 刚按下
  bool wasReleased;    // 刚松开
  uint32_t pressedAt;  // 内部用
  bool pressedFor(uint32_t ms);
};
HwBtn& hwBtnA();          // Key1(GPIO0)
HwBtn& hwBtnB();          // Key3 短按(AXP IRQ 脉冲事件,不可 pressedFor)
uint8_t hwAxpBtnEvent();  // 0 / 0x02 短按 / 0x04 长按

struct HwTouch {
  bool down;
  int16_t x, y;        // 已映射到 HW_W × HW_H 逻辑空间
  bool justPressed;
  bool justReleased;
};
const HwTouch& hwTouch();

void hwInputUpdate();   // 每帧调

// IMU
void hwImuInit();
void hwImuAccel(float* ax, float* ay, float* az);

// 电源
struct HwBattery {
  int   mV;
  int   mA;        // + 放电, − 充电
  int   pct;
  bool  usbPresent;
  bool  charging;
  int   tempC;
};
HwBattery hwBattery();
void  hwPowerOff();

// RTC
struct HwTime { uint8_t H, M, S; uint16_t Y; uint8_t Mo, D, dow; };
bool hwRtcRead(HwTime* t);
bool hwRtcWrite(const HwTime& t);

// 蜂鸣
void hwAudioInit();
void hwBeep(uint16_t freqHz, uint16_t durMs);

// 边框 alert(代替 LED)
void hwBorderAlert(bool on);

// 总初始化
void hwInit();
```

### 关键设计点

1. **`HwBtn` 模拟 M5.BtnA 接口**:`isPressed/wasPressed/pressedFor()` 全保留,main.cpp 的按键逻辑 sed 替换即可
2. **触摸映射到逻辑坐标系**:FT3168 报告 0..367 / 0..447,内部除 2 给 main 看到 0..183 / 0..223,与 Canvas 坐标一致
3. **`hwBeep` 非阻塞**:FreeRTOS task + I2S DMA。main.cpp 调用方式不变
4. **`hwBorderAlert` 替代 LED**:`hwDisplayPush()` 内部检测 alert 状态,push 完 canvas 后叠加 4px 红色边框
5. **`hwAxpBtnEvent` 模拟 `M5.Axp.GetBtnPress()`**:AXP IRQ 经 TCA9554 间接读到,语义保持

## 4. 显示与渲染

### 初始化序列(严格顺序)

LCD/触摸 reset 引脚不直接接 ESP32,而是接在 TCA9554 I/O 扩展器上:

```
1. Wire.begin(IIC_SDA=15, IIC_SCL=14)
2. expander.begin(0x20)
3. expander.pinMode(0..2, OUTPUT)
4. 拉低 0/1/2 → delay(20) → 拉高 0/1/2  // 释放 reset
5. ft3168->begin()
6. gfx->begin()
7. gfx->setBrightness(0)  // 防白闪
8. canvas = new Arduino_Canvas(HW_W, HW_H, gfx)
9. canvas->begin()        // ps_malloc 在 PSRAM
10. gfx->setBrightness(150)  // 默认中等亮度;main 在 settingsLoad 后调 hwDisplayBrightness 覆盖
```

任何一步失败:串口报错 + `while(1)`(开发期 fail-fast)。

### Canvas 与 2× push

```cpp
static Arduino_Canvas* canvas;     // 184×224×16bit = 80KB,PSRAM
static uint16_t* lineBuf;          // 368×2 = 1472B,内部 RAM(快)

void hwDisplayPush() {
  uint16_t* src = canvas->getFramebuffer();
  for (int y = 0; y < HW_H; y++) {
    uint16_t* row = src + y * HW_W;
    for (int x = 0; x < HW_W; x++) {
      uint16_t c = row[x];
      lineBuf[x*2] = c;
      lineBuf[x*2 + 1] = c;
    }
    // 边框 alert:y < 4 || y >= HW_H-4 时 lineBuf 全填 RED
    // 否则正常画两次行
    gfx->draw16bitRGBBitmap(0, y*2,     lineBuf, 368, 1);
    gfx->draw16bitRGBBitmap(0, y*2 + 1, lineBuf, 368, 1);
  }
}
```

### 性能预算

- 全帧 push:184×224 像素读 + 368×448 像素写 = ~250KB SPI
- QSPI @ 80MHz 理论 ~30ms/帧,实测预期 25–35 fps
- M5Stick 当前 ~60 fps,会减半。像素艺术 + GIF(8–15 fps)视觉上 OK
- 备用加速(MVP 不做):脏区检测 / 分块 DMA

### AnimatedGIF 集成

```cpp
// character.cpp 当前回调:spr.drawPixel(x, y, color)
// 改为:hwCanvas()->drawPixel(x, y, color)
```

GIF 文件本身宽 96px(协议规定),HW_W=184 居中即可。无需改资源。

### ASCII 宠物(buddies/)

18 个文件改造:
- `buddy.h` 引入 `using BuddySurface = Arduino_GFX;`
- 函数签名 `void renderXxx(TFT_eSprite& spr, ...)` → `void renderXxx(BuddySurface& s, ...)`
- API 几乎一致(`drawString` / `fillRect` / `setTextColor` / `setCursor` / `print` 都有)
- 每文件 ~5–10 行改动,主要 sed `spr.` → `s.`

## 5. 输入(按键 + 触摸)

### Key1(GPIO0,直连)

```cpp
#define KEY1_PIN 0   // GPIO0,按下 = LOW
static HwBtn k1;

static void scanKey1(uint32_t now) {
  bool pressed = digitalRead(KEY1_PIN) == LOW;
  k1.wasPressed  = pressed && !k1.isPressed;
  k1.wasReleased = !pressed && k1.isPressed;
  if (k1.wasPressed) k1.pressedAt = now;
  k1.isPressed = pressed;
}
bool HwBtn::pressedFor(uint32_t ms) {
  return isPressed && (millis() - pressedAt) >= ms;
}
```

物理 R8 (10K 上拉) + C14 (100nF) 已去抖,无需软件。

### Key3(AXP IRQ → TCA9554)

按 Key3 的事件链:
1. 机械按键拉低 AXP2101 PWRON
2. AXP2101 内部分类(短按/长按),触发 IRQ
3. AXP IRQ → TCA9554 P5
4. 每帧 I2C 读 expander.digitalRead(5),低 = 有 IRQ
5. 读 AXP IRQ status 寄存器得知短按/长按

```cpp
static HwBtn k3;
static uint8_t axpEvt;   // 0 / 0x02 短按 / 0x04 长按

static void scanKey3() {
  if (expander.digitalRead(5) == 0) {
    uint32_t s = pmu.getIrqStatus();
    if (s & XPOWERS_PWR_BTN_CLICK_INT)        axpEvt = 0x02;
    if (s & XPOWERS_PWR_BTN_LONGPRESSED_INT)  axpEvt = 0x04;
    pmu.clearIrqStatus();
  }
  bool pressed = (axpEvt == 0x02);
  k3.wasPressed = pressed;
  k3.wasReleased = pressed;
  k3.isPressed = false;   // 短按是脉冲,不持续
  if (pressed) axpEvt = 0;
}
```

### 重要语义差异 — 三键 → 两键映射

M5Stick 有 BtnA / BtnB / PWR 三键,新板只有 Key1 / Key3 两键。映射:

| M5 原行为 | 新板映射 |
|---|---|
| BtnA(前键)所有用法 | Key1 直连,完全保留(包括 `pressedFor(600)` 开菜单) |
| BtnB(右键)所有用法(deny/翻页/确认) | **Key3 短按**(AXP IRQ 0x02) |
| PWR 短按(切屏 on/off) | **Key3 长按**(AXP IRQ 0x04,~1s) |
| PWR 长按 6s(硬件关机) | Key3 超长按 6s(AXP 硬件关机,无软件介入,与 M5 一致) |

**关键差异说明**:
- M5 BtnB 没用 `pressedFor` — 安全(Key3 是脉冲事件,无法长按检测)
- M5 PWR 短按很快,新板 Key3 长按需 ~1s 才触发切屏 — 用户感觉略慢,但是 2 按键的物理限制
- AXP IRQ 事件路由约定:0x02 由 `scanKey3()` 消费给 BtnB,0x04 由 `hwAxpBtnEvent()` 返回给 main 用 — 二者互斥,AXP 同一事件只置一种 flag,不会冲突

### 触摸(FT3168)

```cpp
volatile bool tpIrqFlag = false;
void IRAM_ATTR onTouchIRQ() { tpIrqFlag = true; }

static HwTouch tp;

static void scanTouch() {
  if (!tpIrqFlag) {
    tp.justPressed = false;
    tp.justReleased = tp.down;
    tp.down = false;
    return;
  }
  tpIrqFlag = false;
  uint8_t fingers = ft3168->IIC_Read_Device_Value(FINGER_NUMBER);
  if (fingers > 0) {
    int rx = ft3168->IIC_Read_Device_Value(TOUCH_COORDINATE_X);
    int ry = ft3168->IIC_Read_Device_Value(TOUCH_COORDINATE_Y);
    tp.justPressed = !tp.down;
    tp.justReleased = false;
    tp.x = rx / 2;   // 368 → 184
    tp.y = ry / 2;   // 448 → 224
    tp.down = true;
  } else {
    tp.justReleased = tp.down;
    tp.down = false;
    tp.justPressed = false;
  }
}
```

### 触摸如何融入 main.cpp

不在 hw/ 层做"触摸→虚拟按键"映射(那样污染抽象)。在 main.cpp 加 hit-test helper:

```cpp
static bool tap(int x, int y, int w, int h) {
  const HwTouch& t = hwTouch();
  return t.justPressed && t.x >= x && t.y >= y && t.x < x+w && t.y < y+h;
}
```

按键和触摸**并联**:

```cpp
// 审批屏
if (inPrompt) {
  bool approve = hwBtnA().wasReleased() || tap(0, H-78, W, 39);
  bool deny    = hwBtnB().wasPressed()  || tap(0, H-39, W, 39);
}

// 菜单屏:按键导航 + 触摸直接点选
if (menuOpen) {
  if (hwBtnA().wasReleased()) menuSel = (menuSel + 1) % MENU_N;
  if (hwBtnB().wasPressed())  menuConfirm();
  int hit = tap(mx, my, mw, MENU_N * 14) ? (hwTouch().y - my) / 14 : -1;
  if (hit >= 0 && hit < MENU_N) { menuSel = hit; menuConfirm(); }
}
```

按键路径完全不变,触摸只是附加事件源。如果用户根本不碰屏,设备和 M5Stick 行为一致。

## 6. 驱动替换映射表

| 原 API | 新 API | 实现 | 备注 |
|---|---|---|---|
| `M5.Axp.GetBatVoltage()` | `pmu.getBattVoltage()/1000.0f` | hw/power.cpp | 库返回 mV |
| `M5.Axp.GetBatCurrent()` | `pmu.getBatteryChargeCurrent()` 或 `pmu.getBattDischargeCurrent()` | hw/power.cpp | 充/放分 API,合成 mA(充电=负) |
| `M5.Axp.GetVBusVoltage()` | `pmu.getVbusVoltage()/1000.0f` | hw/power.cpp | mV |
| `M5.Axp.GetTempInAXP192()` | `pmu.getTemperature()` | hw/power.cpp | AXP2101 也支持 |
| `M5.Axp.ScreenBreath(n)` | `gfx->setBrightness(lut[lvl])` | hw/display.cpp | **AMOLED 自带亮度,不走 PMU** |
| `M5.Axp.SetLDO2(false)` | `gfx->displayOff()` + brightness=0 | hw/display.cpp | AMOLED 自带 sleep |
| `M5.Axp.PowerOff()` | `pmu.shutdown()` | hw/power.cpp | AXP2101 关机 |
| `M5.Axp.GetBtnPress()` | `hwAxpBtnEvent()` | hw/input.cpp | 见第 5 节 |
| `M5.Imu.Init()` | `qmi.begin(...)` | hw/imu.cpp | SensorLib |
| `M5.Imu.getAccelData(&ax,&ay,&az)` | `qmi.getAccelerometer(ax,ay,az)` | hw/imu.cpp | g 单位,直接换 |
| `M5.Rtc.GetTime/GetDate` | `pcf.getDateTime()` | hw/rtc.cpp | PCF85063 |
| `M5.Rtc.SetDate/SetTime` | `pcf.setDateTime(...)` | hw/rtc.cpp | BLE time 同步 |
| `M5.Beep.tone(f,d)` | `hwBeep(f,d)` | hw/audio.cpp | I2S sine task |
| `M5.Beep.update()` | (无,FreeRTOS) | hw/audio.cpp | main 不调 |
| `M5.begin()` | `hwInit()` | hw/hw.cpp | 调用所有子初始化 |
| `M5.update()` | `hwInputUpdate()` | hw/input.cpp | 仅扫输入 |
| `digitalWrite(LED_PIN, ...)` | `hwBorderAlert(bool)` | hw/border.cpp | 屏幕边框替代 |

### AXP2101 配置

```cpp
pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
pmu.enableBattDetection();
pmu.enableVbusVoltageMeasure();
pmu.enableBattVoltageMeasure();
pmu.enableTemperatureMeasure();
pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
pmu.enableIRQ(XPOWERS_PWR_BTN_CLICK_INT | XPOWERS_PWR_BTN_LONGPRESSED_INT);
pmu.clearIrqStatus();
```

电池 SOC 沿用原码线性公式(3.2V→0%、4.2V→100%),不依赖 AXP2101 内部 SOC。

### IMU 轴方向

M5Stick 与 Waveshare 板 IMU 朝向不同。原码假设:
- 摇晃 = 加速度幅值变化(方向无关) — 不受影响
- 面朝下 = `az < -0.7`,可能要翻转 az 符号
- 横屏检测 = ax 主导 — 已删除横屏时钟,无需校准

**第一次烧录写串口调试,确定 az 朝向**。如果反了,在 `hwImuAccel` 内 `*az = -d.z;`。

### ES8311 + I2S 蜂鸣

```cpp
static QueueHandle_t beepQ;
struct BeepReq { uint16_t freq, dur; };

static void beepTask(void*) {
  // I2S:48 kHz,16-bit mono,DMA 4×512
  i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr);
  i2s_set_pin(I2S_NUM_0, &pins);
  es8311_codec_init();
  digitalWrite(PA_CTRL, HIGH);   // 喇叭功放开

  BeepReq r;
  while (xQueueReceive(beepQ, &r, portMAX_DELAY)) {
    int samples = 48000 * r.dur / 1000;
    int16_t buf[256];
    float phase = 0, dphase = 2*M_PI*r.freq / 48000;
    for (int n = 0; n < samples; n += 256) {
      int chunk = std::min(256, samples - n);
      for (int i = 0; i < chunk; i++) {
        buf[i] = (int16_t)(8000 * sinf(phase));
        phase += dphase;
      }
      size_t bw;
      i2s_write(I2S_NUM_0, buf, chunk*2, &bw, portMAX_DELAY);
    }
  }
}

void hwBeep(uint16_t f, uint16_t d) {
  if (!beepQ) return;
  BeepReq r{f, d};
  xQueueSend(beepQ, &r, 0);   // 满了就丢
}
```

PA 简化策略:开机就开,功耗增加几 mA 接受。如果功耗成问题,后续改为 idle 关 PA + 来 req 开 PA + 5ms 启动等待。

## 7. 构建系统与分区

### `platformio.ini`(waveshare 分支)

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

### 分区表 `no_ota_8mb.csv`

```
# Name,    Type, SubType,  Offset,   Size
nvs,       data, nvs,      0x9000,   0x6000
phy_init,  data, phy,      0xf000,   0x1000
factory,   app,  factory,  0x10000,  0x600000
spiffs,    data, spiffs,   0x610000, 0x1F0000
```

老板 4MB,新板 8MB:app 6MB 充裕,LittleFS ~2MB(角色包上限 1.8MB)。

### 烧录流程

1. PlatformIO 选 `waveshare-amoled`
2. `pio run -e waveshare-amoled -t upload`
3. 首次:`-t erase` 先擦,清出厂 NVS
4. LittleFS 初始为空,角色包通过 BLE 推送(协议不变)
5. 串口:`pio device monitor`(USB-CDC 与 USB 共线)

## 8. 测试策略

无硬件 CI,全手测。分 5 阶段冒烟,每阶段单独烧、单独验证,失败立刻停:

| 阶段 | 烧什么 | 验什么 | 通过标准 |
|---|---|---|---|
| 1. 显示 | hello world(只 hw/display + main 显示 "OK") | AMOLED 出图、亮度、颜色 | 30+ fps,无撕裂 |
| 2. 输入 | 按键串口打印 + 触摸坐标打印 | Key1/Key3/AXP IRQ/触摸坐标 | 按一下打一行;触摸坐标 0..183/0..223 |
| 3. 传感器 | IMU + RTC + 电池循环输出 | 加速度、温度、RTC、电压 | 摇晃数变化;充电时电流为负;RTC 持续走 |
| 4. BLE | bleInit + 收 heartbeat | 配对、收 prompt | 桌面端 Hardware Buddy 看到设备;passkey 显示正确 |
| 5. 全集成 | 完整 main.cpp | 所有原功能 | 桌面端发审批 → 屏显示 → Key1 通过 → 桌面端确认 |

每阶段写独立 `test_<n>.cpp` 临时 main,验证完丢弃。

## 9. 风险清单

| 风险 | 对策 |
|---|---|
| IMU 轴方向与 M5 不同 | 阶段 3 串口实测校准 |
| AMOLED 烧屏(常驻 HUD 标题) | 5 分钟后整屏 1px 抖动,或亮度自动衰减 |
| 触摸 IRQ 在快速滑动时偶尔丢 | TP_INT 直连 GPIO21,问题不大;严重时加每帧轮询 |
| GIF 解码 + Canvas push 同时跑卡顿 | 阶段 5 实测,必要时降 GIF 帧率或脏区 push |
| XPowersLib 具体 API 名/单位与版本相关 | 第 6 节映射表是按例程 + 文档写的,实际写代码时按本地安装的 XPowersLib 源码二次确认(`getBattVoltage` 单位、`shutdown` vs `setPowerOffMode`、IRQ flag 名等) |
| ES8311 寄存器配置 | 抄例程 `15_ES8311.ino` 完整初始化序列 |

## 10. 工期估算

每天 4–6 小时全职等价:

- 阶段 1–3(驱动调通):**1 周**
- 阶段 4(BLE 移植 + 配对联调):**0.5 周**
- 阶段 5(主循环重接 + 18 species + GIF + 触摸 + UI 重排):**1.5 周**
- 调优 / 烧屏防护 / 文档:**0.5 周**

**总计 ~3.5 周**。

## 11. 范围外(明确不做)

- SD 卡读写
- 麦克风录音
- WiFi 网络
- 横屏时钟
- 烧屏防护以外的电源管理优化
- 自动化测试(无硬件 CI 可行性)
- main 分支与 waveshare 分支的 HAL 共享(刻意分叉)
