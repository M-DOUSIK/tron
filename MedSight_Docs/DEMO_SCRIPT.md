# DEMO_SCRIPT.md — MedSight Demo Video Shot List

A shot list for a contest demo video. Includes the failure paths on
purpose — an unrecognized face and a missing SD card are not embarrassing
things to hide, they're the hardening work from Session 12 and the exact
thing that separates a real device from a happy-path prototype.

Before filming: **cold boot the board** (power fully removed, then
reapplied) rather than starting from a warm reset or a debugger session —
this is also what proves Session 12's cold-boot display fix
(`milestones/session_12_notes.md` Addendum 9) still holds under the exact
conditions a demo audience would experience.

## Shot list

1. **Boot.** Power the board from cold. Show the screen going from off to
   the home screen with no stale/garbled frame in between (Session 13, Part
   B item 7's clean-boot fix) — hold on the home screen for a couple of
   seconds so the mascot's idle animation is visible.

2. **Register a patient.**
   - Tap **REGISTER PATIENT**.
   - Show the instruction screen (now visually distinct from Dispense —
     teal, "REGISTER PATIENT" — Session 13, Part B item 5).
   - Face the camera, tap **READY**, hold still through the live preview.
   - Show the **"Checking…"** screen appear (Session 13, Part A/B) —
     narrate that this is the moment the NPU is running on-device, with
     nothing sent anywhere over a network.
   - Enter a name on the on-screen keyboard.
   - Set a pill count using the tap-to-add capsule icon.
   - Confirm registration; show the "Registered!" acknowledgement.

3. **Dispense to them.**
   - Tap **DISPENSE PILLS** — show the instruction screen's amber accent,
     distinct from Register's teal.
   - Face the camera again; show **"Checking…"** again, then the match.
   - Show the dispensing screen (progress bar) and, **if Session 14 has
     run**, the physical hopper actually turning and a pill dropping into
     the collection tray — otherwise narrate plainly that this segment is
     the software-simulated dispense (see `DESIGN_PROTOTYPE.md`; don't
     imply hardware that isn't there).
   - Tap **I TOOK IT!**, show the "Thank You!" confirmation.

4. **Show the SD log on a PC.** Pull the microSD card, show
   `patients.dat` and the event log opened on a laptop — the registration
   and dispense events from steps 2–3 should both be present, in plain
   text, with no face-embedding bytes anywhere in the file (Session 13,
   Part D verified this against a real capture, not just code review).

5. **Physical dispense close-up** (only if Session 14 has run). A closer
   shot of the turntable singulating a pill past the break-beam sensor,
   narrating that the motor stops on a *verified* count, not a timed guess.

## Failure paths (demonstrate the hardening, don't cut around it)

6. **Unrecognized face → TRY AGAIN.** Have someone not registered on the
   device attempt DISPENSE PILLS. Show the **"FACE NOT RECOGNISED"**
   screen with **TRY AGAIN** / **CANCEL** — this replaced Session 10's dead
   end (an unconditional return to the home screen with no way to retry).
   Tap TRY AGAIN to show the retry actually works, then CANCEL to return
   home.

7. **SD card removed mid-flow.** With the device idle at the home screen,
   physically remove the microSD card, then reach the home screen again
   (or trigger the check that surfaces it) and show the on-screen warning
   — "no SD card detected... the device still works, but doses are not
   being recorded." Narrate that dispensing intentionally still works
   without a card (a missed dose matters more than a missed log line), but
   the operator is never left unaware that logging has stopped.

## What not to show as if it were built

Per `DESIGN_PROTOTYPE.md`: don't film or narrate the 6–8 hopper
multi-medication architecture, or the enclosure, as though either exists —
neither is physically built. If a shot needs to reference them, show the
design documents/prompt on screen and say so plainly.
