# Session 15 — Carer Mode, Scheduled Dosing, and the Submission

## What this session is for

Sessions 01–14 built the thing. This session asks two questions that nobody has
asked since March:

1. **Does what we built match what we promised?** The Program Plan submitted
   for round 1 (`scratch/Program Plan 54916.pdf`) is what won the board and the
   place in round 2. Judges have read it. Several things in it were never
   built, and several things that were built are not in it. Some of those
   divergences are improvements worth stating proudly; at least one is a
   substitution that has to be explained rather than glossed over.
2. **What is the highest-value thing left to build?** That question has been
   answered: **carer mode and scheduled dosing** (Part B). It closes the
   Program Plan's second-biggest gap — "Schedule Validation" was a named core
   function and nothing in the device has ever known the time of day — and it
   is what turns MedSight from a dispenser into an *adherence* device.

This session is therefore both a build session and a reconciliation session.
Part A is the analysis, Part B is the feature, Parts C and D are the
submission. It may propose Sessions 16+; it should only do so with a
clear-eyed view of the calendar.

### What was ruled out, and why — decided before this session

**A second NPU model is out of budget on this board, alongside this project.**
Both pill classification and action recognition were considered and rejected on
measured numbers, not preference:

| Constraint | Measured at end of Session 12 | Consequence |
|---|---|---|
| Debug-build ROM | 443 KB of 511 KB — **85% full, 78 KB free** | The two existing models' generated forward-pass code is the bulk of a 305 KB `.text`. A third does not fit in 78 KB. |
| Free AXISRAM2 | ~380 KB, minus the kernel heap → **~350 KB usable** | Action recognition is temporal: even an 8-frame ring at 128×128×3 is 393 KB, over budget before the model itself. |
| NPU activation pools | `fd` + `faceid` already sit at `0x34200000`, overlapping the framebuffer | A third model needs its own pool, and Session 13 claims `GUI_BUFFER_ADDRESS` for the second framebuffer that fixes the display corruption. **Those two wants compete directly.** |
| Remaining sessions | Session 14's dispenser task and driver; this session's carer mode, RTC, password UI and schedule storage | Consume the headroom before a third model starts. |

**Pill classification is additionally redundant** given where the project ended
up: the hopper holds one medication type, so the device already knows what it is
dispensing. **Action recognition is additionally weaker than what replaced it**:
with Session 14's IR counter, a pill physically left the hopper and was counted,
for a face-verified patient, inside their scheduled window. That is stronger
evidence of adherence than a hand-to-mouth gesture classifier, and it is
measured rather than inferred.

Record this reasoning in the reconciliation document — "we measured and it does
not fit, and here is why what we built is better" is a strong answer to a judge
asking about the missing classifier. A vague one is not.

---

## How to Start This Session

1. **READ `scratch/Program Plan 54916.pdf` IN FULL.** It is four pages. It is
   the single most important input to this session and the only document in the
   project the judges have already seen.
2. **READ ALL DOCUMENTATION** in `MedSight_Docs/`, and
   `milestones/session_11_notes.md` through `session_14_notes.md`.
3. **READ THE CONTEST RULES AGAIN**, in particular rule 1.4's evaluation
   criteria for the RTOS Application category — real-time performance, power
   saving, small memory footprint, and for the TRON × AI theme, "the high
   degree of relevance to µT-Kernel 3.0". Session 12 was built around those
   words; check the finished system against them honestly.

---

## Part A — Program Plan reconciliation (do this first)

Produce `MedSight_Docs/PROGRAM_PLAN_RECONCILIATION.md`: a clause-by-clause
comparison of what was promised against what exists, with a decision for each
gap. **Be honest in both directions** — a submission that quietly drops a
promised feature reads far worse than one that says "we replaced X with Y, here
is why Y is better."

The known divergences, from a first reading. **Verify each against the actual
code before writing anything down** — do not trust this list:

