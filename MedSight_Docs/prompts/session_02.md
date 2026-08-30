# Session 02 — UART Debug Logging

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Add UART-based debug logging so the developer can observe system state over a serial
terminal, before any display or complex peripherals are introduced.

BACKGROUND AND CONTEXT
Building directly on tron/session_01/ (LED blink confirmed working). We need visible
debug output for every subsequent session — this is infrastructure, not a feature.

RELEVANT PROJECT FILES AND FOLDERS
- Core/Src/main.c, Core/Inc/main.h (from session_01)

REQUIRED INPUTS
- The working project from tron/session_01/.

EXPECTED OUTPUTS
- printf retargeted to USART1 at 115200 baud, 8N1.
- A DEBUG_LOG(...) macro wrapping printf for consistent use in later sessions.
- main.c prints "MedSight Session 02 Active" once at boot, then logs an incrementing
  tick value once per second.

CONSTRAINTS
- Use USART1 specifically, 115200 baud.
- Do not introduce RTOS, camera, or AI code this session.
- Keep the LED blink from Session 01 working alongside the new UART output.

CODING STANDARDS
- DEBUG_LOG macro should be trivially disable-able later (e.g. via a single #define)
  without touching call sites, since later sessions must never leak sensitive data
  (see COMPLIANCE_PRIVACY_POSTURE.md — no biometric payloads over this channel, ever,
  in any future session).

FOLDER STRUCTURE TO FOLLOW
Same as Session 01 — no restructuring.

FILES TO CREATE
- None new required; a small debug_log.h with the macro is acceptable if it keeps
  main.c clean.

FILES TO MODIFY
- Core/Src/main.c
- Possibly Core/Src/syscalls.c or equivalent for _write()/__io_putchar() retargeting

DOCUMENTATION TO UPDATE
- docs/milestones/session_02_notes.md: which UART peripheral/pins were used, and
  confirmation the LED blink still works unmodified.

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly.
- (Manual) Serial terminal at 115200 8N1 shows "MedSight Session 02 Active" followed by
  incrementing tick messages, once per second.

COMPLETION CHECKLIST
- [ ] USART1 initialized at 115200 8N1
- [ ] printf/_write retargeting implemented
- [ ] DEBUG_LOG macro defined and used
- [ ] LED blink from Session 01 still functions
- [ ] docs/milestones/session_02_notes.md written

COMMON PITFALLS
- Forgetting to route UART through the on-board ST-LINK virtual COM port rather than a
  separate physical UART pin — check which one the DK board exposes by default.
- Buffering issues with printf if the C library's stdout isn't flushed per line —
  verify each log line actually appears promptly, not batched.

DEFINITION OF DONE
Terminal reliably shows boot message plus incrementing ticks. LED still blinks. Docs
updated.

SELF-REVIEW BEFORE DECLARING COMPLETE
Confirm no sensitive-data logging patterns were introduced (this session has no
sensitive data yet, but the macro's design should make it easy to avoid later).
```

## Expected Deliverables
UART debug logging implementation, `session_02_notes.md`.

## Manual Verification Steps
1. Flash the board.
2. Open a serial terminal (115200 8N1) on the ST-LINK virtual COM port.
3. Confirm boot message and incrementing ticks appear once per second.
4. Confirm LED still blinks.

## Acceptance Criteria
Stable, correctly-timed serial output across at least 60 seconds of observation, no
dropped or garbled lines.

## Next Prompt
Copy to `tron/session_02/`, proceed to `session_03.md`.
