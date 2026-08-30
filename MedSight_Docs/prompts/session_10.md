# Session 10 — Multi-Hopper Dispenser Mechanism

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Implement a multi-hopper motor-control driver: each hopper is one medication type,
independently addressable, with its own release mechanism, its own IR break-beam
sensor, and its own jam/timeout handling — abstracted behind a single
dispense_dose(hopper_id, count) entry point.

BACKGROUND AND CONTEXT
Building on tron/session_09/ (Registration Flow working — patients enrolled, medicine
catalog defined). MedSight is a shared device
serving multiple people on multiple medications, not a single-medicine device — see
MECHANICAL_DESIGN.md §1. The contest-deadline build target is 2-3 physically wired
hoppers; the architecture must support up to 6-8 without firmware changes, per
MECHANICAL_DESIGN.md §1-2. Per HARDWARE_ARCHITECTURE.md §2, each hopper's motor and IR
sensor are bench-wired by the human developer before this session — Antigravity writes
the control code only.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ (new dispenser.c/.h)
- MECHANICAL_DESIGN.md (multi-hopper architecture, per-hopper mechanism, jam-handling
  requirements)
- HARDWARE_ARCHITECTURE.md §2 (per-hopper motor/IR sensor wiring pattern)
- tron/session_09/ as the base

REQUIRED INPUTS
- 2-3 hopper modules physically wired per HARDWARE_ARCHITECTURE.md (human task, done
  before this session starts): each a duplicate of the Mr Innovative turntable/guide
  reference design (see MECHANICAL_DESIGN.md §3), with a 28BYJ-48 unipolar stepper
  motor driven via a ULN2003 driver board (4 GPIO lines per hopper, direct
  coil-sequence drive), and its own local IR break-beam sensor.