| Program Plan said | Reality | Bearing |
|---|---|---|
| **"On-device CNN-based pill/packet visual classification"** — the core function. Camera looks at the medication and identifies it. | **Never built.** Replaced by face recognition, which verifies *who is taking it*, not *what it is*. | **The biggest divergence, and it must be addressed head-on.** The reasoning is real and already recorded in `MASTER_PROJECT_PLAN.md` §8: under a hopper design, each hopper holds one known medicine by construction, so classifying it re-confirms what the mechanism guarantees. But note the plan's device had **no** hopper — the user held the pill up to the camera — so that argument does not fully carry. **Decided: explain the substitution, do not build it** — see the memory analysis above. The honest and strong framing is that the delivered system verifies the *patient* and physically *counts* the pills, which together prove more about adherence than identifying a pill the single-medication hopper already knows. |
| **"Schedule validation — compares identified medicine against the patient's stored plan"**, time-match logic, an alarm-triggered capture | **Never built.** There is no schedule, no RTC use, no time-of-day logic. `schedule_time_source.c` was designed in `SOFTWARE_ARCHITECTURE.md` and never written. Dispensing is entirely user-initiated. | Significant. This was a named core function and it is the one that makes the device an *adherence* product rather than a dispenser. Cheap to build (Part B). |
| **"Audio and/or visual alert feedback"**, a buzzer | **Visual only.** No buzzer, no audio. The board has a SAI codec and a MEMS mic that have never been initialised. | Modest, and **decided: the claim was dropped from all documents in Session 12 rather than built.** "and/or" gives cover. Say plainly that alerts are on-screen, that the SAI codec is available and unused, and that audio is recorded as deferred work in Part D. |
| **"Caregiver notification flag stored in non-volatile memory for later review"** | **Delivered, differently** — a full append-only SD event log, which is strictly more than a flag. | Favourable. Say so. |
| **Four tasks: Camera / Inference / Validation / Alert** | **Five, decomposed differently**: `cam_isp`, `ui`, `ai`, `logger`, `heartbeat`, with a written rate-monotonic derivation. | Favourable — the delivered design is better reasoned than the sketch. `session_12_notes.md` Part A3 is the evidence. |
| **"Extensibility: future work could include multi-patient support"** | **Delivered** — a 10-patient gallery with face-based identification. | Favourable: a stated *future* enhancement was shipped. |
| **"Extensibility: Bluetooth caregiver notifications"** | Not built, and deliberately never will be — the zero-network stance is a documented design principle. | Fine. State it as a deliberate choice, not an omission. |
| **~3,300 lines estimated** | Substantially exceeded. | Favourable, if framed as scope delivered rather than lines typed. |
| **Team of 4, with named roles** | Solo for Sessions 01–13; a hardware teammate joined for Session 14. | State the truth. Worth noting that the Program Plan's four roles — RTOS integration, AI/vision, firmware/drivers, UI/testing — were all genuinely covered, just by fewer people. |
| **Physical dispensing** | **Not in the Program Plan at all** — the plan's device was camera-only verification. Sessions 14+ build a real turntable dispenser. | Favourable, and worth being explicit: this is scope *added* beyond what was promised, not restored. |

For each row, the reconciliation document must record: what was promised, what
exists, **why** it differs, and whether the gap is being closed (Part B), or
accepted and explained (Part C).

---

## Part B — Carer mode and scheduled dosing

This is the build. Four pieces, in dependency order.

### B1. A real time source — `schedule_time_source.c`

`SOFTWARE_ARCHITECTURE.md` has specified this module since Session 10 and it
was never written, because nothing needed the time. Write it now, to the design
already documented: **one clearly isolated point of substitution**, exactly
like the OSAL, so the demo's fast countdown timer and a real wall clock are
interchangeable without `state_machine.c` knowing which is behind it.

- **Back it with the STM32N6's internal RTC.** It runs from the board's LSE/LSI
  and keeps time across a reset, which is the whole point — a schedule that
  forgets itself at every power cycle is not a schedule.
- **Keep the fast-timer mode** (`MASTER_PROJECT_PLAN.md` §6 already documents
  this simplification): a build switch that compresses a day into minutes so a
  full dosing cycle is demonstrable on camera. **Both modes must go through the
  same interface** — the substitution point is the deliverable, not the timer.
- **The RTC is new peripheral bring-up.** Manual HAL only, no `.ioc`, and read
  `ENGINEERING_LESSONS.md`'s VddIO finding first — the RTC's backup domain has
  its own power and write-protection rules, and getting them wrong looks
  exactly like a dead peripheral.

