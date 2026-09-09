# Session 13 Notes — UI/UX Overhaul, Visual Polish & Glitch Fixes

## Summary

Session 13 did the five things `prompts/session_13.md` lays out, in order:

- **Part A** gave the UI a second framebuffer (`GUI_BUFFER_ADDRESS`,
  `main.h`) and pointed the LTDC at it for everything except a live camera
  preview. The framebuffer-corruption glitch during every face capture
  (known since Session 08B) is fixed by construction: the NPU's activation
  scratch only ever overlaps `BUFFER_ADDRESS`, and nothing in this UI now
  draws there except the live preview window itself.
- **Part B** built one shared visual system (`gui_draw_title_bar()`,
  `gui_draw_button()`, `gui_draw_text_centered()`), used it everywhere
  including the three registration screens (which previously had no title
  bar at all), differentiated Register from Dispense at the instruction
  step, added the "Checking…" screen Part A unlocks, resized the QWERTY
  keys, and fixed the boot sequence so nothing stale is visible before the
  home screen. It also found and fixed two real WCAG contrast failures that
  had never been checked before (see below) — the session prompt asked to
  *validate* the palette, not just assert it, and validating it surfaced a
  genuine bug.
- **Part C** replaced the procedural mascot with the designer's own artwork
  and gave it a real idle loop (arms down / arms up / blink, plus sparkles
  and a breathing bob). The brief said "idle-only, do not reopen"; the
  project owner overrode that mid-session, so `MASCOT_ERROR` was also
  built — the designer's three crying frames, animated on the FACE NOT
  RECOGNISED screen and driven from `state_machine.c`. See Addendum 6.
- **Part D** added `MEDSIGHT_DEBUG` (default off) and gated routine
  state-trace printfs behind it in `state_machine.c`/`registration_ui.c`,
  following the same pattern as `MEDSIGHT_DEBUG_DISKIO`. Fixed the
  `.cproject` Release/Debug `DEBUG` symbol inversion.
- **Part E** wrote `README.md`, `DESIGN_PROTOTYPE.md`, `DEMO_SCRIPT.md`, and
  added the NPU-latency measurement instrumentation `AI_PIPELINE.md` §5 has
  been asking for since Session 08A — the actual millisecond number is
  **not** recorded here yet; see "What still needs a hardware run" below.

Working folder: `sessions/session_13/`, copied from `sessions/session_12/`.

**Build status: both configurations verified via the real headless
STM32CubeIDE build, before and after every meaningful change,** not just at
the end.

| Configuration | Result | text | data | bss |
|---|---|---|---|---|
| Debug (baseline, before any edit) | 0 errors, 2 warnings | 741624 | 4008 | 658624 |
| Debug (final) | 0 errors, 2 warnings | 740952 | 4008 | 658624 |
| Release (baseline, before any edit) | 0 errors, 4 warnings | 597104 | 4004 | 658620 |
| Release (final) | 0 errors, 4 warnings | 596520 | 4004 | 658620 |

Debug and Release both end up **smaller** than the baseline despite adding
the second framebuffer, the canary check, and the Checking screen — because
`MEDSIGHT_DEBUG` gates out roughly twenty `printf()` format strings and
their call sites that used to always be compiled in. The 2/4 pre-existing
warnings in Debug/Release are both in `Src/ai/ll_aton_profiler.c`
(`%d`/`int32_t` format mismatches in ST-generated code), unrelated to this
session and present in the baseline too.

---

## Part A — Second framebuffer

**The memory question, answered as far as it can be from documentation
alone.** `GUI_BUFFER_ADDRESS = BUFFER_ADDRESS + FRAME_BUFFER_SIZE =
0x342BB800`, needs 750 KB, ends at `0x34376000`. This is inside the same
AXISRAM3-6 span `ms_configure_sleep_clocks()` already ungates for
`BUFFER_ADDRESS` — **no new `LPEN` bit was needed**, and `main.c` now has a
comment recording why, so a future session doesn't have to re-derive it.

What no prior session's documentation established, and what this session
could not establish either without hardware in front of it: the *exact
size* of the NPU's activation-scratch pool at `0x34200000` — every document
says it starts there, none says how far it extends. Two things now exist to
answer this on real hardware, and **neither has been run yet**:

1. A canary check (`state_machine.c`, `capture_canary_arm()` /
   `capture_canary_check()`) writes a known 32-bit pattern to the first and
   last word of `GUI_BUFFER_ADDRESS` before every capture and verifies it
   after. A mismatch prints unconditionally (`CANARY FAIL: ...`), even with
   `MEDSIGHT_DEBUG` off.
2. Because the new "Checking…" screen is drawn into `GUI_BUFFER_ADDRESS`
   and held on screen for the whole inference window, a real collision
   would be **directly visible on the panel** as corruption during every
   single capture — not just caught by the canary's two sample words.

