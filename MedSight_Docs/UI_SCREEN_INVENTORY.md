# UI_SCREEN_INVENTORY.md — MedSight

Every screen the device can show, as of the end of Session 12. Written as the
input to Session 13's design overhaul: it is a *description of what exists*, not
a design brief. Where a screen has a known problem, it says so.

Screen state is driven by `AppState_t` in `FSBL/Inc/ui/state_machine.h`.
Drawing lives in `FSBL/Src/ui/gui_draw.c` (shared screens) and
`FSBL/Src/ui/registration_ui.c` (the three registration entry screens).
The mascot layer is `FSBL/Src/ui/anime_ui.c`.

Everything is 800×480, RGB565, drawn CPU-side into a single framebuffer at
`0x34200000`, then flushed out of the D-cache so the LTDC can see it. There is
no double buffering — that fact constrains almost every visual problem listed
below.

---

## 1. The screens

| # | State | Drawn by | What the user sees | Exits via |
|---|---|---|---|---|
| 1 | `STATE_HOME` | `gui_draw_home_screen()` | Teal title bar "SMART PILL DISPENSER"; two large buttons, **REGISTER PATIENT** (teal) and **DISPENSE PILLS** (amber); a bordered dialog box with Lumio's greeting; the mascot animating at the right. Session 12 adds an SD-card warning in the dialog box when the card is missing. | Tap either button |
| 2 | `STATE_INSTRUCT_REGISTER` | `gui_draw_ready_screen()` | Keeps the title bar, replaces the two buttons with one full-width **I AM READY**, and rewrites the dialog text to "Please face the camera… Press READY when set." | Tap READY (after a 500 ms dwell guard) / USER1 |
| 3 | `STATE_CAMERA_REGISTER` | *(live camera)* | 1.5 s of live camera preview so the user can frame themselves, then the camera freezes and the NPU runs. **Nothing is drawn during inference** — the screen holds the last camera frame, and the NPU visibly corrupts it because its scratch memory is the framebuffer. | Face found → 4; not found → 11; USER1 → 1 |
| 4 | `STATE_KEYBOARD_REGISTER` | `registration_ui_draw_keyboard()` | Full-screen uppercase QWERTY (3 letter rows + SPACE / DEL / DONE), 70×56 px keys, with a name field and a trailing `_` cursor along the top. | DONE with a non-empty name → 5; USER1 → 1 |
| 5 | `STATE_PILLCOUNT_REGISTER` | `registration_ui_draw_pillcount()` | A tappable two-tone pill-capsule icon with a "+1" badge (each tap adds one), a large centred digit, a RESET button top-right, and a NEXT button. Range 1–10. | NEXT → 6; USER1 → 1 |
| 6 | `STATE_CONFIRM_REGISTER` | `registration_ui_draw_confirm()` | "CONFIRM REGISTRATION", the entered name and pills-per-dose, and two centred buttons **CONFIRM** / **RETRY**. | CONFIRM → saves, 2 s "Registered!" dialog → 1; RETRY → 3 |
| 7 | `STATE_INSTRUCT_DISPENSE` | `gui_draw_ready_screen()` | Identical to screen 2 — same READY button, same wording. The two flows are visually indistinguishable at this step. | Tap READY → 8; USER1 → 1 |
| 8 | `STATE_CAMERA_DISPENSE` | *(live camera)* | Same as screen 3: preview, freeze, inference, same corruption. | Match → 9; no match / no face → 11; USER1 → 1 |
| 9 | `STATE_DISPENSING` | `gui_draw_dispensing_screen()` + `gui_draw_dispensing_progress()` | "DISPENSING YOUR PILLS", the patient's name, their dose ("3 pills"), and a bordered progress bar that fills left-to-right over ~2 s in 10 steps. | Automatic → 10 |
| 10 | `STATE_CONFIRM_TAKEN` | `gui_draw_confirm_taken_screen()`, then `gui_draw_taken_thankyou_screen()` | Amber title "TAKE YOUR PILL NOW", a large green **I TOOK IT!** button filling most of the screen, and a small grey **SKIP** in the top-right corner. On confirm, a plain "Thank You!" screen for 1.5 s. | I TOOK IT / SKIP / 30 s timeout → 1 |
| 11 | `STATE_FACE_RETRY` *(Session 12)* | `gui_draw_two_choice_screen()` | Amber title "FACE NOT RECOGNISED", advice text that differs by cause ("I could not see a face…" vs "Sorry, I do not recognise you…"), and two centred buttons **TRY AGAIN** / **CANCEL**. | TRY AGAIN → 3 or 8; CANCEL / 30 s → 1 |
| 12 | `STATE_ALERT` *(Session 12)* | `gui_draw_alert_screen()` | Coloured title bar (red or amber), a message, one wide **OK** button. Currently raised for: gallery full (on tapping REGISTER), and the AI pipeline failing to initialise. | OK / 30 s → wherever the alert was told to return |