- A hopper configuration (compile-time array or simple config struct is fine for this
  session) mapping hopper_id -> {4 ULN2003 GPIO pins, IR sensor pin, turntable/guide
  parameters for that hopper's specific pill}.

EXPECTED OUTPUTS
- A dispenser.c/.h exposing a single abstracted entry point, e.g.
  `dispenser_result_t dispense_dose(uint8_t hopper_id, uint8_t count)`, that:
  - drives the specified hopper's 28BYJ-48 stepper through its coil sequence to spin
    the turntable, singulating loose pills through the passive guides one at a time
    (per the Mr Innovative reference design, MECHANICAL_DESIGN.md §3),
  - counts drops via that hopper's own IR break-beam sensor until `count` is reached,
  - times out and returns a jam/incomplete result — scoped to that specific hopper —
    if the target count isn't reached within a defined window, rather than retrying
    motor actuation blindly.
- EXTI interrupt handling per hopper's IR sensor with debounce logic (rejecting rapid
  multi-triggers from a single pill's mechanical edge passing the beam).
- All of this exposed as OSAL-task-safe functions (per the Session 07 OSAL rules) — no
  direct FreeRTOS calls.
- Support for at least 2 hoppers wired and independently testable this session; the
  data structures must not hardcode a hopper count of 1 anywhere.

CONSTRAINTS
- `dispense_dose()`'s public signature must not leak mechanism-specific details
  (e.g. raw motor pulse counts) to its callers — callers pass hopper_id and count only.
- Motor actuation code path must be behind a clear per-hopper abstraction so adding a
  physical hopper later (up to the 6-8 design target) means adding a config entry, not
  touching dispense_dose()'s callers.
- EXTI debounce must reject rapid multi-triggers per hopper.
- A jam/timeout on one hopper must not block or abort dispensing from other hoppers
  in the same dose event — each hopper's dispense_dose() call is independent.
- This session does not yet hook into the overall state machine (that's Session 11) —
  provide a simple test entry point (e.g. a debug command or button, reusing the
  interactive GUI's "Dispense Medicine" button from Session 05) to trigger a test
  dispense from a chosen hopper for manual verification.

FILES TO CREATE
- FSBL/Src/dispenser.c
- FSBL/Inc/dispenser.h

FILES TO MODIFY
- FSBL/Src/main.c or FSBL/Src/ui/interactive_gui.c (wire the Session 05 "Dispense Medicine"
  button placeholder to call dispense_dose() against a selectable test hopper for
  manual test purposes)
- Update SOFTWARE_ARCHITECTURE.md's pin map with each wired hopper's motor-driver
  control pins and IR EXTI pin

DOCUMENTATION TO UPDATE
- docs/milestones/session_10_notes.md: how many hoppers were physically wired for this
  session, per-hopper motor type actually used, timeout value chosen for jam
  detection, debounce parameters, and confirmation the hopper_id abstraction was
  tested with more than one hopper (not just hopper 0).
- SOFTWARE_ARCHITECTURE.md pin map (per hopper).
- MECHANICAL_DESIGN.md if the actual per-hopper mechanism diverged from the
  Mr Innovative turntable/guide reference design.

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly.
- (Manual) For each wired hopper: triggering dispense_dose(hopper_id, count) actuates
  that hopper's motor correctly and counts the correct number of drops via SD logger;
  a test with fewer pills available than `count` (or an empty hopper) logs the
  jam/incomplete event for that specific hopper instead of a false success; triggering
  dispense on one hopper does not disturb or block a subsequent call to a different
  hopper.

COMPLETION CHECKLIST
- [ ] Abstracted dispense_dose(hopper_id, count) implemented, motor/sensor details
      cleanly hidden behind it
- [ ] At least 2 hoppers wired, configured, and independently testable
- [ ] Per-hopper stepper actuation implemented (turntable singulates & releases pills, counted)
- [ ] Per-hopper EXTI + debounce implemented for each IR sensor
- [ ] Per-hopper jam/timeout detection implemented, scoped to that hopper only
- [ ] Manual test trigger available via the interactive GUI's Dispense Medicine button,
      selectable across the wired hoppers
- [ ] session_10_notes.md and pin map updated

COMMON PITFALLS
- Turntable/guide fit or stepper torque insufficient once a hopper is actually loaded with its real
  pills — bench-test each hopper loaded, not empty, and per its own specific pill type
  (per risk register in MASTER_PROJECT_PLAN.md).
- IR beam false-triggers from ambient light, and this can differ hopper to hopper
  depending on physical placement — verify each hopper under your actual room
  lighting, not just one hopper in isolation.
- Debounce window too short (multi-count a single drop) or too long (miss a fast
  drop) — tune empirically, per hopper if their mechanisms differ.
- Accidentally hardcoding assumptions that only hold for a single hopper (e.g. a
  single global "last dispense result" variable instead of one per hopper) — this
  would silently break as soon as two hoppers are used together, so test with at
  least two hoppers dispensing in the same session, not just one at a time in
  isolation across separate test runs.
- Over-abstracting toward a generic plugin system for mechanisms you're not actually
  building — the abstraction exists for real, planned scaling (2-3 hoppers now, up to
  6-8 later), not speculative generality beyond that.

DEFINITION OF DONE
Triggering a test dispense against each of the 2-3 wired hoppers independently
actuates the correct hopper's motor and correctly counts drops via that hopper's own
sensor; a jam/incomplete test on one hopper doesn't affect the others; the
hopper_id-based abstraction is demonstrated working with more than one hopper, not
just hardcoded for one.

SELF-REVIEW BEFORE DECLARING COMPLETE
Confirm no direct FreeRTOS calls were introduced outside ms_osal.c (same check as
Session 07). Grep for any global/static variable that assumes a single hopper (e.g.
un-indexed "current_count" or "last_result") and confirm state is properly per-hopper.
```

## Expected Deliverables
`dispenser.c/.h` supporting multiple independently addressable hoppers, updated pin
map, session notes with tuned per-hopper mechanical parameters.

## Manual Verification Steps
1. Attach 2-3 hopper modules (motor + local IR sensor each) per HARDWARE_ARCHITECTURE.md.
2. Load each hopper with its own test pills (or leave one empty for the jam test).
3. Trigger a dispense on hopper 0; confirm correct actuation and correct count logged.
4. Trigger a dispense on hopper 1 (a different hopper); confirm it works independently
   and didn't inherit any state from hopper 0's test.
5. Test the empty/insufficient-pills case on one hopper; confirm jam/timeout logging
   scoped to that hopper, with no effect on the other hopper's behavior.

## Acceptance Criteria
Correct, independent actuation and counting across at least two distinct hoppers,
correct per-hopper jam detection, across multiple repeated trials — not just one
hopper tested once.

## Next Prompt
Copy to `tron/session_10/`, proceed to `session_11.md`.
