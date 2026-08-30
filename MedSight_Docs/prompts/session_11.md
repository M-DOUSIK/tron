# Session 11 — Full System Integration (Dispense Flow + State Machine)

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Tie camera/UI, AI (face recognition + action recognition), the multi-hopper dispenser,
registration data, and logging into a single central state machine implementing the
full dispense flow, plus the prototype-only fast-timer schedule and a no-auth
delete-user-data option.

BACKGROUND AND CONTEXT
Building on tron/session_10/ — every subsystem exists individually (camera/UI from
Sessions 03-05, AI from 08A-C, registration from 09, dispenser from 10, logging from
06). This session wires them together into the actual dispense flow: from the main
screen, the user taps "Dispense Medicine"; the camera (off by default to save power)
wakes and identifies the patient against the enrolled face gallery from Session 09; on
a match, the hoppers holding that patient's currently-due medications dispense; action
recognition confirms the pill was actually taken; the patient also taps a manual
confirmation button; the cycle logs and returns to idle. An unrecognized face triggers
an intruder alert instead of dispensing.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ (new state_machine.c/.h)
- SOFTWARE_ARCHITECTURE.md §6 (state flow and edge cases — update it per this session's
  actual dispense-flow design, described below, since it currently only reflects the
  earlier single-mechanism design)
- MASTER_PROJECT_PLAN.md §6-7 (prototype-timer and notification scope this session
  implements)
- tron/session_10/ as the base

REQUIRED INPUTS
- Registration data from Session 09 (enrolled patients with face embeddings, name,
  phone number, and their medicine/quantity/time selections) — this session reads that
  data, it doesn't define it.

EXPECTED OUTPUTS
- state_machine.c/.h implementing the dispense flow:
  1. IDLE: main screen shown (Register / Dispense Medicine buttons, mascot idle),
     camera OFF.
  2. User taps "Dispense Medicine" -> camera powers ON -> face recognition runs
     against the enrolled gallery (Session 08B/09).
  3. No match / unrecognized face -> intruder alert: local LCD+buzzer alert (per
     MASTER_PROJECT_PLAN.md §7 — no phone push in the prototype), SD log entry, camera
     OFF again, return to IDLE. Do NOT dispense.
  4. Match found -> look up that patient's medicines currently due (per the
     prototype's fast-timer schedule, see below) -> call dispense_dose(hopper_id,
     count) for each due hopper in sequence.
  5. After dispensing, run action/consumption recognition (Session 08C) over a short
     frame window to detect the pill-taken gesture.
  6. Require BOTH the action-recognition result AND an explicit on-screen "Confirm"
     button tap before logging the dose as consumed — action recognition alone is not
     sufficient, per the project's belt-and-suspenders requirement.
  7. SUCCESS -> MASCOT_SUCCESS animation, SD log, camera OFF, return to IDLE.
  8. Any failure point (no consumption action detected within a timeout, or the
     confirm button never tapped within a timeout) -> MASCOT_ERROR, SD log noting
     which check failed, camera OFF, return to IDLE.
- **Prototype fast-timer schedule:** implement the "currently due" time check behind a
  swappable time-source interface (e.g. `schedule_get_current_time()`), with the
  prototype implementation being a fast, configurable countdown (so a full day's
  schedule can be tested in minutes) rather than the real RTC. This must be a single,
  clearly isolated substitution point — do not scatter fast-timer-specific logic
  through the state machine, per MASTER_PROJECT_PLAN.md §6.
- **Delete user data:** a no-authentication on-device option (reachable from the main
  screen or a simple settings/debug menu) that removes a selected patient's
  registration record (face embedding, name, phone number, schedule) from the SD card.
  No PIN or auth gate in the prototype — explicitly documented as a prototype-only
  simplification per MASTER_PROJECT_PLAN.md §6, not appropriate for real patient data.
- All inter-module communication via OSAL queues (per Session 07 rules) — state_machine
  is the only module allowed to call into camera/UI, AI, dispenser, registration data,
  and logging modules directly; those modules don't call each other.

CONSTRAINTS
- Do not modify the internals of anime_ui, ai_vision, dispenser, or
  sd_logger beyond exposing whatever clean API calls state_machine needs — this
  session is about orchestration, not reimplementing subsystems.
- Camera must default OFF and only power on for the duration of a dispense-flow face
  check — this is a real power-saving requirement, not just a nice-to-have; verify
  actual camera power state, don't just stop calling its update function while leaving
  it powered.
- No networking, no cloud — still and always zero. The "intruder alert" and "dose due"
  notification are both local-only (LCD + buzzer) in the prototype, per
  MASTER_PROJECT_PLAN.md §7 — do not add any radio/network code to satisfy the
  "notify their phone" requirement from the original flow description; that's flagged
  future work requiring an explicit connectivity decision.