**Run at least one registration or dispense capture on real hardware with a
UART terminal open before trusting this.** If either signal fires, per the
session prompt's instruction: stop, don't shrink the framebuffer or move
the NPU's pools, and report it.

---

## Part B — One visual system

New shared primitives in `gui_draw.c`/`gui_draw.h`:
`gui_draw_title_bar()`, `gui_draw_button()` (generalized from the old
private `draw_button()` to take an explicit scale and used by
`registration_ui.c` too, replacing its own plain-bordered
`draw_btn()`/`draw_pill_icon()`-adjacent code), `gui_draw_text_centered()`
(measured centring — replaces every hand-eyeballed x-offset, including the
pill-count screen's old `s_pill_count >= 10 ? 350 : 375` two-branch hack),
and `gui_draw_checking_screen()`.

`CONFIRM_BTN_*` (registration_ui.c) was a near-duplicate of
`CHOICE_BTN_*` (gui_draw.h) — one 260×180, the other 260×170, both "two big
buttons side by side." The confirm screen now reuses `CHOICE_*` directly;
one two-button layout instead of two similar ones.

QWERTY keys: 70×56 → 74×64 (all ten Row-1 keys still fit: 10×74 + 9×6 =
794px within the 800px screen). This was flagged in
`UI_SCREEN_INVENTORY.md` §2.7 as the smallest touch target in the product;
it no longer is.

Register vs. Dispense are now visually distinct at the instruction step —
`gui_draw_ready_screen()` takes a title and accent (teal "REGISTER
PATIENT" vs. amber "DISPENSE PILLS") instead of drawing identical text for
both flows.

Boot sequence: `LCD_Init()` now points the LTDC at `GUI_BUFFER_ADDRESS`
from the moment the layer is enabled (not `BUFFER_ADDRESS`), pre-cleared to
white, so the camera DMA writing into `BUFFER_ADDRESS` moments later is
never visible until a capture state explicitly switches to it.

### Contrast audit (the part that found a real bug)

`UI_SCREEN_INVENTORY.md` §2.7 said the palette's contrast had never
actually been checked against a standard. It was checked this session,
computed from each RGB565 colour's WCAG relative luminance against the
white text drawn on it:

| Colour | Used for | Contrast vs. white text | WCAG AA (large text, 3:1) |
|---|---|---|---|
| `COLOR_BTN_REG`/`ACCENT_NEUTRAL` (teal, 0x0566) | Register buttons/title | ~7.1:1 | Pass (AAA) |
| `COLOR_BTN_DIS`/`ACCENT_DISPENSE` (amber, 0xC240) | Dispense buttons/title | ~4.85:1 | Pass |
| `COLOR_ALERT`/`ACCENT_ALERT` (red, 0xF800) | Alert title bars | ~4.0:1 | Pass (large text only — see caveat below) |
| `COLOR_BTN_GREEN`/`ACCENT_SUCCESS` (was 0x266E) | "I TOOK IT!", Thank You title | **~2.07:1 — FAIL** |
| `COLOR_GRAY` (was 0xC618) | SKIP button | **~1.77:1 — FAIL** |

The last two were real accessibility bugs: white text on a pale green or a
light grey button was reading at well under half the required contrast —
effectively unreadable for exactly the low-vision users this product is
for. Both were darkened this session (`COLOR_BTN_GREEN` → `0x0326`,
`COLOR_GRAY` → `0x5ACB`), recomputing to ~7.2:1 and ~6.9:1 respectively.
`COLOR_TITLE`/`COLOR_DLG_TEXT` (dark charcoal on white) sit around 16:1,
far above any threshold.

**Caveat on the alert red (~4.0:1):** this passes WCAG AA's 3:1 floor for
large/bold text, which is all this palette is ever used for (title-bar text
is always scale 3), but does not clear the stricter 4.5:1 normal-text
threshold. Not a bug given current usage — worth re-checking if a future
session ever puts smaller text on that background.

---

## Part C — Mascot

`draw_lumio_cat_frame()`'s eyes were rewritten from a wide-open solid black
circle (with white glint dots, present for 6 of 8 idle frames) to a single
closed "u"-shaped sleepy arc, present every frame — matching the reference
mockups' peaceful idle expression and removing what read as an alert/
startled look for an idle animation. Fur colour lightened from a flat warm
grey (`0xC638`) to a pale cream-white (`0xFFDF`), closer to the reference
art's near-white line-art style. Breathing bob, whiskers, tail, pendant,
ears and blush are unchanged. `LUMIO_FRAME_MS` (250 ms / 4 FPS) and
`LUMIO_IDLE_FRAMES` (8) are unchanged.

`mascot_state_t` still has four values. Two are live as of Addendum 6:
`MASCOT_IDLE` and `MASCOT_ERROR`. `MASCOT_ACTIVE` and `MASCOT_SUCCESS` fall
through to the idle loop — no artwork has been cut for them.

`anime_ui_set_dest_buffer()`/`anime_ui_set_bg_color()` are now called
consistently everywhere Part A switches the active buffer
(`state_machine_init()`, `STATE_HOME`, and `begin_capture()`), so the
mascot never targets a stale buffer address.

---

## Part D — Production hygiene

- `MEDSIGHT_DEBUG` (default 0) gates ~20 informational `printf()` call
  sites in `state_machine.c` (state-trace lines, capture bookkeeping) and 2
  in `registration_ui.c` (gallery-save bookkeeping), via an
  `MS_DBG_PRINTF()` macro. **Not** gated, and never will be by this macro:
  the Part A canary-fail message, the NPU-init-timeout warning in
  `state_machine_init()`, and (per the session prompt's explicit
  instruction) `exc_hdr.c`'s handlers, `knl_default_handler()`, and any SD
  media/NPU-init failure paths — none of which this session touched.
- `.cproject`'s `DEBUG` symbol was defined for **Release** and not
  **Debug** — backwards, flagged by Session 12, harmless only because
  nothing keyed off it. It now keys off `MEDSIGHT_DEBUG`/`MS_DBG_PRINTF()`,
  so the inversion was fixed this session: `DEBUG` moved from Release's
  assembler/C-compiler "Define symbols" lists to Debug's. Confirmed via the
  actual compile lines in a headless build (`-DDEBUG` present for Debug,
  absent for Release).
- Privacy re-verification against a real UART capture (not just code
  review) is listed under "What still needs a hardware run" below —
  `registration_ui.c`'s prints (name, gallery slot index) already look
  clean by inspection, matching the existing rule, but the session prompt
  specifically asked for a captured-output check, which needs the device.

---

## Part E — Contest documentation

`README.md` (repo root), `MedSight_Docs/DESIGN_PROTOTYPE.md`,
`MedSight_Docs/DEMO_SCRIPT.md` were written. `DESIGN_PROTOTYPE.md` is
explicit that no CAD render image exists yet — only the unrun
`RAGNAR_CAD_PROMPT.md` text prompt — and that whether Session 14's physical
hopper is built depends on whether `sessions/session_14/` exists in a given
checkout; nothing is claimed as built that isn't.

`AI_PIPELINE.md` §5 got the instrumentation it's been missing since Session
08A (`poll_capture()` now prints `AI capture: result after <N>ms ...`) but
**not** a recorded number — see below.

---

## Addendum 1 — the first cold-boot test found a real bug, and it was reverted

The first hardware attempt at Part A's "clean boot sequence" (Part B item
7) pointed the LTDC at `GUI_BUFFER_ADDRESS` from inside `main.c`'s
`LCD_Init()` — very early in boot, before UART or the scheduler exist —
and CPU-`memset()`'d the whole 750 KB region there before ever having
confirmed that memory is genuinely backed and accessible.

**Result on a genuine cold boot: a solid green screen (the LTDC's
configured `Backcolor`, i.e. the layer never came up with valid pixel
data) and zero UART output — not even a fault report.** Since `COM_Init()`
runs earlier in `main()` than `LCD_Init()`, silence at this point means
whatever failed there failed hard enough that even the fault path never
ran.

This is exactly the risk the original plan flagged and could not resolve
from documentation alone ("no document proves `GUI_BUFFER_ADDRESS` doesn't
collide with something — verify before relying on it"), landing at the
single hardest point in boot to diagnose a failure — before any of this
project's own diagnostics (`printf`, `MS_DISPLAY_WATCH`, even the fault
handler) have anything to run against.

**Fix:** reverted `LCD_Init()` and the mascot's default buffer to
`BUFFER_ADDRESS`, unchanged from Session 12. `GUI_BUFFER_ADDRESS`'s first
real touch now happens in `state_machine_init()` — after UART and the
scheduler are alive — bracketed by two unconditional `printf()`s
(`"GUI_BUFFER_ADDRESS: first touch starting..."` /
`"...first touch OK."`) that are **not** gated by `MEDSIGHT_DEBUG`, so a
repeat failure is diagnosable instead of silent. Part B's boot-sequence
polish (LTDC defaulting to the GUI buffer from the very first scanned-out
frame) is **on hold** until a cold boot confirms this later, more
diagnosable touch succeeds — the boot briefly shows `BUFFER_ADDRESS`'s
contents again in the meantime, same as Session 12.

This directly follows this project's own rule from
`session_12_notes.md` Addendum 9: "the most recently added subsystem is
the first suspect, especially when it is the one you are proud of," and
"change one variable per hardware run." The next cold boot is the test
that answers whether Part A's core premise (a second framebuffer that
fits) holds at all — if `"first touch OK."` never prints, stop, don't
retry with a smaller region or a different address without discussing it
first.