### B2. Carer mode — a gated configuration area

A hidden, password-gated mode reached from the home screen, for the person
setting the device up rather than the patient using it.

**Entry gesture.** Something a patient will not find by accident but a carer
can be told in one sentence — the phone-developer-settings pattern: several
deliberate taps on a specific, non-obvious element (the title bar is the
natural target), within a time window, then a password prompt. Pick the counts
and the window, and write down why.

**Password handling — read this before implementing.** The device has no secure
element, so be clear-eyed about what this is and is not:

- **Do not store the password in plaintext**, in flash or on the card. Store a
  hash (a simple one is fine here; the threat model is a curious patient, not
  an attacker with the card in a lab) and compare hashes.
- **Ship a build-time default**, so a fresh device is usable, and allow it to
  be changed from inside carer mode with the new value persisted to the SD
  card. If the card holds no password, fall back to the default.
- **Compare in constant time** if it is free to do so, and rate-limit attempts
  — after N failures, refuse for a period. Both are cheap.
- **Say plainly in `COMPLIANCE_PRIVACY_POSTURE.md` §6 what this protects
  against**: casual access, not a determined attacker with physical possession
  of an unencrypted SD card. That honesty is worth more than the feature.

**What carer mode contains:**

1. **Set the clock** — date and time, written to the RTC. Needed at least once
   per device; a scheduled dose is meaningless without it.
2. **Per-patient dose schedule** — the times of day this patient takes their
   medication. Registration currently collects a name, a face and a dose size;
   the carer adds *when*. Keep the data model minimal: a small fixed array of
   times per patient is enough and matches the one-medication-type hopper.
3. **Set the dose size** — currently collected during registration by the
   patient themselves, which is the wrong person to be deciding it.
4. **Review the log** — the missed and confirmed doses for a patient, read back
   from `events.log`. This is what makes the audit trail useful on the device
   rather than only on a PC, and it is the strongest demo moment in carer mode.
5. **Delete a patient** — the delete function that
   `COMPLIANCE_PRIVACY_POSTURE.md` §5 records as deliberately never built,
   because the docs described it with no authentication. Behind a password it
   becomes appropriate. Removing a patient must remove **both** the name and
   the embedding together; a partial delete defeats the point. Update
   `COMPLIANCE_PRIVACY_POSTURE.md` §5 when this lands.

**Schedule storage.** The schedule belongs with the patient record, so
`PatientRecord` grows and `patients.dat`'s format version increments from 2 to
3. Session 12 built the versioned header for exactly this — a v2 card will now
be reported and rejected cleanly instead of silently loading as empty. Say so
in the release notes: carers re-register once.

### B2a. Registration must be authorised — close the open-enrolment hole

**This is the most important security change in the session, and it is a
genuine hole in the current build, not a hardening nicety.**

Right now `STATE_INSTRUCT_REGISTER` is reachable by anyone from the home
screen. A stranger can walk up to the device, enrol their own face, set their
own dose size, and then use DISPENSE to have the machine hand them the
medication — and the audit log will faithfully record it as a legitimate,
face-matched dispense to a registered patient. Face recognition is doing
exactly what it was built to do; the problem is that the gallery it matches
against will accept anyone who asks.

Every other control in the device rests on the gallery being trustworthy. If
enrolment is open, identification is theatre.

**The fix: registration happens beside the carer, never alone.**

Gate the *entry* to registration behind the same password prompt as carer
mode. Pressing REGISTER PATIENT on the home screen prompts for the carer
password first; the existing registration flow runs only after it validates,
and returns to the home screen on cancel or failure.

**Gate at the start, not at the end.** The obvious alternative — let the
patient register and ask for the password to confirm at the end — is worse in
three ways: it wastes the user's time before rejecting them, it means a face
capture and a name have already been taken from someone who was never
authorised (which `COMPLIANCE_PRIVACY_POSTURE.md` should not have to
apologise for), and a mid-flow abort has to unwind partially-written state.
Ask first, then everything downstream is already authorised. If you disagree
after implementing it, say so in the notes with the reason.

**Implementation notes:**

- Reuse **one** password-prompt screen and **one** validation routine for both
  carer-mode entry and this. Two copies of a password check is two places to
  get the comparison, the rate-limiting, or the hash wrong. Build the prompt
  as a reusable component in B2 and call it from both.
