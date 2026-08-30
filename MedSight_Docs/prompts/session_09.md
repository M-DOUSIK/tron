# Session 09 — Registration Flow (Enrollment UI + SD Profile Storage)

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Implement the patient registration flow: from the main screen's "Register" button,
capture one photo for one-shot face enrollment, collect name via an
on-screen keyboard, let the user select which configured medicine(s), quantity, and
time from the device's medicine catalog, and persist all of it to the SD card as a
patient profile.

BACKGROUND AND CONTEXT
Building on tron/session_08C/ (AI toolchain, face recognition, and action recognition
all proven working individually). MedSight is a shared device for multiple people —
this session is what actually creates the patient records that later sessions (the
multi-hopper dispenser in Session 10, the full dispense flow in Session 11) depend on.
Per COMPLIANCE_PRIVACY_POSTURE.md, all of this data — face embedding, name, phone
number, schedule — is written only to the local SD card, never transmitted anywhere.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ui/ (extend interactive_gui.c, or add a new registration_ui.c/.h — your
  call based on how large the existing interactive_gui.c has grown)
- FSBL/Src/ai_vision.c/.h (Session 08B's face embedding capture, reused here for
  enrollment)
- FSBL/Src/sd_logger.c/.h (extend with patient-profile read/write, distinct from the
  event-log API already there)