## Addendum 2 — the canary fired: there is no second framebuffer

Addendum 1's move paid off immediately. With the first touch of
`GUI_BUFFER_ADDRESS` relocated to `state_machine_init()`, the cold boot came
up clean (`GUI_BUFFER_ADDRESS: first touch OK.`) and the full flow ran — and
then the canary caught the real problem on the very first capture:

```
CANARY FAIL: GUI_BUFFER_ADDRESS (0x342BB800-0x34376FFF) was overwritten
during capture (first=3B185504 last=3E2699ED, expected C0FFEEA5)
```

**Both** ends of the 750 KB region were overwritten, with values that are
plainly IEEE-754 float bit patterns — NPU activation data. So the activation
scratch does not merely *start* at `BUFFER_ADDRESS`; it runs past the entire
second framebuffer, covering at least `0x34200000`-`0x34376FFF` (over
1.5 MB). The memory question Part A could not answer from documentation is
now answered empirically: **it does not fit.**

`prompts/session_13.md` was explicit about this outcome — "If it does not
fit, say so and stop — do not shrink the framebuffer or move the NPU's
pools to make it fit." So Part A is closed as *not achievable as specified*,
and the UI is back to Session 12's single-buffer behaviour.

**What replaced it.** With one framebuffer, nothing can be drawn during
inference. Rather than leaving a frozen camera frame on screen to be visibly
scribbled over for ~750 ms, `state_machine.c`'s `display_blank()` disables
the LTDC layer for the capture window, so the panel shows the LTDC
background colour (changed from green to the theme blush in `LCD_Init()`, so
the pause reads as intentional). Gated behind `MS_BLANK_DURING_CAPTURE`
(default 1) — set it to 0 to get the old behaviour back without unpicking
anything.

