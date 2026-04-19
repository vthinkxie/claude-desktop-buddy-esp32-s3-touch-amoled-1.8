# Safe-Area for Rounded AMOLED — Design

**Date**: 2026-04-19
**Status**: Approved — ready for implementation plan
**Scope**: `src/hw/display.{h,cpp}`, `src/main.cpp`, `src/character.cpp`, `src/buddy.cpp`, `src/buddies/*.cpp`

## Problem

The Waveshare 1.8" AMOLED (physical 368×448) has rounded bezel corners that
clip a few pixels at each corner. The firmware draws into a logical 184×224
canvas that is 2× nearest-neighbor upscaled to the panel. Today, text, icons,
and status elements that sit near the canvas edges get visibly eaten by the
bezel, and the bottom-left / bottom-right corners are the most noticeable
because bottom rows of text frequently anchor there (e.g. `setCursor(8, 184)`
with `"enter on desktop:"`).

Placement is currently handled by ad-hoc hard-coded offsets at each call site,
with no shared rule. Every new screen risks re-introducing the clip.

## Goal

Define a single source-of-truth "safe area" in the logical canvas coordinate
system and migrate all UI-level drawing call sites to use it. Buddy sprites
and full-canvas background fills are excluded — pixel-level clipping of their
body is not visually noticeable.

## Non-Goals

- Pixel-accurate rounded-corner masking of the canvas. The problem is visual
  layout of UI elements, not stray pixels outside the rounded region.
- Changing the logical canvas size or the 2× upscale pipeline.
- Changing the hardware-layer alert bar coordinates (`src/hw/display.cpp`).
  That code writes directly in physical 368×448 coordinates and already has
  its own bezel-avoidance inset; it is calibrated independently.

## Design

### Constants (source of truth)

Add to `src/hw/display.h`, alongside `HW_W` / `HW_H`:

```cpp
// Logical-canvas safe-draw region for rounded AMOLED bezel.
// Symmetric inset; value finalized after on-device calibration
// (see DEBUG_SAFE_BOX below).
constexpr int SAFE_INSET = 8;

constexpr int SAFE_L = SAFE_INSET;
constexpr int SAFE_T = SAFE_INSET;
constexpr int SAFE_R = HW_W - SAFE_INSET;        // 176
constexpr int SAFE_B = HW_H - SAFE_INSET;        // 216
constexpr int SAFE_W = HW_W - 2 * SAFE_INSET;    // 168
constexpr int SAFE_H = HW_H - 2 * SAFE_INSET;    // 208
```

Rationale for `constexpr int` rather than accessor functions: all usages live
in logical-canvas coordinates, values are compile-time constants, and this
matches the existing `HW_W` / `HW_H` style.

### Debug overlay

Add to `hwDisplayPush()` in `src/hw/display.cpp`, as the first step before the
2× upscale blit:

```cpp
#ifdef DEBUG_SAFE_BOX
  hwCanvas()->drawRect(SAFE_L, SAFE_T, SAFE_W, SAFE_H, GREEN);
#endif
```

Enable via `platformio.ini`:

```ini
build_flags = -DDEBUG_SAFE_BOX
```

Used once during calibration to pick the final `SAFE_INSET` value, then
disabled. The macro stays in source as a future calibration aid.

### Calibration procedure

1. Enable `-DDEBUG_SAFE_BOX` with `SAFE_INSET = 8`.
2. Flash and observe the green rectangle relative to the bezel on a
   representative screen (clock mode is convenient).
3. Adjust:
   - If any side of the green rect is visibly eaten by the bezel → increase
     `SAFE_INSET`.
   - If all four sides sit clearly inside the bezel with a visible gap →
     decrease `SAFE_INSET`.
4. Converge on a value (expected range: 6–10).
5. Commit the final value in its own commit, disable `-DDEBUG_SAFE_BOX`,
   re-flash, verify.

### Migration scope

| Category | Change |
|---|---|
| Text (`setCursor`, width-aware right/bottom alignment) | All y anchors <= 4 become `SAFE_T`; all y anchors near `HW_H` become `SAFE_B - lineH`. All x anchors <= 4 become `SAFE_L`; right-aligned become `SAFE_R - textW`. |
| Buddy centered render (`character.cpp`, `buddy.cpp`) | Horizontal center uses `HW_W/2` unchanged. Bottom anchor changes from `HW_H` to `SAFE_B`. Sprite dimensions and bounding box unchanged (the sprite body may still cross the safe line; only the anchor moves). |
| Panel cards (`fillRoundRect` with `mx, my, mw, mh`) | For each site in `main.cpp`: verify `mx >= SAFE_L`, `my >= SAFE_T`, `mx+mw <= SAFE_R`, `my+mh <= SAFE_B`. If not, adjust `mx/my/mw/mh` to fit. |
| Icons / status strips (battery glyph, clock chars, canvas-drawn alert panels) | Same rule as text. |
| Full-canvas background `fillRect(0, 0, HW_W, HW_H)` | **Unchanged.** Full clear is fine; bezel just hides corner pixels. |
| Buddy body pixel drawing | **Unchanged.** Only the anchor point moves. |
| Hardware alert bar in `src/hw/display.cpp` | **Unchanged.** Different coordinate system. |

### Why symmetric

User confirmed symmetric inset after considering asymmetric (larger bottom)
and mixed options. Tradeoff accepted: the top may have a slightly larger gap
than strictly needed, in exchange for a one-constant mental model.

## Implementation Phases

**Phase 1 — constants + calibration (1 commit pair):**
1. Add constants to `src/hw/display.h` and `DEBUG_SAFE_BOX` overlay to
   `src/hw/display.cpp`. No migration yet.
2. Calibrate, commit final `SAFE_INSET` value, disable flag.

**Phase 2 — migration (split by file for reviewable commits):**
- `src/main.cpp` — clock mode, HUD, menu, transcript, alert panels.
- `src/character.cpp` + `src/buddy.cpp` — buddy bottom anchor.
- `src/buddies/*.cpp` — only if any buddy hardcodes canvas-edge anchors
  (audit required; most are body sprites that don't touch edges directly).

## Verification

Manually flip through every major screen after Phase 2:

- Clock mode (time, date, battery, temperature readouts)
- HUD mode (buddy + overlays)
- Transcript view (long text, CJK)
- Menu / settings pages
- Alert panels (red border + text)
- Each of the 18 buddies in `src/buddies/`

Watch for: last/first character of bottom rows, battery/time glyphs in top
corners, panel-card corners, buddy feet / shadows at bottom center.

## Risk / Rollback

- Phase 1 is additive and feature-flagged — zero risk to the default build.
- Phase 2 is purely coordinate adjustments. If a screen looks off after
  migration, revert the per-file commit. No cross-file coupling.

## Open Items

- Final `SAFE_INSET` value — determined empirically in Phase 1.
- Whether any buddy sprite hardcodes `HW_H` as a bottom anchor — audit during
  Phase 2 pass.
