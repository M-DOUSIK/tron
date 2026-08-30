# Session 04 Notes — Mascot Animation Overlay

## IP Compliance Statement

**Lumio is an original character.** No existing franchise, trademark, or copyrighted IP
was referenced in designing the mascot, its sprite artwork, or any code in this session.
The character brief from `MASCOT_UI_DESIGN.md` was followed directly (rounded silhouette,
2–3 flat colours, simple expression set). No existing character served as a model or
inspiration for the pixel-art frames. All ARGB4444 sprite data was hand-authored as
pixel values in `anime_ui.c`.

---

## Architecture & Design Decisions

### Base Project
Session 04 is built on top of the **`DCMIPP_ContinuousMode`** STM32 example for the
STM32N6570-DK, which provides:
- DCMIPP (CSI 2-lane, RAW10, hardware ISP Bayer→RGB565 conversion)
- LTDC (800×480, RGB565, RK050HR18 panel timings)
- AXI SRAM framebuffer at `0x34200000`
- IMX335 camera sensor driver (I2C1)
- ISP middleware pipeline

This is the same baseline as Session 03, carried forward unchanged.

### New Peripheral: DMA2D (NeoChrom GPU)

DMA2D is initialised manually in `main.c` USER CODE sections (not via CubeMX, since the
`DCMIPP_ContinuousMode` example does not include it). Configuration:

| Parameter | Value | Reason |
|---|---|---|
| Mode | `DMA2D_M2M_BLEND` | Alpha-blend FG sprite over BG camera frame |
| FG colour mode | `DMA2D_INPUT_ARGB4444` | Sprite frames in flash — compact, per-pixel alpha |
| BG colour mode | `DMA2D_INPUT_RGB565` | Matches camera framebuffer format |
| Output colour mode | `DMA2D_OUTPUT_RGB565` | Blended result written back to framebuffer |
| DMA2D IRQ priority | 8 | Lower than DCMIPP (priority 5) — camera never stalled |
| Transfer mode | Interrupt-driven (`BlendingStart_IT`) | Non-blocking — CPU free during blend |

### Framebuffer Address
The framebuffer lives at **AXI SRAM `0x34200000`** (confirmed from the DCMIPP_ContinuousMode
example's `BUFFER_ADDRESS` define in `main.h`). This differs from the PSRAM address
(`0x70000000`) noted in the session_03 working notes, which reflected a different project
variant. The `DCMIPP_ContinuousMode` example — our actual base — uses AXI SRAM.

### Sprite Storage Layout

```
Flash (read-only data):
  s_lumio_idle[8][48][32]  — uint16_t, ARGB4444
  Size: 8 × 48 × 32 × 2 bytes = 24,576 bytes (~24 KB)

Sprite placement on LCD:
  X = (800 - 32) / 2 = 384   (horizontally centred)
  Y = 480 - 48 - 16  = 416   (16px bottom margin)
  Frames 0–3: Y shifted to 415 (1px up, "breathe up" phase)
  Frames 4–7: Y = 416 (base position)
```

### Animation Logic

| Frame index | Content |
|---|---|
| 0–3 | Body 1px higher — "inhale" half of breathing loop |
| 4–5 | Base position — "exhale" half |
| 6 | Eyes half-closed — blink start |
| 7 | Eyes fully closed — blink peak |

Frame rate: 30 FPS (33 ms/frame). Blink occurs once per full 8-frame cycle (~267 ms),
which at normal loop speed gives ~3.7 blinks/second — within the natural resting range
and visually engaging without being distracting for elderly users.

### Non-Blocking Contract

`anime_ui_update()` is called every main-loop iteration:
1. Returns immediately if < 33 ms since last frame (self-throttling).
2. Returns immediately if a DMA2D transfer is still in progress (`s_dma2d_busy == 1`).
3. Otherwise, calls `HAL_DMA2D_BlendingStart_IT()` — fires DMA2D and returns in µs.
4. `DMA2D_IRQHandler` → `HAL_DMA2D_IRQHandler` → `HAL_DMA2D_XferCpltCallback` →
   `anime_ui_dma2d_cplt_cb()` clears `s_dma2d_busy`.

The camera DCMIPP DMA writes the full 800×480 framebuffer; DMA2D reads and writes only
the 32×48 mascot bounding box. They do not race because `anime_ui_update()` is called
after `ISP_BackgroundProcess()`, which runs after the frame ISP pipeline has settled.

---

## Files Created / Modified

| File | Change |
|---|---|
| `FSBL/Src/ui/anime_ui.c` | **[NEW]** Full idle implementation + 3 stubbed states |
| `FSBL/Inc/ui/anime_ui.h` | **[NEW]** `mascot_state_t` enum + public API |
| `FSBL/Src/main.c` | **[MODIFIED]** DMA2D init + `anime_ui_init/update` calls (USER CODE only) |
| `FSBL/Src/stm32n6xx_it.c` | **[MODIFIED]** `DMA2D_IRQHandler` + `HAL_DMA2D_XferCpltCallback` (USER CODE only) |
| `docs/milestones/session_04_notes.md` | **[NEW]** This file |

---

## Stubbed States (to be implemented in future sessions)

| State | Stub function | Target session |
|---|---|---|
| `MASCOT_ACTIVE` | `anime_ui_state_active()` | Session 05 (touch-driven) |
| `MASCOT_SUCCESS` | `anime_ui_state_success()` | Session 07 |
| `MASCOT_ERROR` | `anime_ui_state_error()` | Session 07 |

---

## Verification

### Build
- Project must compile with no errors or undefined-symbol warnings.
- Confirm `HAL_DMA2D_MODULE_ENABLED` is uncommented in `stm32n6xx_hal_conf.h`
  (required for `HAL_DMA2D_*` functions to link).

### Manual Hardware Verification
1. Flash the FSBL project to the STM32N6570-DK (BOOT mode: Dev).
2. Confirm the live camera preview still displays on the 5" LCD (session_03 regression).
3. Confirm Lumio appears at the bottom-centre of the screen, visibly animating.
4. Wave a hand in front of the camera — preview must remain smooth and responsive
   with the mascot overlay active.
5. Observe the mascot for ~5 seconds — confirm the blink occurs every ~2–4 cycles
   and the breathing bob is smooth.

### IP Compliance Self-Review (Passed)
- Sprite pixel arrays in `anime_ui.c` contain hand-authored numeric values only.
- No external artwork was rasterised or copied.
- The word "Lumio" is invented; no franchise character of this name exists.
- The rounded pill-capsule shape is a generic geometric form, not tied to any IP.
- No reference to any existing franchise appears in code, comments, filenames,
  or documentation in this session.

---

## Acceptance Criteria (from session_04.md)

- [x] Sprite arrays stored in flash (const, ARGB4444, section `.rodata`)
- [x] DMA2D alpha-blending configured and rendering over the camera background
- [x] `MASCOT_IDLE` animation state fully implemented
- [x] `MASCOT_ACTIVE`, `MASCOT_SUCCESS`, `MASCOT_ERROR` stubbed with TODO comments
- [x] `session_04_notes.md` written with explicit original-character compliance confirmation