FILES TO CREATE
- FSBL/Src/state_machine.c
- FSBL/Inc/state_machine.h
- FSBL/Src/schedule_time_source.c/.h (the swappable fast-timer/RTC abstraction)

FILES TO MODIFY
- FSBL/Src/main.c (create the state machine task via OSAL, remove any ad-hoc test
  triggers left over from Sessions 08-10)
- FSBL/Src/ui/interactive_gui.c (wire the real Dispense Medicine flow, add a delete-
  user-data entry point, add the Confirm button used in step 6 above)

DOCUMENTATION TO UPDATE
- docs/milestones/session_11_notes.md: fast-timer configuration used, confirmation
  each edge case was tested, confirmation camera power-gating was verified.
- Update SOFTWARE_ARCHITECTURE.md §6 to match this session's actual dispense flow
  (it currently describes the earlier, simpler flow — supersede it here).

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly.
- (Manual) Run a full simulated cycle with the fast timer: dose comes due, tap Dispense
  Medicine, enrolled face recognized, correct hopper(s) dispense, action recognition +
  manual confirm both required, Success logged, camera powers back off.
- (Manual) Run an intruder test: unrecognized face at the Dispense Medicine step ->
  local alert triggers, no dispense occurs, camera powers back off.
- (Manual) Run a missed-consumption test: dispense occurs but no action/confirm within
  timeout -> MASCOT_ERROR, logged correctly.
- (Manual) Run the delete-user-data flow: remove a test patient's record, confirm it's
  gone from the SD card afterward.

COMPLETION CHECKLIST
- [ ] Central state machine implemented matching the dispense flow above
- [ ] Camera confirmed OFF at idle, ON only during an active face check
- [ ] Face-match gating implemented (no match -> alert, not dispense)
- [ ] Per-hopper dispensing driven from the matched patient's due schedule
- [ ] Action recognition AND manual confirm both required before logging consumption
- [ ] Fast-timer schedule implemented behind a swappable time-source interface
- [ ] No-auth delete-user-data option implemented, explicitly documented as
      prototype-only
- [ ] All inter-module communication via OSAL queues, state_machine as sole orchestrator
- [ ] session_11_notes.md written; SOFTWARE_ARCHITECTURE.md §6 updated

COMMON PITFALLS
- Race conditions between the state machine and AI/dispenser tasks if queue depths or
  blocking behavior aren't matched to real timing — test with realistic delays, not
  just fast bench cycles.
- Edge cases silently falling through to the "success" path if a check is written as
  "if not obviously wrong, assume correct" instead of an explicit pass/fail branch.
- Leaving the camera powered between dispense events because it's easier than properly
  gating it — this defeats the actual power-saving requirement; verify with a current
  meter if possible, not just code inspection.
- Baking fast-timer assumptions (e.g. tick units) into state_machine.c directly instead
  of behind schedule_time_source.h — this is exactly the kind of coupling the
  prototype/final split in MASTER_PROJECT_PLAN.md §6 is meant to prevent.

DEFINITION OF DONE
A complete dispense cycle — enrolled-patient success path, intruder-alert path, and
missed-consumption path — all execute correctly via the fast-timer schedule, with
correct camera power-gating and correct SD logging throughout. Delete-user-data works
and is documented as a prototype-only, unauthenticated feature.

SELF-REVIEW BEFORE DECLARING COMPLETE
Re-read the dispense flow above and confirm each numbered step has a corresponding
explicit code branch, not an implicit fallthrough. Confirm no networking code was
introduced anywhere to satisfy the phone-notification requirement.
```

## Expected Deliverables
`state_machine.c/.h`, `schedule_time_source.c/.h`, wired dispense-flow UI, delete-
user-data feature, full end-to-end autonomous operation, session notes.

## Manual Verification Steps
1. Run a full normal cycle via the fast timer: dose due, tap Dispense Medicine,
   correct face match, correct hopper(s) dispense, action recognition + confirm tap
   both satisfied, Success logged, camera powers off afterward.
2. Run an intruder test: unfamiliar face at the Dispense Medicine step, confirm local
   alert (not a phone notification) and no dispense.
3. Run a missed-consumption test: no action/confirm within timeout, confirm
   MASCOT_ERROR and correct logging.
4. Run the delete-user-data flow and confirm the record is actually gone from the SD
   card afterward.
5. Confirm camera power state visibly changes (or is otherwise verifiable) between
   idle and an active face check.

## Acceptance Criteria
All four scenarios (normal, intruder, missed-consumption, delete-data) behave
correctly and are correctly logged; camera power-gating verified.

## Next Prompt
Copy to `tron/session_11/`, proceed to `session_12.md` — the µT-Kernel 3.0 migration,
which is the actual TRON contest compliance requirement.
