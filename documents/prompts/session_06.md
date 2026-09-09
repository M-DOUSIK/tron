# Session 06 — Local SD Card Logging

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Implement strict, 100% offline event and biometric-data logging to the on-board
microSD card.

BACKGROUND AND CONTEXT
Building on tron/session_05/ (interactive touch GUI with mascot states working). Per
COMPLIANCE_PRIVACY_POSTURE.md, MedSight has zero cloud connectivity by design — every
event, dispensing record, and future face-recognition data point must be written
locally and only locally.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ (new sd_logger.c/.h)
- COMPLIANCE_PRIVACY_POSTURE.md (data-handling rules to follow)
- tron/session_05/ as the base

REQUIRED INPUTS
- A microSD card inserted in the DK board's slot (your task).

EXPECTED OUTPUTS
- A logging module that appends timestamped text events to the SD card, plus a binary
  write function capable of storing arbitrary-length binary blobs (this session tests it
  with a dummy array standing in for future face-recognition data — no real AI exists
  yet).

CONSTRAINTS
- No networking hardware or cloud APIs — SDMMC/FATFS only.
- Never write biometric-shaped payloads to the UART debug channel introduced in
  Session 02 — SD card only.
- This session does not yet have real face data; use an obviously-dummy placeholder
  array and label it as such in code/comments.

CODING STANDARDS
- Append-only semantics for the text event log — don't overwrite prior entries.
- Clear separation between the text-event API and the binary-blob API.

FOLDER STRUCTURE TO FOLLOW
- FSBL/Src/sd_logger.c
- FSBL/Inc/sd_logger.h

FILES TO CREATE
- FSBL/Src/sd_logger.c
- FSBL/Inc/sd_logger.h

FILES TO MODIFY
- FSBL/Src/main.c (log a "System Boot" event and one dummy binary write at startup, for
  test purposes)

DOCUMENTATION TO UPDATE
- docs/milestones/session_06_notes.md: SD card file/folder layout used, FATFS
  configuration details.
- Cross-reference COMPLIANCE_PRIVACY_POSTURE.md §4 once the actual storage format is
  decided (embeddings vs. raw arrays will be finalized properly in Session 08C — note
  that this session's format is a placeholder).

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly.
- (Manual) Remove the SD card after a boot cycle, inspect on a PC, confirm both the text
  log entry and the dummy binary file are present and correctly formed.

COMPLETION CHECKLIST
- [ ] SDMMC peripheral and FATFS middleware initialized
- [ ] Append-only text event logging function implemented
- [ ] Binary blob write function implemented
- [ ] "System Boot" event and dummy binary write happen at startup
- [ ] session_06_notes.md written

COMMON PITFALLS
- FATFS mount failures if the card isn't formatted as expected (FAT32) — document the
  required format in the session notes.
- Forgetting to properly close/flush files before power-loss scenarios — acceptable to
  defer robust power-loss handling to Session 12, but note it as a known gap.

DEFINITION OF DONE
SD card, when read on a PC after a test run, shows a correctly formatted "System Boot"
text entry and a correctly-sized dummy binary file.

SELF-REVIEW BEFORE DECLARING COMPLETE
Grep the diff for any accidental UART logging of the binary payload — confirm it only
goes to SD.
```

## Expected Deliverables
`sd_logger.c/.h`, session notes documenting the SD file layout.

## Manual Verification Steps
1. Flash, power on, let it boot.
2. Remove the SD card, inspect on a PC.
3. Confirm the text log entry and dummy binary file are present and well-formed.

## Acceptance Criteria
Correct, readable log output on the SD card after a normal boot cycle.

## Next Prompt
Copy to `tron/session_06/`, proceed to `session_07.md`.