- The rate limiting from B2 applies here too, and matters more: this screen is
  reachable from the home screen with no hidden gesture in front of it.
- The registration entry point is now the only place a patient-facing button
  leads to a password prompt. Word it for the patient who pressed it by
  mistake: something closer to "A carer needs to set this up for you" than
  "Access denied". Getting the tone right here is part of the design, not
  decoration.
- Once B2's dose-size and schedule editing exist, reconsider whether the
  registration flow should still collect the dose size at all, or just name +
  face, with everything clinical set in carer mode. Decide it deliberately and
  record the decision.

**Update the docs when this lands:**

- `COMPLIANCE_PRIVACY_POSTURE.md` — the current text describes the gallery as
  trustworthy without saying what protects enrolment. State the old behaviour
  plainly as a limitation that was found and fixed; a posture document that
  hides a fixed hole is worth less than one that shows the fix.
- `UI_SCREEN_INVENTORY.md` — the password prompt is a new screen reachable
  from two entry points.
- `SOFTWARE_ARCHITECTURE.md` §7 — the state machine gains a gated transition.

**Where this came from.** The user raised it unprompted while closing out
Session 12: *"registration is not authenticated right, anybody can just simply
register"*. It had not been caught by any prior session's review, including
the compliance-posture pass that was specifically looking for this class of
problem. Worth remembering that the person who uses the device found it and
the documents did not.

### B3. The scheduled-dose flow

With a clock and a schedule, the device can finally act on time:

- **A dose window opens** → the home screen shows a clear, unmissable reminder
  naming the patient, and the mascot goes to `MASCOT_ACTIVE`. This is the local
  alert `MASTER_PROJECT_PLAN.md` §7 describes. **On-screen only** — there is no
  buzzer and no audio in this project — that claim was dropped in Session 12
  rather than built; see Part D.
- **The patient turns up and taps Dispense** inside their window → the normal
  flow runs, and the log records the dose *against its scheduled window*.
- **The window closes with no dispense** → a missed-dose event is logged and
  the mascot goes to `MASCOT_ERROR`. This is the single most valuable line in
  the whole audit trail, and the device has never been able to write it.

**Contest fit, and do it the idiomatic way.** A dose window is a genuine
periodic real-time deadline — precisely what rule 1.4's "real-time performance"
asks about. µT-Kernel's **alarm handler** (`tk_cre_alm`) is the right primitive:
a one-shot handler armed for the next dose time, rather than a task polling the
clock. Extend the OSAL with it, following exactly the pattern Session 12 used
for event flags, and keep every `tk_*` call inside `ms_osal.c`. Note the
constraint that shapes the design: an alarm handler runs in handler context, so
it may not `printf` and may not block — it should set an event flag and let a
task do the work.

### B4. Investigate the memory budget — is the FSBL 512 KB limit real?

**This is an investigation with a written answer, not necessarily a change.**
It is worth doing because the conclusion that ruled out a second AI model rests
on numbers that may be an inherited linker-script choice rather than a hardware
limit.

The facts to start from:

- The STM32N657 has roughly **4.2 MB of contiguous on-chip SRAM**, and this
  project's linker script (`STM32N657X0HXQ_AXISRAM2_fsbl.ld`) claims only two
  small windows of it: `ROM` at `0x34180400` for **511 KB** (`.text` +
  `.rodata`) and `RAM` at `0x34000400` for **1023 KB** (`.data`/`.bss`/heap/
  stack). That is ~1.5 MB of 4.2 MB.
- Those numbers came from the ST `DCMIPP_ContinuousMode` example this
  repository was founded on in Session 03, not from any analysis of what
  MedSight needs. **511 KB is a choice, not a ceiling.**