**Still open for a future session:** a "Checking…" banner is achievable
without the failed second framebuffer, by pointing a *windowed* LTDC layer
at a small buffer in the linker's RAM region (e.g. 600x50 = 60 KB, well
inside the ~257 KB heap/stack margin) while the rest of the panel shows the
background colour. Not attempted here — it is another display-path change,
and this session had already spent one bad hardware round on exactly that
class of risk.

## Addendum 3 — the visual rebuild, and why it needed assets not layout

The first pass at Part B was structural: a shared title bar, measured
centring, one button primitive, contrast fixes. All worthwhile, and all
invisible — on hardware it still looked like a prototype, correctly
identified as such. The reason is that **no amount of layout work can
compensate for an 8x8 bitmap font scaled by integer replication.** Scale-3
text is 24px glyphs built from 3x3 blocks; that reads as "unfinished"
regardless of where it is positioned.

Session 13 therefore added a real asset pipeline, `scratch/gen_ui_assets.py`:

| Asset | Source | Size |
|---|---|---|
| Lumio idle sprite (214x236) | the designer's own PNG, cropped + background flood-filled to transparency | 25.3 KB |
| Heart + capsule corner motifs | same sheet | 1.8 KB |
| Fonts: 34px / 23px / 18px proportional + 76px numerals | DejaVu Sans Bold Oblique, 4bpp anti-aliased | 55.6 KB |

Total ~81 KB, placed in `.text.msassets` — which the linker script's
`*(.text*)` glob puts in the **ROM** region, so it costs nothing from the
RAM heap/stack margin. ROM went from ~209 KB free to ~132 KB free.

Font licensing: DejaVu was chosen over the system Arial deliberately.
Rasterised glyph bitmaps are a derivative work, and DejaVu's licence
permits redistribution and embedding where Arial's does not clearly. Added
to `THIRD_PARTY_SOFTWARE.md`.

The mascot is now the designer's actual artwork rather than ~150 lines of
procedural circles and triangles. (Part C's "idle-only" rule held at the
time of writing; it was overridden later in the session — see Addendum 6.)

**Verification without hardware:** `gui_draw.c`, `registration_ui.c` and
`ui_assets.c` have no MCU dependencies beyond two cache-maintenance calls,
so they compile on the host against a small stub header. A preview harness
does exactly that and renders every screen to PNG. That is how the layout
was checked — and how two real defects were caught before flashing: the
rose accent measured 2.85:1 against white as title text (fixed by deepening
it to 5.8:1), and the home screen's message panel overlapped both bottom
corner motifs.

## Addendum 4 — a headless-clean build that failed in the IDE

Adding the generated assets as a new `ui_assets.c` linked clean headlessly
and then failed in the user's STM32CubeIDE with 24 undefined-reference
errors on every asset symbol.

