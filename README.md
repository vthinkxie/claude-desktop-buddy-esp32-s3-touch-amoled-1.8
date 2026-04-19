# claude-desktop-buddy — ESP32-S3 AMOLED port

Claude for macOS and Windows can connect Claude Cowork and Claude Code to
maker devices over BLE, so developers and makers can build hardware that
displays permission prompts, recent messages, and other interactions.

This is a port of [anthropics/claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy)
(originally targeting M5StickC Plus) to the
**Waveshare ESP32-S3-Touch-AMOLED-1.8** board. The BLE wire protocol is
unchanged — it pairs with the same Claude desktop apps and behaves like a
larger-screen sibling of the M5StickC version.

> **Building your own device?** You don't need any of the code here. See
> **[REFERENCE.md](REFERENCE.md)** for the wire protocol: Nordic UART
> Service UUIDs, JSON schemas, and the folder push transport.

## Hardware

[Waveshare ESP32-S3-Touch-AMOLED-1.8](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.8)
— ESP32-S3R8 (8 MB OPI PSRAM, 8 MB flash), 1.8" 368×448 AMOLED with QSPI
SH8601 driver, FT3168 capacitive touch, AXP2101 PMU, QMI8658 IMU,
PCF85063 RTC, ES8311 audio codec + speaker, two physical buttons.

The firmware targets ESP32-S3 with Arduino framework 3.x via the
[pioarduino](https://github.com/pioarduino/platform-espressif32) platform.
All board-specific drivers are wrapped in a thin HAL layer at `src/hw/`,
so adapting to a similar ESP32-S3 board mainly means changing
`src/hw/pins.h` and re-checking the display init.

Internal canvas is **184×224** (logical), 2× nearest-neighbor upscaled to
the 368×448 panel each frame. This preserves the chunky-pixel look of the
M5StickC original and keeps Canvas memory at ~80 KB (PSRAM). Rendering is
~25 fps after the upscale.

## Flashing

Install
[PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/),
then:

```bash
pio run -e waveshare-amoled -t upload
```

If you're starting from a previously-flashed device (e.g. the factory
Xiaozhi firmware), wipe it first:

```bash
pio run -e waveshare-amoled -t erase && pio run -e waveshare-amoled -t upload
```

LittleFS auto-formats on first boot if the partition isn't recognised.

Once running you can also wipe everything from the device itself:
**hold Key1 → settings → reset → factory reset → tap twice**.

## Pairing

To pair your device with Claude, first enable developer mode (**Help →
Troubleshooting → Enable Developer Mode**). Then open the Hardware Buddy
window in **Developer → Open Hardware Buddy…**, click **Connect**, and pick
your device from the list (advertised as `Claude-XXXX`). macOS will prompt
for Bluetooth permission on first connect; grant it.

The device shows a 6-digit passkey on screen — type it on the desktop to
complete LE Secure Connections bonding. Once paired, the bridge
auto-reconnects whenever both sides are awake.

## Controls

The board has two physical keys. **Key1** is the BOOT button (acts as
"A" in the table). **Key3** is the AXP power key — short-press is "B",
long-press toggles screen off, very-long-press hardware-shuts-down.

|                          | Normal               | Pet         | Info        | Approval    |
| ------------------------ | -------------------- | ----------- | ----------- | ----------- |
| **Key1** (BOOT)          | next screen          | next screen | next screen | **approve** |
| **Key3** (PWR, short)    | scroll transcript    | next page   | next page   | **deny**    |
| **Hold Key1**            | menu                 | menu        | menu        | menu        |
| **Key3** (PWR, ~1s long) | toggle screen off    |             |             |             |
| **Key3** (PWR, ~6s)      | hard power off       |             |             |             |
| **Shake**                | dizzy                |             |             | —           |
| **Face-down**            | nap (energy refills) |             |             |             |

Touch is supplemental — keys remain primary:

- **Approval screen** — tap upper half = approve, lower half = deny
- **Menu / Settings / Reset** — tap a row to select+confirm in one go
- **Info / Pet pages** — tap top-right corner to cycle pages
- **Normal HUD** — tap bottom 32 px to scroll the transcript

### Sleep & wake

- **USB plugged** — never auto-offs; the clock face stays visible
- **Battery + clock visible** — auto-off after **5 minutes**
- **Battery + other screens** — auto-off after **30 seconds**
- **Approval prompt up** — never auto-offs