- **Why FSBL at all:** the STM32N6 has no internal user flash. Its ROM
  bootloader loads a First Stage Boot Loader image into SRAM and runs it, and
  a project may optionally chain to a second "Appli" stage. The ST example ran
  entirely in FSBL and Session 03 kept that ("Maintain the FSBL-only
  architecture"). Nothing about FSBL itself caps memory — it is simply the
  stage this application runs in.
- The rest of the SRAM is **not free space**, though: the camera framebuffer
  (`0x34200000`), the NPU activation pools, and Session 13's second
  framebuffer (`GUI_BUFFER_ADDRESS`) all live at absolute addresses above the
  linker regions. `session_08B_notes.md` Addenda 2 and 4 document three more
  fixed regions in external memory (`0x70380000`, `0x72000000`, `0x90000000`).

**What to produce:** a memory map of what is actually claimed versus what
exists, from the reference manual and the map file — not from these notes.
Then answer, in writing:

1. How much of the 4.2 MB is genuinely unclaimed?
2. Can `ROM` and `RAM` be grown without colliding with the framebuffers or the
   NPU pools? Note that the NPU's data masters cannot reach every bank —
   `AI_LESSONS.md` records a hard fault caused by exactly that, which is why
   the weights live in external OSPI flash.
3. **If** the answer is that a third model would now fit, say so with numbers —
   and then say what it would cost in *time*, which is the constraint that
   actually matters this close to the deadline. Do not start building a model
   on the strength of a memory answer alone.

Treat a negative result as a real result. "We measured the budget and a third
model does not fit, here is the map" is a good thing to be able to say.

### B5. Measure the current draw, if the instrumentation cooperates

Session 12 measured ~89.6% CPU idle but nobody has measured actual **current**.
`scratch/x-cube-n6-ai-power-measurement` has been sitting in this repo unused
since the beginning. Rule 1.4 names power saving explicitly, and "42 mA idle
against 180 mA during inference" is a categorically stronger claim than a
percentage. Cheap if it works; **abandon it quickly if it does not** — it is a
nice-to-have, not this session's job.

---

## Part C — Make the submission argue for itself

The engineering is only half of a contest entry. Assume a judge reads the
Program Plan, then the README, then runs the demo video.

1. **Lead with the numbers.** ~89.6% CPU idle; NPU inference latency
   (Session 13 measures it); pill-count accuracy over 10 dispenses
   (Session 14); memory footprint from `arm-none-eabi-size`; six modified
   µT-Kernel files out of ~230, none of them a system call. Rule 1.4 asks for
   real-time performance, power saving and small footprint — answer all three
   with measurements.
2. **Make the µT-Kernel relevance explicit and concrete.** Not "we used an
   RTOS" but: inference dispatched through a `tk_wai_flg` event flag; a
   rate-monotonic priority derivation; `low_pow()` implemented as a real WFI
   with measured effect; deferred object creation bridging the kernel's
   "create only once running" rule. `session_12_notes.md` has all of it —
   including the idioms deliberately *rejected*, which is itself a strong
   signal to an expert reader.
3. **Tell the debugging stories.** The D-cache vector-table bug (Session 11
   Addendum 7), the SysTick-starves-PendSV instrumentation trap (Addendum 8),
   the BASEPRI/WFI hang (Session 12 Addendum 1), the six-session-old
   `disk_ioctl()` bug (Addendum 2). These are the strongest evidence in the
   whole project that the team actually understands the platform rather than
   assembling examples. Most entries will not have them; almost none will have
   written them down as they happened.
4. **Be straight about the honest limitations.** No regulatory claim — see
   `COMPLIANCE_PRIVACY_POSTURE.md`, and do not let the submission drift into
   overclaiming. The no-auth delete option. The face-match threshold's
   measured behaviour. One hopper, not six. Judges see through inflation, and
   `COMPLIANCE_PRIVACY_POSTURE.md` §3 already argues that honesty is the
   stronger position.
5. **Update `MASTER_PROJECT_PLAN.md`** with the final session count and an
   accurate history. The changelog is the project's own record of how its scope
   moved and why; keep it truthful, including about the reversals.

---

## Part D — The roadmap beyond 15

**Do not create a separate future-work document.** Put this in the README's
"Known limitations and future work" section (Session 13 Part E creates it) and
in this session's own notes. One place, where a judge will actually read it —
the Program Plan's own "Extensibility" section was one of its strengths, and
being able to say precisely what comes next is a sign of a project that is
understood rather than merely finished.

For each item: what, why it matters, what it would cost, and what it depends
on. This is what the project has explicitly deferred, roughly in the order it
would be worth doing:

| Item | Why it was deferred | What it needs first |
|---|---|---|
| **Audio alerts** (SAI codec + speaker, both unused) | Claim dropped in Session 12 rather than built | Nothing — it is genuinely available, just unbuilt |
| **Multi-hopper** — the 6–8 hopper architecture in `MECHANICAL_DESIGN.md` | Session 14 builds one; the data model and `dispenser_dispense()` were kept extensible | Mechanical build; a `hopper_id` in the patient record |
| **Encrypted SD storage** | Prototype scope; `COMPLIANCE_PRIVACY_POSTURE.md` §6 lists it as a known limitation | A key-storage answer on a part with no secure element |
| **Pill classification** | Measured out of ROM/RAM budget; also redundant with a single-medication hopper | Either a bigger memory budget or dropping another model |
| **Action recognition** | Same budget analysis; weaker than IR-counted physical dispensing | A temporal model that fits in ~350 KB, which is the hard part |
| **Caregiver notifications off-device** | Zero-network is a permanent design principle, not an omission | A deliberate connectivity and privacy decision, which would change what this project *is* |

**Only propose Sessions 16+ if the calendar genuinely allows.** Every prior
changelog entry that invented optional stretch sessions — 12B, 15, 16 in the
v6/v7 numbering — ended with them being dropped. Do not add to that list for
the sake of a plan that looks ambitious.

---

## Definition of Done

- [ ] `PROGRAM_PLAN_RECONCILIATION.md` written, every row verified against real
      code, each gap either closed or explained.
- [ ] **B4 answered in writing**: the real memory budget, with a map, and a
      numbers-backed statement on whether a third NPU model could fit.
- [ ] **Part B built and hardware-tested**: `schedule_time_source.c` behind a
      swappable interface with the RTC live and the fast-timer demo mode
      working; carer mode reachable only via the hidden gesture + password;
      clock, schedule, dose size, log review and delete-patient all functional;
      a scheduled dose window that fires, and a missed window that is logged.
- [ ] **Registration is authorised (B2a)**: REGISTER PATIENT prompts for the
      carer password before any face is captured, sharing one prompt screen and
      one validation routine with carer-mode entry. Verified on hardware that
      an unauthorised user cannot enrol.
- [ ] `patients.dat` format bumped to v3 with the schedule; a v2 card is
      rejected with a clear message rather than silently ignored.
- [ ] Submission materials argue the case: measured numbers, concrete µT-Kernel
      relevance, the debugging stories, honest limitations.
- [ ] `MASTER_PROJECT_PLAN.md` reflects the true final scope and history.
- [ ] Deferred work recorded in the README and this session's notes — no
      separate future-work document.
- [ ] Build 100% clean, both configurations; full flow works end to end on
      hardware; `milestones/session_15_notes.md` written.

---

## What This Session Does NOT Do

- **No networking.** Still permanent. If a "caregiver notification" idea
  resurfaces here, it is local (on-screen + SD log) or it is future work.
- **No second AI model.** Pill classification and action recognition are both
  ruled out on measured memory grounds — see the table above. Do not reopen
  this without re-measuring.
- **No audio.** The buzzer claim was dropped from the docs in Session 12 rather
  than implemented. If a scheduled alert really needs sound, that is Part D
  roadmap material, not a late addition here.
- **The carer-mode hook already exists in the UI.** Session 13 drew the
  pill-count screen as four hopper slots with only slot A live; B, C and D
  are greyed with a "SOON" label and a caption pointing at the carer app.
  If this session builds any part of carer mode, that screen needs colours
  and touch targets turned on (`HOPPER_LIVE` in `registration_ui.c`), not a
  redesign — and the per-patient record needs a hopper field before it can
  mean anything.
- **No mascot animation work — in this session.** Not a standing rule: the
  mascot has two live states (`MASCOT_IDLE`, `MASCOT_ERROR`) as of Session
  13, and `MASCOT_ACTIVE`/`MASCOT_SUCCESS` are unbuilt rather than
  forbidden. Building either needs new artwork and is out of scope here.
  See `MASCOT_UI_DESIGN.md` §4.
- **No overclaiming.** No regulatory language, no "PMDA-compliant", no implying
  a feature exists because it is in the Program Plan.
- **No scope the calendar cannot hold.** The honest failure mode of this
  session is starting three things and finishing none.