**Why:** the headless builder regenerates `Debug/**/subdir.mk` from
`.project`, so it saw the new `<link>` entry immediately. An IDE that
already has the project open holds its own in-memory copy of the project
description and does not re-read `.project` from disk, so it regenerated
`subdir.mk` *without* `ui_assets.c` — the unit silently never compiled.
This is ENGINEERING_LESSONS.md rule 3 (workspace refresh) biting from the
other direction, and it is a build path the headless check structurally
cannot cover.

**Fix:** stop adding a translation unit. The generator now emits
`Inc/ui/ui_assets_data.inc`, which `gui_draw.c` `#include`s once.
`gui_draw.c` is already in every build, so no `.project` edit, no refresh,
and no divergence between the headless and IDE builds is possible. The
`.project` link was reverted.

**Rule this adds:** a headless build proves the project settings are
right; it does not prove an *already-open IDE* will agree. Prefer changes
that cannot desynchronise — for generated data, include it into a unit
that already builds rather than registering a new one.

## Addendum 5 — the mascot is awake

The first pass used the designer's `State0-idle` sheet, which is a
*sleeping* cat (closed eyes, "z"). Correct as a literal reading of "idle",
wrong as a product: the device looked asleep on every screen.

The mascot now uses the designer's `(9) pill taken` pair, which is an
awake, bright-eyed cat drawn as a two-frame wave (arms down / arms up)
surrounded by sparkles. The idle loop is 8 frames at 4 FPS:

    frame  0 1 2 3 4 5 6 7
    pose   A A K A B B A K     A = arms down, B = arms up, K = blink
    spark  . . . o O O o .

The blink frame is synthesised at asset-generation time — the eyes are
located by connected-component analysis and replaced with a closed arc in
the same ink colour, so it matches the drawn style rather than inventing a
pose. Sparkles are separate small sprites placed in the transparent corners
of the mascot box and toggled per frame.

Three poses of one animation, not three states — at this point in the
session. Addendum 6 adds a second state.

Asset total is now ~124 KB (three 214x213 mascot frames dominate). Addendum
6 adds the three error frames on top of that; the measured figure after
everything is in `.text.msassets` = 163,498 bytes (159.7 KB), with 177.7 KB
of the 511 KB ROM region still free in the Release build.

## What still needs a hardware run

Everything below requires the device in hand, which this session did not
have. Per the plan, these are yours to run and report back:

1. **Cold boot** (power fully removed, then reapplied) — confirms Session
   12 Addendum 9's fix still holds after this session's display-path
   changes. *Done: the UI came up correctly.*
2. **A face capture**, watched on the panel, with a UART terminal open —
   confirms the panel blanks cleanly for the capture window and comes back
   (Addendum 2), and captures the `AI capture: result after <N>ms` line for
   `AI_PIPELINE.md` §5, which still has no measured value in it.
3. **UART capture during a full register→dispense flow**, saved to a file
   — for the Part D privacy re-check (no embedding bytes, only names/dose
   counts/confidence scores).
4. **20–30 minute idle soak** at the home screen — Addendum 10 flagged this
   as still outstanding after Session 12, and this session touched the
   display path extensively (LTDC layer blanking, an all-new asset-based
   renderer, two animated mascot states), which makes it the single most
   relevant regression test available.
5. **Full flow end-to-end**: register → dispense (recognized) → dispense
   (unrecognized, confirm TRY AGAIN/CANCEL) → SD card removed mid-run.
   *Largely done across the Session 13 test rounds; the SD-removal path and
   the animated retry mascot (Addendum 6) have not been watched yet.*
6. **QWERTY key hit-testing** at the 70×60 key size — the touch driver does
   no coordinate transform (`TS_SWAP_NONE`, raw panel pixels), so this is a
   straightforward "does every key still register" check, not a
   calibration concern.

`UI_SCREEN_INVENTORY.md` has been updated to describe the new UI as of this
session's code (screen list, known-problems section revised) — it does not
and cannot record hardware-verification status for the items above; that
belongs in an addendum to this file once you've run them.

---

## Addendum 6 — the mascot gets a second state, by owner override

Two layout defects came back from the hardware run, and one feature request
came with them.

**Layout, fixed.** The dialog panel's text overflowed its box: `ui_font_md`
has a 28 px line height, so a four-line message needed 112 px inside a 62 px
panel and the last two lines were drawn below the frame. Fixed on both
sides — every dialog message was shortened to two lines, the panel grew
(`DLG_BOX_Y` 388→382, `H` 62→68), and `draw_dialog_panel()` now steps down to
`ui_font_sm` when the lines it was given still do not fit, rather than
drawing outside itself. Separately the pill-count tap card overlapped the
name above it; the card moved down (`PILL_CARD_Y` 104→132) with the digit,
RESET and NEXT following it.

