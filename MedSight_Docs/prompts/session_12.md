# Session 12 - System Hardening, Error Recovery & SD Logging Polish

## Context (for Antigravity to read)
Sessions 09-11 complete: full registration + dispense flow, running on μT-Kernel 3.0.
Action recognition via camera gesture was deprioritised (user confirmed: "OK" button
is the final confirmation model — no action recognition needed).

**Hardware decision (FINAL):** No physical motors/servos/IR. Software-only prototype.
The dispense is simulated. The "OK" button is the production UX.

## Prompt for Antigravity (copy-paste as-is)
```
OBJECTIVE
Harden the system for robust real-world use: error recovery, edge cases, and SD card
logging completeness.

BACKGROUND AND CONTEXT
The prototype currently crashes or hangs in some error paths (SD card removed mid-use,
face not recognised after retries, gallery full). This session makes the system resilient.

EXPECTED OUTPUTS
1. SD Card Hot-Plug Resilience:
   - If SD card is removed during use, log the error, display a non-crashing error screen,
     and auto-retry mount on the next operation.
   - Never call Error_Handler() for SD failures — gracefully degrade.
2. Gallery Full Handling:
   - If MAX_PATIENTS is reached, show "Gallery full — please contact admin" instead of
     silently failing.
3. Face Recognition Retry Logic:
   - If ai_vision_run_pipeline() returns false, retry up to 3 times with a 1s delay each.
   - After 3 failures, show a "Face not recognised" screen with a "Try Again" and "Cancel"
     button instead of hanging.
4. Pill Count Tracking:
   - After each confirmed dispense, decrement patient's remaining_pills count in
     patients.dat. When remaining_pills == 0, show "Refill needed" alert.
5. Full SD Log Review:
   - Ensure every state transition, dispense, and confirmation is logged.
   - Remove any debug printf() calls that could expose patient names over UART in
     production builds (guard with #if DEBUG_UART).
6. Stress test: Register 3 patients, dispense to each in sequence, verify SD log is
   correct, verify face matching still works after 10 repeated dispense cycles.

CONSTRAINTS
- No .ioc changes. No physical actuator code.
- OSAL-safe: ms_osal.h only.
- No face embeddings in UART logs even in DEBUG mode.
```
