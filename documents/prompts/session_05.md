# Session 05 — Interactive GUI (Touch + Mascot States)

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Upgrade the idle mascot overlay from Session 04 into a real interactive UI: touch input
via the board's on-board capacitive touch panel, tappable buttons, and a mascot state
machine that reacts to both touch and system events.

BACKGROUND AND CONTEXT
Building on tron/session_04/ (idle mascot animation looping over the live camera feed).
The STM32N6570-DK's 5" LCD ships with an on-board capacitive touch panel — confirmed
via prior real bring-up on this exact board to be a **GT911 controller on I2C2, pins
PD14 (SCL) and PD4 (SDA)**, requiring the **FSBL** context selected in the `.ioc` file.
This is existing hardware, not a new component to wire up — this session is a
driver/software task only. See ENGINEERING_LESSONS.md before starting: an earlier
attempt at this exact peripheral (I2C touch) failed by manually copying HAL driver
files and hand-editing generated Makefiles instead of using STM32CubeMX properly — the
correct approach is enabling I2C2 in the `.ioc` file and generating code from there,
nothing else. See MASCOT_UI_DESIGN.md for the character brief and animation states
this session wires up interactively.

CRITICAL CONSTRAINT — READ BEFORE STARTING
The mascot remains a wholly original character design per MASCOT_UI_DESIGN.md. Do NOT
introduce any Pokémon-style, anime-franchise-referencing, or otherwise
copyrighted/trademarked visual design into the buttons, mascot states, or any new UI
assets this session adds — this rule applies to every UI session, not just Session 04.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ui/ (extend from Session 04)
- ENGINEERING_LESSONS.md (mandatory read before this session — I2C2/.ioc/Makefile
  rules specific to this exact peripheral)
- MASCOT_UI_DESIGN.md (animation state table — this session makes those states
  reachable via touch/system events instead of always sitting idle)
- SOFTWARE_ARCHITECTURE.md (module boundary rules, folder structure)
- tron/session_04/ as the base

REQUIRED INPUTS
- None beyond the on-board GT911 touch panel already present on the DK's LCD module.
  Enable I2C2 via the `.ioc` file (FSBL context) per ENGINEERING_LESSONS.md — do not
  manually add I2C HAL driver files to the project.

EXPECTED OUTPUTS
- touch_driver.c/.h: I2C2 driver for the on-board GT911 touch controller (pins PD14
  SCL / PD4 SDA), exposing a simple polling or interrupt-based API returning touch
  coordinates and press/release state.
- interactive_gui.c/.h: a small GUI layer drawing the two main-screen buttons —
  **"Register"** and **"Dispense Medicine"** (per MASCOT_UI_DESIGN.md §5 — these are
  the actual final button labels, wired to debug output only this session; Session 09
  gives "Register" its real registration-flow behavior, Session 11 gives "Dispense
  Medicine" its real dispense-flow behavior) — and managing a MASCOT_STATE enum:
  MASCOT_IDLE, MASCOT_ACTIVE, MASCOT_SUCCESS, MASCOT_ERROR.
- Tapping a button transitions the mascot state (e.g. to MASCOT_ACTIVE) and prints a
  debug UART message; the mascot's rendered animation (via Session 04's anime_ui.c)
  updates to match the new state.

CONSTRAINTS
- Touch polling/interrupt handling must not block or freeze the main loop or stall the
  camera DMA pipeline (same non-blocking rule as Session 04's animation).
- No RTOS yet — this session is still bare-metal/superloop; concurrency comes in
  Session 07.
- Keep touch_driver.c and interactive_gui.c separate — the I2C touch driver shouldn't
  contain UI/state logic, and the GUI layer shouldn't contain raw I2C calls.

CODING STANDARDS
- MASCOT_STATE as a proper enum, not magic integers.
- Button hit-testing (touch coordinate → button region) implemented as a clean,
  reusable function — later sessions will add more buttons.

FOLDER STRUCTURE TO FOLLOW
- FSBL/Src/ui/touch_driver.c, FSBL/Inc/ui/touch_driver.h
- FSBL/Src/ui/interactive_gui.c, FSBL/Inc/ui/interactive_gui.h

FILES TO CREATE
- FSBL/Src/ui/touch_driver.c
- FSBL/Inc/ui/touch_driver.h
- FSBL/Src/ui/interactive_gui.c
- FSBL/Inc/ui/interactive_gui.h

FILES TO MODIFY
- FSBL/Src/main.c (poll/service touch input, call into interactive_gui)
- FSBL/Src/ui/anime_ui.c (accept a MASCOT_STATE input to select which animation to
  render, replacing the Session 04 always-idle behavior)

DOCUMENTATION TO UPDATE
- docs/milestones/session_05_notes.md: touch controller part/protocol details, I2C
  bus/pins used, button hit-test regions chosen.
- Update SOFTWARE_ARCHITECTURE.md's pin map with the touch controller's I2C pins.
- Update MASCOT_UI_DESIGN.md if the actual implemented state transitions differ from
  the original brief.

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly.
- (Manual) Tapping each drawn button reliably registers the correct touch coordinates,
  transitions MASCOT_STATE correctly, updates the rendered mascot animation, and prints
  the expected debug message — with the camera preview remaining smooth throughout.

COMPLETION CHECKLIST
- [ ] I2C touch controller driver implemented (touch_driver.c/.h)
- [ ] MASCOT_STATE enum defined (IDLE, ACTIVE, SUCCESS, ERROR)
- [ ] Register and Dispense Medicine buttons drawn and hit-tested correctly
- [ ] anime_ui.c updated to render per current MASCOT_STATE
- [ ] Camera preview remains smooth (no regression from Session 03/04)
- [ ] session_05_notes.md written; pin map and MASCOT_UI_DESIGN.md updated as needed

COMMON PITFALLS
- Touch coordinate system not matching the LCD's actual orientation/resolution —
  verify all four corners and center, not just one tap location.
- Polling touch too infrequently (sluggish response) or too aggressively (starves the
  camera/animation loop) — tune and note the polling interval chosen.
- Reaching for "anime-style" button/icon art that drifts toward existing franchise
  visual language — re-check against MASCOT_UI_DESIGN.md's original-character brief.

DEFINITION OF DONE
Touching either on-screen button reliably and correctly changes the mascot's state and
rendered animation, with a confirming debug message, and the camera preview stays
smooth throughout.

SELF-REVIEW BEFORE DECLARING COMPLETE
Tap all four corners of the screen and the exact center to confirm coordinate mapping
is correct, not just the button regions you happened to test during development.
```

## Expected Deliverables
`touch_driver.c/.h`, `interactive_gui.c/.h`, updated `anime_ui.c`, updated pin map.

## Manual Verification Steps
1. Flash and observe the LCD with two buttons drawn.
2. Tap each button; confirm correct mascot state change, animation update, and debug
   message.
3. Tap all four screen corners and center; confirm accurate coordinate mapping.
4. Confirm camera preview stays smooth throughout touch interaction.

## Acceptance Criteria
Reliable, accurately-mapped touch input driving correct mascot state transitions, no
camera pipeline regression.

## Next Prompt
Copy to `tron/session_05/`, proceed to `session_06.md`.
