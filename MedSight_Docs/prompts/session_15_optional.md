# Session 15 (Optional Stretch) — Boot-Time Demo/Project Selector

**This session is explicitly optional.** It's a nice-to-have polish item, not a
contest requirement, and it touches the boot path — a genuinely riskier place to make
changes than anywhere else in the project. Only attempt this after Session 14's
submission-ready baseline is safe, ideally after actual submission, not before. If it
turns out harder than expected, drop it — nothing else in the project depends on it.

## What This Is

Store the final MedSight application in non-volatile flash such that on every power-on,
the board boots to some default/demo behavior, but with a simple, discoverable way
(e.g. holding a button during boot, or a boot-time menu on the LCD) to instead launch
the actual MedSight application. This is presentation polish for demoing the board to
other people without it always jumping straight into a live patient-facing flow. Build
the demo screen the same way as the rest of the app's UI — hand-rolled DMA2D/LTDC
compositing, same as Sessions 04/05/09/11 — no new toolchain needed for this.

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Implement a simple boot-time selector: default boot goes to a lightweight demo/idle
screen; a clear, discoverable user action launches the full MedSight application
instead.

BACKGROUND AND CONTEXT
Building on tron/session_14/ — the submission-ready, µT-Kernel 3.0-compliant baseline.
This is optional post-submission polish, not a functional requirement. If STM32N6's
boot/flash layout makes true dual-image boot selection impractical in the time
available, a much simpler acceptable fallback is: boot to a static "Welcome — hold
[button] for 2 seconds to start MedSight" screen, then launch the real application
from within the same image after that hold, rather than true separate boot images.
Prefer the simple fallback over a risky bootloader-level implementation if time is
short — this session's value is the demo experience, not the specific mechanism.

RELEVANT PROJECT FILES AND FOLDERS
- Whatever boot/startup files are relevant to your chosen approach (either linker/boot
  configuration for true dual-image selection, or main.c for the simpler
  hold-to-start fallback)
- tron/session_14/ as the base

CONSTRAINTS
- Do not modify anything in the already-frozen Session 12/13 µT-Kernel baseline logic
  itself — this session adds a selector in front of it, it doesn't change how the
  application behaves once launched.
- If attempting true dual-image boot selection: back up tron/session_14/ before
  touching flash layout/linker scripts — a mistake here can require a full reflash via
  the ST-LINK to recover, which is a bigger risk than anywhere else in the project.
- Prefer the simple hold-to-start fallback unless you have specific confidence in the
  dual-image approach and time to properly test it.

EXPECTED OUTPUTS
- A boot-time screen that doesn't immediately drop into the live patient-facing
  dispense flow.
- A clear, discoverable action that launches the real MedSight application from there.

DOCUMENTATION TO UPDATE
- docs/milestones/session_15_notes.md: which approach was taken (true dual-image vs.
  hold-to-start fallback) and why, plus recovery steps if something goes wrong with
  the boot path.

VALIDATION AND TESTING REQUIREMENTS
- (Manual) Power-cycle the board multiple times; confirm it reliably reaches the
  demo/idle screen each time, and that the discoverable action reliably launches the
  full application each time.
- (Manual) Confirm you can still reflash tron/session_14/'s image directly via
  ST-LINK if this session's changes cause a problem — verify your recovery path
  before considering this session done.

COMPLETION CHECKLIST
- [ ] Boot-time demo/idle screen implemented
- [ ] Discoverable action reliably launches the full application
- [ ] Recovery path (reflash to Session 14's baseline) confirmed still working
- [ ] session_15_notes.md written

DEFINITION OF DONE
The board reliably boots to a non-patient-facing demo screen, with a reliable,
discoverable way to launch the real application, and you've confirmed you can still
recover to the Session 14 baseline if needed.

SELF-REVIEW BEFORE DECLARING COMPLETE
Confirm the Session 12/13 µT-Kernel application logic itself is byte-for-byte
unchanged — this session only adds something in front of it.
```

## Expected Deliverables
Boot-time selector (dual-image or hold-to-start fallback), session notes, confirmed
recovery path.

## Manual Verification Steps
1. Power-cycle several times, confirm reliable boot-to-demo behavior.
2. Confirm the launch action reliably starts the real application.
3. Confirm you can still reflash the Session 14 baseline directly if needed.

## Acceptance Criteria
Reliable boot selector behavior, confirmed recovery path, zero changes to the frozen
application logic.

## Next Prompt
None — this is the final optional session.