Any key press or screen tap wakes the panel.

## Notable differences from the M5StickC original

- **Display layer** — Arduino_GFX + PSRAM Canvas (was M5.Lcd / TFT_eSprite)
- **Attention indicator** — small red pill at top of screen
  (M5 used a GPIO red LED; the AMOLED board has none)
- **Landscape clock removed** — 368×448 is near-square; rotation pointless
- **Battery current not exposed** — XPowersLib / AXP2101 only reports
  voltage, %, and isCharging. The info-page "current" reads 0 mA
- **Transcript supports CJK** — uses `chill7_h_cjk` font for the HUD lines
  so Chinese / Japanese log entries render legibly
- **Other UI strings stay ASCII** — non-ASCII bytes in `msg`, `promptTool`
  and `promptHint` are replaced with random Matrix-rain symbols rather
  than rendering as garbage glyphs

## Per-state animations

| State       | Trigger                     | Feel                                      |
| ----------- | --------------------------- | ----------------------------------------- |
| `sleep`     | bridge not connected        | eyes closed, slow breathing               |
| `idle`      | connected, nothing urgent   | blinking, looking around                  |
| `busy`      | sessions actively running   | sweating, working                         |
| `attention` | approval pending            | alert, **red top-bar pulses**             |
| `celebrate` | level up (every 50K tokens) | confetti, bouncing                        |
| `dizzy`     | you shook the device        | spiral eyes, wobbling                     |
| `heart`     | approved in under 5s        | floating hearts                           |

Eighteen ASCII species, each with all seven animations. **Settings →
ascii pet** cycles them; choice persists in NVS.

## Custom GIF characters

If you want a custom GIF character instead of an ASCII buddy, drag a
character pack folder onto the drop target in the Hardware Buddy window.
The app streams it over BLE and the device switches to GIF mode live.
**Settings → reset → delete char** reverts to ASCII mode.

A character pack is a folder with `manifest.json` and 96 px-wide GIFs:

```json
{
  "name": "bufo",
  "colors": {
    "body": "#6B8E23",
    "bg": "#000000",
    "text": "#FFFFFF",
    "textDim": "#808080",
    "ink": "#000000"
  },
  "states": {
    "sleep": "sleep.gif",
    "idle": ["idle_0.gif", "idle_1.gif", "idle_2.gif"],
    "busy": "busy.gif",
    "attention": "attention.gif",
    "celebrate": "celebrate.gif",
    "dizzy": "dizzy.gif",
    "heart": "heart.gif"
  }
}
```

State values can be a single filename or an array. Arrays rotate
loop-by-loop, useful for an idle activity carousel.

GIFs are 96 px wide; up to ~140 px tall keeps the character above the HUD.
The whole folder must fit under 1.8 MB; `gifsicle --lossy=80 -O3 --colors 64`
typically cuts 40–60 %.

See `characters/bufo/` for a working example. If you're iterating on a
character and would rather skip the BLE round-trip,
`tools/flash_character.py characters/bufo` stages it into `data/` and runs
`pio run -t uploadfs` directly over USB.

## Project layout

```
src/
  main.cpp           — loop, state machine, UI screens
  buddy.{cpp,h}      — ASCII species dispatch + render helpers
  buddies/           — one file per species, seven anim functions each
  character.{cpp,h}  — GIF decode + render
  ble_bridge.{cpp,h} — Nordic UART service, line-buffered TX/RX
  data.h             — wire protocol, JSON parse, CJK matrixifier
  xfer.h             — folder push receiver
  stats.h            — NVS-backed stats, settings, owner, species choice
  hw/                — board HAL (display, input, power, imu, rtc,
                       audio, expander, border, pins)
lib/
  ES8311/            — vendored Espressif codec driver
  Arduino_DriveBus/  — vendored FT3168 touch driver (Waveshare)
  Adafruit_XCA9554/  — vendored TCA9554 expander driver
characters/          — example GIF character packs
tools/               — generators and converters
docs/superpowers/    — design spec + implementation plan from the port
```

## Availability

The BLE API is only available when the Claude desktop apps are in
developer mode (**Help → Troubleshooting → Enable Developer Mode**).
It's intended for makers and developers and isn't an officially
supported product feature.