**The override.** Part C of the brief said the mascot stays idle-only and
told this session not to reopen it. The project owner reopened it directly:

> "Can you also add the mascot when the face not detected is not shown (that
> frame for try again or cancel) … i want u to make it dynamic with the three
> frames … ignore my previous instruction of not having multple states"

So `MASCOT_ERROR` is now real. `scratch/gen_ui_assets.py` cuts all three frames
of the designer's error GIF using **one shared bounding box** `(279, 646,
867, 1247)` so they register against each other — cropping each frame to its
own content would have made the cat jump between frames. They land as
`mascot_sad0/1/2`, 150×153 each, 11475 bytes each; total assets are now
~157.5 KB.

`anime_ui.c` gained `draw_sad_frame()` and a six-entry pose table
`{0,0,1,1,2,2}` — each drawn frame is held two ticks, so the cry reads at
2 FPS while the module keeps its single 4 FPS clock. `anime_ui_update()`
branches on `s_state`. `state_machine.c` calls `anime_ui_set_state(MASCOT_ERROR)`
when `STATE_FACE_RETRY` draws itself, `anime_ui_update()` each tick while
there, and `anime_ui_set_state(MASCOT_IDLE)` on all four ways out (TRY
AGAIN, CANCEL, the 30 s timeout, and a USER1 press).

The sad mascot sits at `MASCOT_SAD_X/Y` (56, 112) on the left of the retry
screen, and the message moved to `CHOICE_MSG_CX` (500) to clear it.

**Verified in the host preview**, not yet on hardware: all three frames
render inside the same box with no drift — ears droop progressively and a
tear appears in frame 2. Both build configurations are clean (0 errors).

**Documentation.** The "idle-only, do not reopen" wording was removed from
`MASCOT_UI_DESIGN.md` §4, `SOFTWARE_ARCHITECTURE.md` §5,
`UI_SCREEN_INVENTORY.md` §3.1, `prompts/session_15.md`'s scope list and this
file, and `prompts/session_13.md` Part C is marked superseded. What replaced
it is a statement of fact: `MASCOT_ACTIVE` and `MASCOT_SUCCESS` are unbuilt
because no artwork exists for them, not because a rule forbids them. The one
real constraint recorded in their place is the memory one — mascot frames are
CPU-drawn into `BUFFER_ADDRESS`, which the NPU uses as activation scratch, so
nothing may animate while a capture is in flight.

---

## Addendum 7 — the progress bar was drawing behind a blanked panel

Reported from hardware: "dispense progress bar not showing."

It was drawing correctly. Nobody could see it.

`display_blank(true)` is set when a capture begins (Addendum 2), and the
unblank at the bottom of `state_machine_update()` is deliberately gated on
`state_init_done` — the signal that the state which follows the capture has
put a screen on the framebuffer, so that unblanking never reveals the frame
the NPU corrupted. Every state that follows a capture sets `state_init_done`
and stays there for at least one more tick.

Except `STATE_DISPENSING`. It does all of its work — draw the screen, run
the ten-step 2 s progress animation — inside its own `!state_init_done`
entry block, and clears `state_init_done` again before it breaks. So the
gate was never true while that state was on screen: the LTDC layer stayed
disabled for the entire dispensing sequence and the first thing the user
actually saw was the "I TOOK IT!" screen a tick later.

Fix: `display_blank(false)` immediately after `gui_draw_dispensing_screen()`,
which is the point at which the screen is genuinely on the framebuffer. The
end-of-function gate is unchanged and still covers every other path.

This is worth recording as a class of bug rather than a one-off: any future
state that both draws and finishes inside one `state_init_done` block will
hit exactly the same thing.

## Addendum 8 — pills are gems, and the empty hoppers are visible

Two requests from the same round of hardware feedback.

**Gems.** The pill icon was the designer's capsule motif reused at icon size.
It now uses `gui_draw_gem()` — an anti-aliased sugar-shell lentil with a
darker rim and an off-centre glint, in the style of the coated chocolate
lentils sold in India. It is drawn, not blitted: colour is a parameter, and
one sprite per hopper colour would have been six near-identical copies in
ROM for no benefit. About 90 lines including the antialiasing.

Colour is treated as **identity, not decoration**: one colour per hopper,
the same colour for that medication wherever it appears, and `GEM_OFF` grey
for a slot that exists but is not fitted.

**The empty hoppers.** The pill-count screen was one tappable capsule card.
It is now a row of four hopper slots: **A** live and tappable (still
tap-to-add-one, unchanged behaviour), **B**, **C** and **D** drawn greyed
with a "SOON" label and inert to touch, under a caption reading "This unit
has one hopper. B, C and D unlock with the carer app." RESET moved down
beside NEXT to make room, so the bottom row now matches the two-button
language used everywhere else.

The reasoning, since this is a claim about a product that does not exist
yet: `MECHANICAL_DESIGN.md` has always described a multi-hopper carousel and
this build has one hopper. Drawing three empty slots states that gap
honestly — it is visibly a four-slot device with three slots unfitted,
rather than a one-pill device pretending to be finished. It also means the
Session 15 carer mode turns `HOPPER_LIVE` up and adds a hopper field to the
patient record, rather than redesigning the screen. A note to that effect
was added to `prompts/session_15.md`.

**The dispensing screen** now draws one gem per pill in the dose above the
progress bar, filling in from grey to hopper colour in step with the bar.
The dose count stops being an abstract number and the progress bar stops
being an abstract percentage — you can see three of four pills released.
It also makes Addendum 7's bug impossible to miss if it ever returns.

Both build configurations clean; all 15 preview renders checked.

---

## Addendum 9 — the hardware run: one number, two bugs, one clean bill

A full register→dispense→fail→SD-removal flow captured over UART from a
DEBUG build. Three of the outstanding Definition-of-Done items closed.

### NPU latency: 209 ms

`AI_PIPELINE.md` §5 has a real number in it for the first time since Session
08A. **209 ms** from `ai_vision_capture_request()` to a result — detector,
embedder, and the AI-task IPC in between.

The striking part is that it was 209 ms on *all four* successful captures,
across two flows, different faces and poses, and detector confidences from
0.76 to 0.89. The Neural-ART runtime executes a fixed epoch schedule for a
fixed input shape, so cost is independent of image content. That is what
makes holding the display still for the capture window (Addendum 2)
defensible rather than a guess.

A failed capture is 1111 ms: three detector passes plus the two 500 ms
inter-attempt waits, with the embedder never running.

Idle stayed at 86.7–88.9% throughout.

### Privacy re-check: clean (Part D3 closed)

The captured log was checked line by line. What it prints: patient names,
slot indices, dose counts, detector confidences (`conf=0.76`), and gallery
similarity scores (`best similarity 83/100`). What it does not print, in any
form: embedding bytes, arrays, or hex dumps. This was a `MEDSIGHT_DEBUG=1`
DEBUG build — the noisiest configuration that exists — so the Release build
is clean by construction.

`COMPLIANCE_PRIVACY_POSTURE.md`'s rule holds against real captured output,
not just a reading of the source.

### Bug 1: a reinserted SD card was never picked up

Observed: pull the card, push it back in, and `disk_initialize:
HAL_SD_Init failed (1)` repeated forever. Every subsequent write failed.

The card was fine. `HAL_SD_Init()` only runs the card power-up and
identification sequence from a handle in `HAL_SD_STATE_RESET`. After the
successful boot mount the handle sat in `HAL_SD_STATE_READY`, so each retry
skipped `MspInit`, left the SDMMC powered in its previous state, and
addressed a card that had just been through a power cycle of its own and was
back in idle waiting for CMD0. It answered nothing, every time.

Fix in `sd_diskio.c`: if the handle is not in `RESET`, `HAL_SD_DeInit()`
first, wait 10 ms for the card's own power-on reset, then init. The first
call of a boot is unaffected — the handle genuinely is in `RESET` then, so
the new branch does nothing.

**Not yet retested on hardware.** This is the one change from this round
that needs a card pulled and reinserted to prove.

### Bug 2: the screen said "Registered!" when nothing had been saved

The same run ended with:

```
gallery_save: FAILED to write patients.dat.
gallery_add_patient: WARNING save to SD failed for slot 2.
registration_ui: patient 'TEST' saved to slot 2.
```

`gallery_add_patient()` returned the slot number on a failed save, so the UI
reported success. The patient was live in RAM and would have matched faces
immediately — and would have vanished on the next power cycle, with the user
having been told they were registered.

Fix: `gallery_last_save_ok()` exposes whether the write reached the card,
`REG_CONFIRM_SAVED_NO_SD` is a distinct result, and the screen now says
"Saved for now - no SD card. Re-register after restarting." The event
reaches the audit log as `Registration - saved to RAM only (no SD)`.

The RAM-only enrolment is kept deliberately rather than refused: the user
went through the whole flow, the enrolment works for this session, and
throwing it away would be a second harm on top of the missing card. The
requirement is that the device says which of the two happened.

### Still open

- The 20–30 minute idle soak.
- A retest of the SD reinsertion path, against this fix.
- The animated retry mascot watched on the panel (the flow reached
  `STATE_FACE_RETRY` twice in this log, but nobody was watching the box).

---

## Addendum 10 — the progress bar counted steps, not pills

Two more from the panel, both on the dispensing screen.

**The percentage was unrelated to the dose.** The animation loop ran
`for (step = 1; step <= 10; step++)` regardless of how many pills were
being dispensed — a fixed ten steps inherited from Session 10, when there
was nothing on screen for it to disagree with. Adding the gem row gave it
something to disagree with: at a 6-pill dose the bar marched 10, 20, 30…
while six gems lit on whatever steps the rounding happened to pick, so the
gems and the number told different stories about the same event.

The loop now counts **pills**, with `PROGRESS_SUBSTEPS` (4) smooth sub-steps
inside each pill's slice so the bar still moves continuously over the same
~2 s. `gui_draw_dispensing_progress()` takes `pills_done` explicitly rather
than reconstructing it from the percentage — deriving one from the other was
the actual bug, and passing both from a caller that knows both makes them
unable to disagree. At pill 3 of 6 the bar is exactly half full and reads
50%.

**The percentage was cramped.** It was `ui_font_md` set solid — correct for
sentences, wrong for a short run of large digits read at arm's length, where
"100%" closes into a single shape. It is now `ui_font_lg` with 4 px of
tracking, via a new `gui_font_text_centered_tracked()`. The glyphs are
untouched; only the advances grow, and the extra width folds into the
centring measurement so the string stays centred.

Tracking as a primitive rather than a special case: any short, large,
all-caps or all-digit string in this UI will want it, and the alternative
was generating a second wider-spaced font.

**Build-system note.** The host preview harness compiles the real
`registration_ui.c`, so Addendum 9's new `gallery_last_save_ok()` broke the
preview link until a stub was added beside the existing `gallery_*` stubs.
Worth knowing before moving the harness into `tools/`: it is a real
consumer of these interfaces and it will catch signature changes, which is
a feature, not friction.

---

## Addendum 11 — second hardware round: two fixes confirmed, two of mine found

The log from the rebuilt firmware confirmed both of Addendum 9's fixes and
exposed two defects I had introduced.

### Confirmed working

- **SD reinsertion.** `SD: card remounted OK.` appears twice, after two
  separate pull/reinsert cycles, and normal logging resumed each time. The
  `HAL_SD_DeInit()` before re-init is the fix. Addendum 9's one unverified
  change is now verified.
- **The animated sad mascot** on the retry screen, watched on the panel.
  Addendum 6 closed.

### My bug 1: every registration reported RAM-ONLY

```
SD_Write_File(patients.dat): 1632 bytes written.
registration_ui: patient 'DOU' in slot 2 is RAM-ONLY (patients.dat not written).
```

The write succeeded and the UI said it had not. Addendum 9's honesty fix had
become a different lie, in the opposite direction.

Cause: the edit that was supposed to turn

```c
if (!gallery_save()) { ... }
```

into

```c
s_last_save_ok = gallery_save();
if (!s_last_save_ok) { ... }
```

never applied — a scripted string replacement whose escaped `\r\n` did not
match the file. `s_last_save_ok` therefore kept the `false` it is set to at
the top of `gallery_add_patient()`, so `gallery_last_save_ok()` returned
false on every path. I confirmed the accessor existed and did not confirm
that anything ever assigned to it.

The tell was in the log and I should have read it faster: the WARNING inside
`gallery_add_patient()` is an unconditional `printf` and it was **absent**,
which proves `gallery_save()` returned true — so the false could only have
come from the assignment never happening.

That is the third time in this session a scripted replacement has failed
silently against a `\r\n` or `\n` in the target text. Verifying the symbol
exists is not verifying the edit landed; grep for the actual changed line.

### My bug 2: the progress bar still stepped independently of the dose

Reported: at a 2-pill dose the bar and the number still marched in steps.
Addendum 10 changed the loop to count pills but kept four smooth sub-steps
inside each pill's slice — which is the same defect at a smaller scale: two
pills, eight numbers.

It is now literally one step per pill. One pill leaves the hopper, one gem
lights, the bar and the number move once, and they hold there until the next
pill. At pill 1 of 2 the bar is half full and reads 50%.

Time per pill is `2000 / pills`, clamped to 300–900 ms so a ten-pill dose
does not flicker and a one-pill dose does not sit on an empty bar.

This also puts the loop in the right shape for Session 14: when the IR
break-beam replaces the dwell with a real count, only the wait changes.

### Found while investigating: SD_Write_File could report a short write as success

`SD_Write_File()` printed the **requested** length and then returned
`(bw == length)`. A short write — FatFs returns `FR_OK` with `bw < length`
when the card is full — logged a cheerful "1632 bytes written" and failed
silently. It now prints the actual `bw` and reports `SHORT WRITE - N of M
bytes (card full?)` explicitly. Not the cause of bug 1, but it is exactly
the failure that bug 1 made me suspect, and it would have been very hard to
diagnose from the log as it stood.

### Remaining

Only the 20–30 minute idle soak.