- COMPLIANCE_PRIVACY_POSTURE.md (data handling rules — apply the same "never over
  UART" rule to name/phone number too, not just face data)
- MASCOT_UI_DESIGN.md (main screen button labels: "Register" / "Dispense Medicine")
- tron/session_08C/ as the base

REQUIRED INPUTS
- A configured medicine catalog: for this session, a simple hardcoded or config-file
  list of {hopper_id, medicine_name} pairs is sufficient — this is a data model only;
  the physical hoppers themselves aren't built until Session 10, and that's fine,
  since this session doesn't need to actually dispense anything.

EXPECTED OUTPUTS
- Main screen updated to show "Register" and "Dispense Medicine" as the two primary
  buttons (superseding the earlier placeholder button labels from Session 05).
- Tapping "Register" starts a guided flow:
  1. Camera captures a single photo; Session 08B's face embedding extraction runs on
     it once (one-shot, not a live continuous scan — this should feel like a phone's
     "look at the camera" face-unlock capture, a single deliberate shot, not a
     multi-second live preview requiring the user to hold still).
  2. On-screen keyboard for name entry.
  3. On-screen keyboard (numeric) for phone number entry.
  4. Medicine selection screen listing the configured catalog; user selects one or
     more medicines, and for each selected medicine, a quantity and a time (using the
     prototype's simple time input — an actual RTC-based time picker isn't required
     yet, a simple numeric/scroll input tied to the schedule format Session 11 will
     define is fine; don't over-build this ahead of Session 11).
  5. Confirmation screen summarizing what was entered, then save.
- All of the above persisted as one patient-profile record on the SD card: face
  embedding (binary), name, phone number, and the medicine/quantity/time selections —
  extend sd_logger.c's API with a distinct profile read/write function set (separate
  from the append-only event log already there, since profiles need to be
  updated/deleted, not just appended).
- Multiple patients must be enrollable and distinguishable — store profiles keyed by
  a generated patient ID, not overwritten by the next registration.

CONSTRAINTS
- No copyrighted character assets in any new UI screens — same rule as
  MASCOT_UI_DESIGN.md §2, applies to every new screen this session adds.
- Face embeddings, names, and phone numbers must never be logged over the UART debug
  channel — SD card only, same rule as Session 06/08C for face data, now extended to
  cover name/phone as well (also personal data).
- On-screen keyboard input must not block the camera/mascot rendering pipeline — same
  non-blocking rule carried from Session 04/05.
- This session does not implement the actual dispense flow or hopper hardware — it
  only defines the medicine catalog data model and captures what each patient wants;
  Session 10 (dispenser) and Session 11 (integration) consume this data later.

CODING STANDARDS
- Registration flow as a clear multi-step state machine of its own (capture -> name ->
  phone -> medicine select -> confirm -> save), not a tangle of boolean flags in
  interactive_gui.c.
- Patient profile struct defined once in a shared header so sd_logger.c,
  ai_vision.c (for embeddings), and later state_machine.c (Session 11) all agree on
  its shape.

FOLDER STRUCTURE TO FOLLOW
- FSBL/Src/ui/registration_ui.c (if split out separately)
- FSBL/Inc/ui/registration_ui.h
- FSBL/Inc/patient_profile.h (shared struct definition)

FILES TO CREATE
- FSBL/Src/ui/registration_ui.c
- FSBL/Inc/ui/registration_ui.h
- FSBL/Inc/patient_profile.h

FILES TO MODIFY
- FSBL/Src/ui/interactive_gui.c (main screen buttons, launch registration flow)
- FSBL/Src/ai_vision.c/.h (expose a clean one-shot embedding-capture function if not
  already factored that way from Session 08B)
- FSBL/Src/sd_logger.c/.h (patient-profile read/write/list functions)

DOCUMENTATION TO UPDATE
- docs/milestones/session_09_notes.md: patient-profile SD storage format, medicine
  catalog config format used, on-screen keyboard implementation notes.
- COMPLIANCE_PRIVACY_POSTURE.md §4: extend to explicitly cover name/phone-number
  handling alongside face data, since this session introduces that.

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly.
- (Manual) Complete a full registration for at least two different test patients;
  confirm both profiles exist distinctly on the SD card afterward (pull the card,
  inspect on a PC) with correct face embeddings, names, phone numbers, and
  medicine/quantity/time selections for each.

COMPLETION CHECKLIST
- [ ] Main screen shows Register / Dispense Medicine buttons
- [ ] One-shot face capture integrated into the registration flow
- [ ] On-screen keyboard implemented for name and phone number entry
- [ ] Medicine catalog selection (medicine + quantity + time) implemented
- [ ] Patient profiles persisted to SD, keyed by patient ID, multiple patients
      distinguishable
- [ ] No personal data (face, name, phone) ever logged over UART (grep-verified)
- [ ] session_09_notes.md written; COMPLIANCE_PRIVACY_POSTURE.md §4 extended

COMMON PITFALLS
- Building a full custom virtual keyboard from scratch when a simple, large-button
  on-screen keyboard would serve elderly users (the actual end users of registration,
  likely caretakers per the project's target users) better than a cramped
  phone-style keyboard — keep touch targets large, consistent with the
  elderly-friendly design goal already established for the mascot buttons.
- Treating "one-shot" face capture as literally a single opportunity with no retry —
  allow a retake if the capture clearly failed (e.g. no face detected in the photo),
  rather than silently enrolling a bad embedding.
- Overwriting an existing patient's profile by accident if the same person registers
  twice — decide and document whether that's treated as "update existing" or "create
  duplicate," don't leave it undefined.
- Hardcoding the medicine catalog in a way that Session 10 can't cleanly map to real
  hopper_id values later — keep the {hopper_id, medicine_name} pairing simple and
  centralized so Session 10 just wires real hardware behind IDs that already exist.

DEFINITION OF DONE
At least two distinct patients can be fully registered end-to-end (face, name, phone,
medicine/quantity/time), with correct, distinguishable profiles verifiable on the SD
card afterward, and no personal data ever appearing in UART output.

SELF-REVIEW BEFORE DECLARING COMPLETE
Grep the entire diff for any DEBUG_LOG call near name/phone/embedding variables.
Confirm two separately registered test patients produce two separate, non-overwritten
profile records on the SD card.
```

## Expected Deliverables
`registration_ui.c/.h`, `patient_profile.h`, extended `sd_logger.c` profile API, main
screen with Register/Dispense Medicine buttons, session notes.

## Manual Verification Steps
1. Register a first test patient end-to-end: photo, name, phone, medicine selection.
2. Register a second, different test patient the same way.
3. Pull the SD card, inspect on a PC, confirm both profiles are present, correct, and
   distinct.
4. Confirm no personal data appears in a UART capture taken during either
   registration.

## Acceptance Criteria
Two distinct, correctly-persisted patient profiles after two separate registration
runs; zero personal data over UART.

## Next Prompt
Copy to `tron/session_09/`, proceed to `session_10.md`.