Plus two overlays that are not states of their own:

- **Dialog text** (`gui_draw_dialog_text()`) — rewrites the bottom dialog box in
  place. Used for "Registered!", the SD-card warning, and transient messages.
- **The mascot** (`anime_ui_update()`) — DMA2D-blended into the top-right region
  on screens 1, 2 and 7 only.

---

## 2. Known problems, for Session 13 to fix

These are recorded from the code and from hardware observation, not guessed.

### 2.1 The mascot has exactly one state, and always has

`mascot_state_t` defines `MASCOT_IDLE`, `MASCOT_ACTIVE`, `MASCOT_SUCCESS` and
`MASCOT_ERROR`. `anime_ui_update()` switches on the current state and calls
`anime_ui_state_active()` / `_success()` / `_error()` — **all three of which
render the same idle breathing animation.** And `anime_ui_set_state()` is never
called from anywhere in the codebase (grep-verified). So the mascot has been
frozen in `MASCOT_IDLE` since Session 04.

This matters because two documents currently claim otherwise:
`MASCOT_UI_DESIGN.md` §4 and `SOFTWARE_ARCHITECTURE.md` §5 both say Session 11
drives `MASCOT_SUCCESS`/`MASCOT_ERROR` from real system events. It never
happened. Session 13 should either implement the states and wire them, or
correct both documents — but not leave the claim standing.

### 2.2 The screen visibly corrupts during every face capture

Known and documented since Session 08B, deferred ever since. Both NPU networks'
activation scratch is hardcoded by ST's code generator to `0x34200000`, which is
this project's framebuffer, so running inference scribbles over whatever is on
screen. It self-heals when the next screen is drawn, but the user watches ~1–4 s
of garbage during the most important moment in the flow.

`GUI_BUFFER_ADDRESS` is already defined in `main.h` and completely unused. A
second framebuffer plus an LTDC layer-address switch would fix this properly and
would additionally let the UI keep animating during inference — which Session 12
established is impossible with one buffer. This is the single highest-value
visual fix available.

### 2.3 Register and Dispense look identical at the instruction step

Screens 2 and 7 are the same function with the same text. A user who taps the
wrong button on the home screen has no way to notice before the camera opens.

### 2.4 Nothing tells the user the device is thinking

Between the preview ending and the result appearing there is no progress
indication at all — the screen simply holds a frozen, corrupting frame. Session
12 made the UI task responsive during this window, so it is now *able* to draw a
"Checking…" state; it just has nowhere safe to draw it (see 2.2).

### 2.5 Inconsistent visual language

Screens accumulated one session at a time and do not share a system:

- Three different title-bar treatments (home/dispensing use teal, confirm-taken
  uses amber, alerts use red or amber, and the registration screens have no
  title bar at all — just `gui_draw_text()` at y=16).
- Button geometry is defined in four places: `gui_draw.h`'s `REG_BTN_*` /
  `DISP_BTN_*` / `READY_BTN_*` / `TAKEN_BTN_*` / `SKIP_BTN_*` / `CHOICE_*` /
  `ALERT_BTN_*`, and `registration_ui.c`'s private `KEY_*`, `PC_*`,
  `CONFIRM_BTN_*`, `RESET_BTN_*`.
- Two different "two big buttons side by side" layouts that are not the same
  size or position (`CONFIRM_BTN_*` vs `CHOICE_*`).
- The "Thank You!" screen is bare text on white with no title bar, no mascot and
  no styling at all.

### 2.6 Text rendering is a single hand-rolled 8×8 bitmap font

`gui_draw_text()` scales an 8×8 glyph by integer replication, so scale-3 text is
chunky 24 px blocks. It is legible but crude, and there is no proportional
spacing, no anti-aliasing, and no way to centre text without the caller counting
characters. Several screens hardcode x-offsets that were eyeballed to look
centred for one specific string.

### 2.7 Elderly-friendliness is asserted, never validated

The palette and touch-target sizes were chosen with the stated goal of being
readable for elderly users with reduced vision, but nothing has been checked
against a contrast standard, and the key sizes on the QWERTY keyboard (70×56 px)
are much smaller than the 60 px+ minimum the rest of the UI follows.

### 2.8 Minor

- The boot sequence shows whatever was in the framebuffer before the home screen
  is first drawn.
- `gui_draw_mascot_bg()` is a no-op kept only for API compatibility.
- `STATE_ALERT`'s message strings are stored as `const char *` pointers to
  literals; nothing owns them, which is fine today but fragile if a caller ever
  passes a stack buffer.
