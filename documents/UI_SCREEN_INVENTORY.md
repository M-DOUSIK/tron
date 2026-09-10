# UI_SCREEN_INVENTORY.md — MedSight

Every screen the device can show, as of the end of **Session 15**. Written as a
description of what exists, not a design brief. Session 13 rebuilt this UI
as one visual system — see `milestones/session_13_notes.md` for exactly
what changed and why; this file just describes the result.

Screen state is driven by `AppState_t` in `FSBL/Inc/ui/state_machine.h`.
Drawing lives in `FSBL/Src/ui/gui_draw.c` (shared screens and the design
system — title bar, buttons, measured text centring) and
`FSBL/Src/ui/registration_ui.c` (the three registration entry screens,
which now use `gui_draw.c`'s shared primitives instead of their own), and
`FSBL/Src/ui/carer_ui.c` (Session 15: the passcode prompt and the seven
carer-mode screens, all drawn from the same primitives).
The mascot layer is `FSBL/Src/ui/anime_ui.c`.

Everything is 800×480, RGB565, drawn CPU-side, into **one** framebuffer at
`BUFFER_ADDRESS` (0x34200000). Session 13 tried to add a second one at
`GUI_BUFFER_ADDRESS` so the UI could keep drawing during inference, and
proved on hardware that it does not fit: a canary written into
`GUI_BUFFER_ADDRESS` came back overwritten with float bit patterns. **Session
15 established the exact extent** by extracting every address literal from the
generated networks (`documents/MEMORY_MAP.md` §2): the activations occupy one
contiguous block, **0x34200000-0x34387FFF**, and `GUI_BUFFER_ADDRESS`
(0x342BB800-0x34376FFF) is entirely inside it. The revert was correct, and now
there is arithmetic behind that rather than one canary.

What replaced it: during the capture window the LTDC layer is **disabled**
(`display_blank()` in `state_machine.c`), so the panel shows the LTDC
background colour — set to the theme blush in `main.c` — instead of a frozen
camera frame being visibly scribbled over. See §3.2.

---

## 1. The screens

| # | State | Drawn by | What the user sees | Exits via |
|---|---|---|---|---|
| 1 | `STATE_HOME` | `gui_draw_home_screen()` | Teal title bar "SMART PILL DISPENSER"; two large buttons, **REGISTER PATIENT** (teal) and **DISPENSE PILLS** (amber); a bordered dialog box with Lumio's greeting; the mascot animating at the right. An SD-card warning appears in the dialog box when the card is missing. | Tap either button |
| 2 | `STATE_INSTRUCT_REGISTER` | `gui_draw_ready_screen()` | Teal title bar **"REGISTER PATIENT"**, distinct wording ("Let's get you set up!…") from screen 7, one full-width **I AM READY** button. | Tap READY (after a 500 ms dwell guard) / USER1 |
| 3 | `STATE_CAMERA_REGISTER` | *(live camera, then a blanked panel)* | 1.5 s of live camera preview so the user can frame themselves; then the camera stops, the LTDC layer is disabled, and the panel holds a plain blush field for the ~750 ms the NPU needs. Nothing is drawn during that window — the NPU owns the only framebuffer. | Face found → 4; not found → 11; USER1 → 1 |
| 4 | `STATE_KEYBOARD_REGISTER` | `registration_ui_draw_keyboard()` | Rose title bar, full-screen uppercase QWERTY (3 letter rows + SPACE / DEL / DONE), **70×60 px** keys with a 5 px gap (was 70×56), name field with a trailing `_` cursor. | DONE with a non-empty name → 5; USER1 → 1 |
| 5 | `STATE_PILLCOUNT_REGISTER` | `registration_ui_draw_pillcount()` | Rose title bar; a row of **four hopper slots** drawn as coloured pill lentils — slot **A** live (tap it to add a pill), **B/C/D** greyed and inert with a "SOON" label and a caption saying they unlock with the carer app; a large centred digit; RESET and NEXT side by side. Range 1–10. | NEXT → 6; USER1 → 1 |
| 6 | `STATE_CONFIRM_REGISTER` | `registration_ui_draw_confirm()` | Teal title bar "CONFIRM REGISTRATION", the entered name and pills-per-dose, and two centred buttons **CONFIRM** / **RETRY** (now sharing the same button geometry as screen 11's TRY AGAIN/CANCEL, rather than a near-duplicate size). | CONFIRM → saves, 2 s "Registered!" dialog → 1; RETRY → 3 |
| 7 | `STATE_INSTRUCT_DISPENSE` | `gui_draw_ready_screen()` | Amber title bar **"DISPENSE PILLS"**, distinct wording ("Let's find your pills!…") from screen 2 — visually and textually distinguishable at this step now. | Tap READY → 8; USER1 → 1 |
| 8 | `STATE_CAMERA_DISPENSE` | *(live camera, then a blanked panel)* | Identical to screen 3. | Match → 9; no match / no face → 11; USER1 → 1 |
| 9 | `STATE_DISPENSING` | `gui_draw_dispensing_screen()` + `gui_draw_dispensing_progress()` | Green title bar "DISPENSING", the patient's name, their dose ("4 pills"), **one gem per pill** which fills in from grey to hopper colour as the dose is released, and a bordered progress bar that fills left-to-right over ~2 s. The loop counts pills, not fixed steps, so the bar, the percentage below it and the lit gems always agree — pill 3 of 6 is a half-full bar reading 50%. | Automatic → 10 |
| 10 | `STATE_CONFIRM_TAKEN` | `gui_draw_confirm_taken_screen()`, then `gui_draw_taken_thankyou_screen()` | Amber title "TAKE YOUR PILL NOW", a large green **I TOOK IT!** button (darkened this session for real contrast — see below) filling most of the screen, a small grey **SKIP** in the top-right (also darkened). On confirm, a styled green-titled "THANK YOU!" screen for 1.5 s (was bare text on white with no styling at all). | I TOOK IT / SKIP / 30 s timeout → 1 |
| 11 | `STATE_FACE_RETRY` | `gui_draw_two_choice_screen()` + `anime_ui_update()` | Amber title "FACE NOT RECOGNISED", the designer's **crying mascot animating on the left** (`MASCOT_ERROR`, three frames), advice text that differs by cause to its right, two centred buttons **TRY AGAIN** / **CANCEL**. | TRY AGAIN → 3 or 8; CANCEL / 30 s → 1 |
| 12 | `STATE_ALERT` | `gui_draw_alert_screen()` | Coloured title bar (red or amber), a message, one wide **OK** button. Raised for: gallery full (on tapping REGISTER), the AI pipeline failing to initialise, and — Session 15 — a carer edit that reached RAM but not the SD card. | OK / 30 s → wherever the alert was told to return |

### Session 15 — the passcode gate and carer mode

Screen 13 is reachable from **two** places and is the only way into 14–20.
There is no third way in and no shortcut between them.

| # | State | Drawn by | What the user sees | Exits via |
|---|---|---|---|---|
| 13 | `STATE_PASSWORD` | `carer_ui_draw_prompt()` | Rose title bar — **"CARER ACCESS"** when reached by the hidden gesture, **"A CARER SETS THIS UP"** when reached by REGISTER PATIENT, because the person most likely to see the second one is a patient who pressed the wrong button. A phone-layout numeric keypad (12 keys at 128×66 px, nearly twice the area of the QWERTY keys), the entered digits shown as **dots, never digits**, and a small CANCEL. Wrong entries redraw with the reason in red. | Correct → 14 or 2; CANCEL / 30 s → 1; 5 wrong → refuses for 30 s |
| 14 | `STATE_CARER_MENU` | `carer_ui_draw_menu()` | Rose title "CARER MODE", the current date/time and time-source mode on a status strip (red if the clock has never been set), four buttons — SET CLOCK, PATIENTS, REVIEW LOG, CHANGE CODE (subtitled "still the default!" when it is) — and EXIT. | Any button → 15–18; EXIT → 1, re-arming the schedule |
| 15 | `STATE_CARER_CLOCK` | `carer_ui_draw_clock()` | **The same numeric keypad as screen 13.** The carer types twelve digits and the screen shows them landing in `DD/MM/YYYY  HH:MM`, with `_` for what is still to come; the current clock sits underneath as a reference. DEL backspaces, OK validates and saves, CANCEL leaves. An invalid entry names the field that is wrong ("That day does not exist in that month") rather than saying "invalid". | OK → "Clock set." then 14, re-arming the schedule; CANCEL → 14 |
| 16 | `STATE_CARER_PATIENTS` | `carer_ui_draw_patients()` | Green title "PATIENTS", up to three rows per page, each showing a name and their whole plan on one line ("3 pills   08:00, 20:00" or "no schedule"), a page indicator, BACK / PREV / NEXT. Says so plainly when nobody is registered. | Tap a row → 17; BACK → 14 |
| 17 | `STATE_CARER_PATIENT` | `carer_ui_draw_patient_menu()` | The patient's name as the title, their plan beneath it, and three buttons: **DOSE TIMES**, **DOSE SIZE**, and **DELETE** (red). | → 18, 19, 20; BACK → 16 |
| 18 | `STATE_CARER_SCHEDULE` | `carer_ui_draw_schedule()` | **Two modes on one screen.** *Grid:* four slots showing `HH:MM` or `--:--`, a CLEAR for the selected slot, and a **live** clock strip. *Entry:* tapping a slot opens the same numeric keypad as screens 13 and 15, four digits into `HH : MM`, OK to commit back to the grid. One tap to start typing. BACK / SAVE on the grid. | SAVE → "Dose times saved." then 17, or an alert if the card write failed; BACK → 17 |
| 19 | `STATE_CARER_DOSE` | `carer_ui_draw_dose()` | A large digit, **the same pill lentils the dispense screen uses** so "3" here and three gems during a dispense are visibly the same fact, − / + over 1–10, BACK / SAVE. | SAVE → "Dose size saved." then 17; BACK → 17 |
| 20 | `STATE_CARER_LOG` | `carer_ui_draw_log()` | Rose title "DOSE HISTORY" and up to eight lines read back from the **end** of `events.log`, filtered to the selected patient and to the lines that answer the question — CONFIRMED, MISSED, DISPENSE, SKIPPED. **Missed doses are the one line that is not grey.** Says plainly when there is no card or nothing recorded. | BACK → 17 or 14 |
| 21 | `STATE_CARER_CHANGE_CODE` | `carer_ui_draw_change_code()` | The same keypad as screen 13, twice: enter a new code, then confirm it. Deliberately does **not** ask for the old code — the carer entered it thirty seconds ago to get here. | Both entries match → saved, then 14; CANCEL → 14 |
| 22 | `STATE_CARER_DELETE_CONFIRM` | `gui_draw_two_choice_screen()` | Red title "DELETE PATIENT", the crying mascot, "Remove <name> completely? Their name and their face are both deleted.", and **DELETE** / **KEEP**. | DELETE → 16, schedule re-armed; KEEP / 30 s timeout → 17 |

**The delete screen times out to KEEP, not to DELETE.** That is the only
correct default for a destructive confirmation and it is worth saying out
loud, because every other timeout in this UI goes forward rather than back.

**Why the clock screen was redesigned after the first hardware round.** It
originally used five stepped fields and a big − / +, chosen so an invalid
date would be impossible to enter. It achieved that and it was miserable:
setting a year, a day and a time from the device's power-on default is on the
order of a hundred taps. "Impossible to enter an invalid value" was the wrong
thing to optimise for — a carer sets the clock while looking at their phone
and already knows the date; the job is to let them **type** it. Twelve digits
and an OK is thirteen taps, and validation is one function call at the end.

**The dose-times screen went the same way, for the same reason.** It kept a
15-minute stepper at first, and the second hardware round's log shows what
that cost: the schedule screen was opened and saved **six times in one
session**, because moving from 04:00 to 06:15 is nine taps and moving to an
arbitrary time is worse. Tapping a slot now opens the keypad directly.

That makes **three screens driven by one keypad** — passcode, clock, dose
time — which is the point: a carer learns it once.

Stepping survives on exactly one screen, **dose size** (1–10), where every
value is at most five taps away and the control shows the whole range
implicitly. That is what stepping is good at. A four-digit year or an
arbitrary time is what it is worst at.

### The clock ticks now

Every screen that shows the clock strip — the carer menu, the clock screen,
the dose-times grid, **and the home screen** — updates it live.
`carer_ui_clock_tick()` runs once per UI pass, redraws only when the
displayed text actually changes, and flushes only that 24-pixel band. In demo
mode that is about once a real second; in a real build, once a minute.

It was drawn once on screen entry before, which in demo mode meant a clock
that was visibly frozen while a simulated minute passed every second — worse
than no clock on a screen whose whole job is scheduling. The demo strip shows
seconds as well, because at that ratio a minutes-only clock sits still for a
whole second and then jumps, which reads as a bug rather than as a fast
clock.

### The hidden entry gesture

Carer mode is reached by tapping the **home screen's title bar five times
within three seconds**. The title bar is the target because it is the one
element on that screen that is unmistakably not a button, so tapping it
repeatedly is not something anyone does by accident — and it is large enough
(320×68 px) to hit reliably with an unsteady hand. Any tap elsewhere resets
the count, so five accidental taps spread across a session never accumulate.

Five and three seconds are decisions: one or two taps are plausible accidents
and ten is a chore to explain over the phone; three seconds is comfortably
long for a deliberate five taps by an older hand.

Plus two overlays that are not states of their own:

- **Dialog text** (`gui_draw_dialog_text()`) — rewrites the bottom dialog box
  in place. Used for "Registered!", the SD-card warning, and transient
  messages.
- **The mascot** (`anime_ui_update()`) — drawn directly into the framebuffer,
  never during a capture. Idle loop on screens 1, 2 and 7; the crying
  `MASCOT_ERROR` loop on screens 11 and 22, **and on the home screen after a
  missed dose**. That last case is why the error pose's position is now a
  parameter (`anime_ui_set_error_box()`) rather than the `MASCOT_SAD_*`
  constant it was hard-coded to: on the home screen that rectangle sits on
  top of the REGISTER and DISPENSE buttons, and the crying mascot was being
  drawn straight over them. The sprite is centred in whatever box the screen
  nominates.
- **The dose-due banner** (Session 15) — not a screen and not a state. When a
  scheduled window opens, the home screen's dialog box carries
  "<name> — your 08:00 pills are due now. Tap DISPENSE MEDICINE." It is drawn
  last, so it wins the panel over the SD-card warning when both apply: a dose
  being due is time-critical and the SD warning is not. When the clock has
  never been set, that same panel says so instead — a device that cannot tell
  the time cannot remind anyone, and the home screen is the only place a carer
  will find that out before a dose is missed.
- `MASCOT_ACTIVE` is selected when a window opens, but it has no artwork yet
  and renders the idle loop, so **the banner is the reminder, not the mascot**.

---

## 2. Design system (Session 13)

The whole UI is now drawn from the project designer's artwork, cut into
sprites and fonts at build time by `tools/gen_ui_assets.py` and packed into
`Inc/ui/ui_assets.h` + `ui_assets_data.inc` (~157 KB of ROM).

- **One frame**, `gui_draw_frame()` — a blush card with a rounded white
  panel inside it, a heart motif top-left and bottom-right and a capsule
  top-right and bottom-left. Every screen sits inside it.
- **One title-bar function**, `gui_draw_title_bar(title, accent)`, used by
  every screen including the three registration screens (which previously
  had none). Accent carries meaning: rose (register flow), amber (dispense
  flow), red (alerts), green (success).
- **One button primitive**, `gui_draw_button()` — rounded, two-tone fill and
  edge, measured-centred label. `registration_ui.c` no longer keeps its own
  button style.
- **Real fonts.** Four 4bpp anti-aliased proportional faces rasterised from
  DejaVu Sans Bold Oblique (lg 34 px, md 23 px, sm 18 px, and a 76 px
  digits-only face for the pill count), replacing the scaled 8×8 bitmap.
  `gui_font_text_centered()` centres per line from real glyph advances.
- **One two-button layout** (`CHOICE_*`, `gui_draw.h`) shared by the
  confirm-registration and face-retry screens, which previously had two
  similarly-sized-but-not-identical private geometries.
- **Pills are gems**, not capsules: `gui_draw_gem()` draws an anti-aliased
  sugar-shell lentil with a rim and a glint, from one colour parameter.
  Colour is identity — one colour per hopper, the same colour for that
  medication everywhere it appears — and `GEM_OFF` grey means "slot exists,
  not fitted". This is what makes the one-hopper prototype legible as a
  four-hopper design without claiming hardware it does not have.
- **The mascot is the designer's cat**, not a procedural approximation:
  214×213 idle poses (arms down / arms up / blink) with sparkles and a
  breathing bob, and a 150×153 three-frame crying pose for `MASCOT_ERROR`.

## 3. Known problems, current status

Recorded from the code, session 13's changes, and what remains open.

### 3.1 The mascot has two states

`mascot_state_t` still defines `MASCOT_IDLE`/`ACTIVE`/`SUCCESS`/`ERROR`.
Two of those are real: `MASCOT_IDLE` (the 8-frame idle loop on the home,
instruction screens) and `MASCOT_ERROR` (the designer's three crying frames,
animated on the FACE NOT RECOGNISED screen). `anime_ui_set_state()` is
called from `state_machine.c` on entry to `STATE_FACE_RETRY` and reset to
idle on all three of that screen's exits.

`MASCOT_ACTIVE` and `MASCOT_SUCCESS` still fall back to the idle loop — no
artwork has been cut for them. Adding more states is a design decision, not
a rule: the earlier "idle-only, do not reopen" wording in this file was
overridden by the project owner during Session 13.

### 3.2 Framebuffer corruption during capture — MITIGATED, not eliminated

Was: both NPU networks' activation scratch scribbled over whatever was on
screen during every face capture, visibly, for up to a few seconds.

The intended fix — a second framebuffer at `GUI_BUFFER_ADDRESS` for the UI
— was implemented, canary-tested on hardware, and **failed**: the canary
came back overwritten, so the NPU scratch runs past the second buffer as
well. Per `prompts/session_13.md` ("if it does not fit, say so and stop"),
it was reverted rather than worked around by shrinking buffers or moving
NPU pools.

Now: the LTDC layer is disabled for the duration of the capture
(`display_blank()`), so the panel shows a flat blush field instead of a
corrupting frame. It reads as a deliberate pause rather than a fault. The
real fix needs the NPU activation pool relocated, which is a code-generation
change outside this session's scope.

### 3.3 Register vs. Dispense at the instruction step — FIXED

Screens 2 and 7 now have different title text, different accent colour, and
different dialog copy.

### 3.4 Nothing tells the user the device is thinking — PARTIALLY ADDRESSED

The planned "CHECKING…" screen depended on the second framebuffer and died
with it (§3.2). The blanked panel is a weaker signal: it is clearly not a
glitch, but it carries no text. If the NPU pool is ever relocated, this is
the first thing to build on top of it.

### 3.5 Inconsistent visual language — FIXED

One frame, one title-bar treatment, one button primitive, one two-button
layout, one font family and measured text centring replace what used to be
four separate button geometries and three title-bar styles.

### 3.6 Text rendering — FIXED

Was: a single hand-rolled 8×8 bitmap scaled by integer replication. Now:
four anti-aliased proportional faces generated at build time (§2). The old
`font8x8` table and `gui_draw_text()` remain in `gui_draw.c` for the few
call sites that have not been migrated, but no screen depends on them for
its primary text.

### 3.7 Elderly-friendliness — validated this session, three real bugs found and fixed

The palette was checked against WCAG AA contrast ratios for the first time.
Three colours failed: `COLOR_BTN_GREEN` (≈2.07:1) and `COLOR_GRAY`
(≈1.77:1) against white, and the rose title text (≈2.85:1) against the
white panel — all under the 3:1 floor for large text. All three were
darkened; see `session_13_notes.md`'s contrast table for the full palette.
QWERTY keys were raised from 70×56 to 70×60.

### 3.8 Minor

- `gui_draw_mascot_bg()` is still a no-op kept only for API compatibility.
- `STATE_ALERT`'s message strings are still stored as `const char *`
  pointers to literals — fine today, fragile if a caller ever passes a
  stack buffer.
- The dialog panel steps its font down to `ui_font_sm` when a message does
  not fit; messages longer than two lines still need shortening by hand.
