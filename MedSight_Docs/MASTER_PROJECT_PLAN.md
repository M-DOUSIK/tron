# MASTER_PROJECT_PLAN.md — MedSight Smart Pill Dispenser

**Target contest:** TRON Programming Contest 2026, RTOS Application category
**Build deadline:** September 30, 2026
**Board:** STM32N6570-DK (Cortex-M55 @ 800MHz + Neural-ART NPU @ 1GHz, ~600 GOPS)
**Developer:** Solo, working with Antigravity as coding agent
**Status note:** The originally filed Program Plan (submitted for the March 31 2026
proposal round) described a camera-only verification device with no physical dispensing.
An intermediate revision of this plan (v1–v4 below) expanded scope to include full
physical multi-hopper dispensing hardware. **That physical-hardware scope was
subsequently cut entirely** — see the Changelog's latest entry and
`prompts/session_10.md`'s "Hardware decision (FINAL): No physical motors/servos/IR
sensors are interfaced." The contest-submitted prototype dispenses via an on-screen
simulation and a manual "✓ I Took It" confirmation, running on µT-Kernel 3.0 for
contest compliance, with an original animated mascot UI and one-shot face
recognition. `MECHANICAL_DESIGN.md` and `RAGNAR_CAD_PROMPT.md` are kept as documented
**design intent** — the mechanism a real product would use, illustrated with 3D
renders for the submission — not as build instructions for this prototype. MedSight
is still conceptually a **shared device serving multiple people on multiple
medications**, not a single-medicine dispenser; that shows up in the patient-profile
data model and registration UI rather than in physical hopper hardware.

---

## 1. What MedSight Is

MedSight is an AI-powered, fully offline smart pill dispenser prototype serving
multiple people and multiple medications from one shared unit. In its current,
contest-submitted form it:
- Simulates dispensing on-screen (an animation, not a physical actuator — see §6 and
  `MECHANICAL_DESIGN.md`, which documents the intended physical mechanism as a
  design/render concept for a future product, not implemented firmware/hardware here).
- Verifies the correct patient is present (one-shot face recognition + small-gallery
  matching, on-device NPU) before showing the simulated dispense — with multiple
  people sharing the device, this is a small face gallery match, not a single
  hardcoded reference face (implemented Session 08B, real enrollment wired Session 09).
- Confirms consumption via a manual "✓ I Took It" button (action/gesture recognition
  was evaluated and dropped before it was ever built — see §8).
- Gives friendly, animated visual feedback via an original mascot character on the
  5" LCD.
- Logs every event (dispense, verification, face data, missed/incorrect dose) to local
  SD card only — zero cloud, zero network stack.
- Runs on µT-Kernel 3.0 in its final, contest-submitted form.

## 2. What MedSight Is Not (scope guardrails)

- **Not** a medically-certified device. Section 7 (Compliance Posture) explicitly frames
  privacy/data-locality choices as *design principles inspired by* Japanese medical-data
  law, not a claim of regulatory approval. Never present it otherwise in docs, UI copy,
  or the contest submission.
- **Not** using any Nintendo/Pokémon or other copyrighted character IP anywhere — UI,
  code comments, asset filenames, or documentation. See `MASCOT_UI_DESIGN.md`.
- **Not** cloud-connected. No Wi-Fi stack, no network peripheral bring-up, ever, in any
  session. If a future session tempts you to add connectivity, that's scope creep —
  reject it.
- **Not** a multi-hopper dispenser. **Superseded in part — read this carefully.**
  Sessions 10-13 were built as a software-only simulation because the physical
  build was not achievable solo before the deadline. That constraint changed
  when a teammate able to design and build the hardware joined, so **Session 14
  builds a real single-hopper stepper turntable with an IR pill counter** (see
  the v11 Changelog entry). What remains out of scope is the *6-8 hopper*
  architecture in `MECHANICAL_DESIGN.md`, which stays documented design intent.
  Note also that physical dispensing was never in the originally submitted
  Program Plan at all — that described a camera-only verification device — so
  Session 14 is scope *added* beyond what was promised, not restored.
  **The no-networking rule above is not affected and never will be.**

## 3. Timeline (Aug 8 → Sep 30, 2026 ≈ 7.5 weeks)

One session ≈ one focused evening/weekend block. **15 session files total, Session 01
through Session 15 — this is the complete, final plan.** (Session 08 split into
08A/08B for risk management — 08C, action recognition, was planned but never run, see
§8. Sessions 14 and 15 were added in v11 when a hardware teammate joined; see the
Changelog.) Roughly 2–3 sessions per week, with slack for the known "boss fight"
sessions (08A/08B AI, 11 µT-Kernel migration, 14 hardware bring-up).

| Week | Sessions | Focus |
|---|---|---|
| 1 | 01, 02 | Bring-up: blink, UART debug |
| 2 | 03, 04 | Camera→LCD pipeline (built on the jpcano LCD reference), mascot idle animation |
| 2–3 | 05 | Interactive touch GUI (GT911, Register/Dispense Medicine buttons, mascot state enum) |
| 3 | 06, 07 | SD logging, OSAL + FreeRTOS |
| 4–5 | 08A, 08B | AI: toolchain proof → face recognition + gallery matching (budget 1.5–2 weeks — hardest session; 08C action-recognition was planned but cut, see §8) |
| 5–6 | 09 | Registration flow (enrollment UI, SD profile storage) — done |
| 6 | 10 | Simulated dispense flow: face gate → on-screen dispense animation → manual "✓ I Took It" confirmation (no physical actuators — see §6) |
| 6–7 | 11 | µT-Kernel 3.0 migration (contest compliance gate) — done |
| 7 | 12 | µT-Kernel-idiomatic integration (event flags, reasoned priorities), power saving (`low_pow`/WFI), system hardening (SD hot-plug, gallery-full/face-retry, pill-count tracking, log review), third-party software inventory — **done, see `milestones/session_12_notes.md`** |
| 7–8 | 13 | UI/UX overhaul: one visual system, the framebuffer glitch, mascot states, contest documentation packaging |
| 8 | 14 | Physical dispensing hardware: 28BYJ-48 stepper turntable + hand-built IR break-beam pill counter, closed-loop count (with the hardware teammate) |
| 8 | 15 | Program Plan reconciliation, highest-value remaining feature, submission materials (last session in the plan) |

**Cut order if time runs short**, most-expendable first: Session 15's optional
feature work (Part B) → Session 14's physical hardware (the simulated dispense
path is deliberately kept working precisely so this cut stays available) →
Session 13's visual polish. **Never** cut the µT-Kernel migration or the
third-party inventory: Session 11 satisfies contest rule 1.1 and Session 12's
`THIRD_PARTY_SOFTWARE.md` satisfies rule 1.3. Those two are the requirements;
everything else is evaluation criteria.

## 4. Execution Workflow (repeats every session)

1. You open the saved project from the previous session's folder
   (`sessions/session_NN/` — older revisions of this document said `tron/`, which
   was never the actual path).
2. You paste that session's prompt (from `/prompts/session_NN.md`) into Antigravity
   unmodified.
3. Antigravity edits/generates code and docs only within that session's stated scope.
4. **You** compile in STM32CubeIDE, flash the STM32N6570-DK, and manually verify against
   that session's checklist. Antigravity never touches hardware.
5. On success: copy the whole working project into a new folder
   `sessions/session_NN/` (don't overwrite the previous one — this is your
   rollback trail). `ENGINEERING_LESSONS.md` documents the exact copy procedure;
   it has bitten this project three separate times.
6. On failure: use the session's "Common pitfalls / rollback strategy" section, fix, or
   discard the session's changes and retry from the last good
   `sessions/session_(NN-1)/`.
7. Move to the next prompt only after that session's Definition of Done is met.

## 5. Non-Negotiable Rules for Every Antigravity Session

- Stay inside the session's stated file scope. Don't let Antigravity "helpfully" refactor
  unrelated modules.
- No cloud/network code, ever. "Notifications" in the prototype are local and
  on-screen only — see §7.
- No copyrighted character assets, ever, including "Pokémon-style" if it resurfaces in
  any future brief — see `MASCOT_UI_DESIGN.md`.
- Follow `ENGINEERING_LESSONS.md` for anything touching a hardware peripheral: always
  configure via the `.ioc` file, never hand-edit generated Makefiles, refresh the
  workspace after manual file additions, trace hardware dependencies via schematic/BSP
  before writing a driver.
- Small, buildable increments — every session must end in code that compiles and a
  physically verifiable result.
- Update the relevant doc (this file's changelog, or the specific architecture doc) after
  each session — don't let docs drift from code.
- If a session's actual implementation deviates from the plan (e.g. a pin conflict forces
  a different GPIO), record it in that session's doc immediately, not "later."

## 6. Prototype vs. Final Scope Differences

Three things are deliberately simplified (or, for dispensing, fully cut) for the
prototype/contest build and must be called out explicitly wherever they matter, so
nobody mistakes the shortcut for the real design:

- **Physical dispensing:** the prototype dispenses via an on-screen animation and a
  manual "✓ I Took It" confirmation button — no physical motors, servos, or IR
  sensors are built or interfaced, at any session, in the contest-submitted build.
  This is a **full cut**, not a scaled-down version: `MECHANICAL_DESIGN.md` documents
  the real, future product's mechanism as design intent (illustrated via
  `RAGNAR_CAD_PROMPT.md`'s 3D renders for the submission), but none of it is part of
  this prototype's firmware or physical build. See §8 and the Changelog for the
  decision history.
- **Time source:** the prototype uses a fast, configurable countdown timer standing in
  for real wall-clock scheduling, so a full dosing cycle can be tested in minutes
  instead of waiting real hours/days. The real RTC-driven daily schedule is the
  intended final behavior — implemented in the state machine as a swappable time
  source (same pattern as the OSAL: one clearly isolated point of substitution), not
  hardcoded to the fast-timer path.
- **Data deletion:** there is **no on-device delete function**, and earlier
  revisions of this plan were wrong to describe one. Deletion is by physical
  control of the SD card — remove it, wipe it, destroy it — which is complete
  and verifiable precisely because the device stores nothing anywhere else. A
  no-authentication delete of biometric data was described in older drafts and
  deliberately never built; see `COMPLIANCE_PRIVACY_POSTURE.md` §5.
- **Dose, not stock:** a patient's `pill_count` is **how many pills they take in
  one sitting** — a fixed property of their prescription. The firmware keeps no
  stock counter, because it has no way to know when a carer refills the hopper;
  Sessions 10-12 briefly carried one (`pills_remaining`) that decremented on
  every dose, which was simply wrong and is gone. Real hopper-level knowledge
  arrives in Session 14, where the IR counter measures pills physically
  dropping instead of assuming a number.

## 7. Notification Architecture (Prototype vs. Future Work)

A medication reminder naturally suggests notifying the patient's phone when a
dose is due. That needs a radio (GSM/SMS, BLE-to-companion-app, or Wi-Fi) which
does not exist in this project's zero-network design, and adding one is a real
scope and privacy decision rather than a firmware detail.

**Prototype behaviour: on-screen alert only.** When Session 15's scheduled dose
window opens, the device shows the reminder on its own display and logs the
event; the patient is assumed to be at the device and to tap Dispense within
their window. A missed window is recorded to the SD card for a carer to review
later, which is the local equivalent of a notification.

**There is no buzzer and no audio.** Earlier revisions of this plan said "LCD +
buzzer"; no audio code was ever written, and the claim was dropped in Session 12
rather than implemented. The board does have an unused SAI codec and speaker
output, so audio remains genuinely possible — `prompts/session_15.md`'s roadmap
section records it as deferred work, and this plan does not claim it.

Phone notification stays explicit future work requiring a connectivity choice —
never something to add via a hidden or "lightweight" networking shortcut.

## 8. AI Model Scope: Two Cuts, Ending at Face Recognition Only

Two successive scope cuts landed here, in order:

1. **Pill classification dropped** (early decision, v3 of this plan): verifying the
   dispensed pill's type via CNN was dropped because — under the physical
   multi-hopper design that existed at the time — each hopper was already one known
   medicine by construction, so classifying it mostly re-confirmed what the mechanism
   already guaranteed, at real NPU/dev cost (see `MECHANICAL_DESIGN.md`, now marked
   design-intent-only). **Action/consumption recognition** was proposed in its
   place: a lightweight model confirming the patient actually took the pill
   (hand-to-mouth gesture over a short frame sequence).
2. **Action recognition also dropped, before it was ever built.** Session 08C, which
   would have implemented it, was never run — `session_08C.md` does not exist. Once
   the physical dispensing hardware was cut entirely (§6, this Changelog), consumption
   confirmation became a simple UI problem: a manual "✓ I Took It" button
   (Session 10) is the final, production confirmation model.

**Net result:** the NPU pipeline runs exactly one model family — one-shot face
recognition + small-gallery matching (Session 08B, real enrollment Session 09) — not
two. `AI_PIPELINE.md` has been updated to match.

## 9. Risk Register

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| NPU toolchain (STM32Cube.AI) has a learning curve / model conversion fails | High | High | Start Session 08A early with a trivial pretrained model first, before the real face model — de-risk the pipeline before the hard model |
| µT-Kernel 3.0 API differs enough from FreeRTOS that OSAL mapping breaks something subtle | Medium | High | This is exactly why Session 07 introduces the OSAL early — isolate the blast radius to one file (`ms_osal.c`); `ENGINEERING_LESSONS.md` links the official `mtk3bsp2_samples` reference for correct API usage |
| Camera/LCD DMA timing conflicts with AI inference DMA to NPU | Medium | Medium | Profile in Session 08A/08B; be ready to drop preview frame rate during inference bursts |
| Drawing directly into the live camera framebuffer (registration/dispense UI screens) races the DCMIPP DMA or the D-Cache, causing visual glitches | Medium | Low | Session 09 hit and fixed two real instances of this — see `milestones/session_09_notes.md`; the established fix pattern (`camera_stop()` before any static UI, `gui_draw_flush()`/`gui_draw_flush_rows()` after every direct draw) applies to any new screen, including Session 10's dispense UI |
| Registration flow (Session 09) or simulated dispense flow (Session 10) scope grows beyond what fits before the deadline | Medium | Medium | Both are fully specified prompts already — resist adding UX polish beyond what's written until Session 13 |
| Running out of time before Sep 30 | Medium | High | Session 13 is the first to cut; Session 11 (µT-Kernel compliance) is the hard deadline gate, protect it |

## 10. Document Index

- `HARDWARE_ARCHITECTURE.md` — board, sensors, power, wiring for what's actually
  built (camera, LCD/touch, SD card); documents the hopper/motor/IR peripherals as
  **not built** for this prototype (design intent only — see §6)
- `MECHANICAL_DESIGN.md` — the physical dispensing mechanism a real product would use
  (Mr Innovative turntable design, duplicated per hopper), BOM, 3D printing, scaling
  rationale — **documented design intent for the submission's 3D renders, not built
  or wired to firmware in this prototype** (see §6)
- `RAGNAR_CAD_PROMPT.md` — text-to-3D prompt generating the enclosure/hopper renders
  that illustrate `MECHANICAL_DESIGN.md`'s design intent for the submission
- `SOFTWARE_ARCHITECTURE.md` — module boundaries, OSAL, folder structure, patient
  profile data model, mascot state enum, dispense-flow state machine, task set and
  priority derivation, power saving, pin map
- `UI_SCREEN_INVENTORY.md` — every screen the device can show, what draws it, and
  the known visual defects; written from the code as Session 13's design input
- `PROGRAM_PLAN_RECONCILIATION.md` — Session 15's clause-by-clause comparison of
  the delivered system against the originally submitted Program Plan (not yet
  written)
- `AI_PIPELINE.md` — face recognition + gallery matching (the only NPU model in the
  final pipeline — action/consumption recognition was evaluated and dropped, see §8),
  STM32Cube.AI workflow
- `MASCOT_UI_DESIGN.md` — original mascot character brief, touch/button layer, IP
  guardrails (explicitly supersedes any "Pokémon-inspired" language in older drafts)
- `COMPLIANCE_PRIVACY_POSTURE.md` — data-locality design principles, personal-data
  handling, explicit disclaimers
- `THIRD_PARTY_SOFTWARE.md` — the contest rule 1.3 inventory: name, rights
  holder, acquisition method and function of every piece of others' software
  this firmware links, plus the µT-Kernel modification table and the explicit
  statement that no `tk_*` API specification was changed (Session 12)
- `ENGINEERING_LESSONS.md` — hard-won STM32CubeIDE/.ioc/Makefile rules from real
  board bring-up; GT911 pin facts; jpcano LCD reference repo; mtk3bsp2_samples
  reference repo; how to drive STM32CubeIDE's own build headlessly
- `prompts/session_01.md` … `session_13.md` — copy-paste-ready Antigravity prompts,
  the complete and final session plan (Session 08 split into 08A/08B; there is no
  08C, 12B, 14, 15, or 16 — those were all dropped, see the Changelog)

## 11. Changelog

- **v11 (this update):** Two changes, one of them a reversal.
  - **Physical dispensing is back, at one hopper.** v8 cut it because it was
    not achievable solo before the deadline. A teammate able to design and
    build the hardware has joined, so the resource assumption behind that cut
    no longer holds. **Session 14** interfaces a 28BYJ-48 stepper-driven
    turntable (exactly the mechanism `MECHANICAL_DESIGN.md` already specified)
    and a hand-built IR break-beam sensor — discrete emitter and receiver LEDs
    with their resistors — that counts each pill as it physically drops — a
    closed loop, so the actuator stops on a real count rather than a timer,
    which is the whole argument `MECHANICAL_DESIGN.md` makes for why a gate
    mechanism was rejected. The 6-8 hopper architecture stays design intent.
    The simulated dispense path is deliberately kept working behind a build
    switch, so the hardware remains cuttable if it misbehaves near the
    deadline. **The no-networking rule is untouched and permanent.** Note for
    the record that dispensing was never in the originally submitted Program
    Plan either — that described a camera-only verification device — so this
    is scope added beyond what was promised, not restored.
  - **Session 15 added:** reconcile the finished system against that original
    Program Plan (`scratch/Program Plan 54916.pdf`), which is the document the
    judges have already read and which promised at least two things that were
    never built — pill/packet classification and schedule validation — then
    spend whatever time remains on the single highest-value item rather than a
    list. Also produces the submission materials that argue the case: the
    measured numbers, the concrete µT-Kernel relevance, and the debugging
    stories from Sessions 11 and 12.
  - **Session count: 13 → 15.** 15 is now the last.
  - **Four documented-but-unbuilt claims resolved** after a full sweep of the
    doc set against the code, rather than left as flags for a future session to
    trip over. **Buzzer/audio** (claimed in three
    documents, zero audio code exists): **claim dropped**. **Mascot
    SUCCESS/ERROR states**: **claim dropped** — the full-screen state changes
    already signal every outcome those animations would, so a parallel channel
    adds work without information. **On-device delete-user-data** (claimed in the plan and
    the compliance posture, no such function exists): **claim dropped**;
    deletion is by physical control of the card, and a password-gated delete
    arrives inside Session 15's carer mode instead. **A second AI model** (pill
    classification or action recognition): **ruled out on measured memory** —
    Debug ROM is 85% full with 78 KB free, usable RAM is ~350 KB against a
    ~393 KB frame ring for a temporal model, and a third activation pool would
    compete directly with Session 13's second framebuffer.
  - Also corrected in the same sweep: `COMPLIANCE_PRIVACY_POSTURE.md` still
    described collecting a patient phone number that was never implemented;
    `SOFTWARE_ARCHITECTURE.md` §2 listed four modules that do not exist
    (`camera_lcd.c`, `interactive_gui.c`, `schedule_time_source.c`,
    `patient_profile.h`) and used a `tron/` root path that was never real;
    `MASCOT_UI_DESIGN.md` attributed the dispense flow to Session 11 instead of
    Session 10.
  - Also in this revision: `pill_count` corrected to mean **the dose** (see §6)
    and the `pills_remaining` stock counter removed; `patients.dat` gained a
    versioned header so a stale or foreign file is reported rather than
    silently ignored; `UI_SCREEN_INVENTORY.md` added as Session 13's input;
    `prompts/session_13.md` rewritten from "final polish" into a real UI/UX
    overhaul, since Session 12 left the functionality working but the interface
    visibly assembled one session at a time.
- **v10:** Session 12 is **done** — µT-Kernel-idiomatic
  integration, power saving, hardening and the third-party inventory. See
  `milestones/session_12_notes.md` for the full record; the parts that change
  this plan's picture of the project:
  - **The AI now runs through the RTOS, not merely beside it.** NPU inference
    moved out of the UI task into its own µT-Kernel task at a deliberately
    *lower* priority, with the request/response handshake carried by a real
    µT-Kernel event flag (`tk_cre_flg`/`tk_set_flg`/`tk_wai_flg`) — the first
    widening of the OSAL surface since Session 07. This is what rule 1.4's
    "high degree of relevance to µT-Kernel 3.0" asks for, and it also buys a
    real behavioural improvement: the UI stays responsive during a capture,
    where before the whole UI task blocked inside the NPU for seconds. Frame-
    buffer ownership between the UI, camera and NPU is now explicit
    (`SOFTWARE_ARCHITECTURE.md` §9) rather than an accident of them being the
    same task.
  - **Two µT-Kernel idioms were evaluated and deliberately NOT adopted** — a
    fixed-size memory pool (`tk_cre_mpf`, because this firmware has no
    fixed-size runtime allocation site at all) and an event flag for
    `STATE_CONFIRM_TAKEN` (because all three of its conditions are produced by
    the task that would wait on them). Both are written up with their evidence.
    `session_12.md`'s instruction to skip a forced idiom rather than perform it
    was followed, and saying so *is* the deliverable.
  - **Power saving exists now.** The vendored BSP's empty `low_pow()` forwards
    to a real `WFI`, with the idle time measured via the DWT cycle counter and
    reported as a percentage every 10 s from task context. This is the one
    change in the session that must be confirmed on silicon rather than argued
    from the ARM manual; a documented one-`#define` fallback is compiled in for
    the case where it is not.
  - **Session 11's last open items are closed** — including the STM32CubeIDE
    clean build for **both** configurations, which turned out not to need a
    human at the GUI at all (`ENGINEERING_LESSONS.md` now records how to drive
    the IDE's own build machinery headlessly). Doing it found a real
    `.cproject` bug latent since Session 08B: the Release configuration was
    missing the ST Edge AI runtime library from its link line, so Release had
    been unbuildable since the AI was introduced.
  - **`THIRD_PARTY_SOFTWARE.md` is written** (rule 1.3), including a µT-Kernel
    modification table derived from a recursive diff against pristine upstream:
    exactly six modified files, and every file implementing a system call
    byte-identical to upstream. It also settles a documentation conflict the
    project had been carrying — the AI models are **CenterFace + MobileFaceNet**
    (`AI_PIPELINE.md` was quoting ST's wrapper name "FaceID"; `session_13.md`'s
    "SCRFD" was simply wrong). Both documents corrected.
  - **Hardware-verified.** Registration, dispense, gallery persistence across a
    power cycle, the new face-retry path and the power measurement all confirmed
    from a real UART capture. Two bugs were found in the process and fixed:
    the plain `WFI` hung the board (BASEPRI masks the very SysTick that would
    wake it — Addendum 1), and `disk_ioctl()` had been returning `RES_NOTRDY`
    for every command since Session 06, which Session 12's new error checking
    was simply the first thing to notice (Addendum 2). **Measured idle: ~89.6%
    of wall-clock time asleep**, waking ~980 times/second — a real number for
    rule 1.4, and the ~10% that is busy is almost entirely the 1 ms camera/ISP
    task.
  - One robustness item is now visible and deliberately left open: the gallery
    match threshold (0.65 cosine, inherited from a reference project that used
    16-bit embeddings where this one uses int8) produced a false rejection of an
    enrolled patient. Session 08B flagged it as needing real measurement;
    Session 12 added the diagnostic that makes the measurement possible but did
    **not** move the threshold blind, since loosening it trades a recoverable
    false rejection for a false acceptance. See `session_12_notes.md`
    Addendum 3.
  - Session count unchanged: **13 sessions, and 13 is still the last.**
- **v9:** Session 11 (µT-Kernel 3.0 migration) is **done and
  hardware-verified** — the full Session 09/10 registration and dispense flows
  run unchanged on µT-Kernel with FreeRTOS entirely removed, confirmed from
  real UART captures (see `milestones/session_11_notes.md`, nine addenda of
  real bugs, several of them genuine ARMv8-M/Cortex-M55 issues the vendored
  BSP could not have hit because its own reference project runs with the CPU
  caches disabled). Following that, the actual TRON Programming Contest 2026
  rules were read directly and checked against the project. The **requirement**
  (rule 1.1 — an application program running on µT-Kernel 3.0) is met. Two
  **evaluation criteria** in rule 1.4 were not being served: "real-time
  performance, power saving, and small memory footprint", and — for the
  "TRON × AI" theme — "the high degree of relevance to µT-Kernel 3.0 will be
  highly evaluated". As shipped after Session 11, the AI runs
  `LL_ATON_OSAL_BARE_METAL` in a polling loop called synchronously from the UI
  task (so it sits *alongside* the RTOS rather than being mediated by it), and
  `low_pow()` in the vendored STM32Cube BSP is an empty function, so the idle
  loop spins the Cortex-M55 at full clock. **Session 12 has therefore been
  rewritten** to absorb the optional "Session 12B" scope that v6 proposed and
  v8 dropped: event flags for the dispense flow's genuine multi-condition
  waits, a reasoned task-priority scheme, an honest evaluation of a fixed-size
  memory pool, and a `WFI`-based `low_pow()` — on top of its original hardening
  scope, plus Session 11's three closeout items and a new
  `THIRD_PARTY_SOFTWARE.md` deliverable that rule 1.3 explicitly requires
  (name, rights holder, acquisition method and function for every piece of
  others' software, plus a rights guarantee — this project links a lot of it).
  The session count is unchanged: **13 sessions, 13 is still the last.**
  Session 12B is not being resurrected as a separate session. `session_12.md`
  carries an explicit instruction to skip any µT-Kernel idiom that has no
  genuine consumer in this codebase rather than force it — a contrived use
  reads worse to an expert judge than not using it.
- **v8 (this update):** Cut the physical multi-hopper dispensing hardware entirely —
  Session 10 onward implements the dispense flow as a software-only simulation (an
  on-screen animation + a manual "✓ I Took It" confirmation button), with no motors,
  servos, or IR sensors built or wired to firmware, per `prompts/session_10.md`'s
  "Hardware decision (FINAL)." `MECHANICAL_DESIGN.md` and `RAGNAR_CAD_PROMPT.md` are
  retained as documented design intent for the submission's 3D renders (Session 13's
  `DESIGN_PROTOTYPE.md` deliverable), not as build instructions — both now carry an
  explicit "not built" banner. This made the action/consumption-recognition model
  (added in v4, see §8) moot too — it existed to solve the same "confirm consumption"
  problem the OK button now solves directly — so action recognition was dropped
  before it was ever built: Session 08C, which would have implemented it, was never
  run, and `session_08C.md` does not exist. The NPU pipeline now runs exactly one
  model (face recognition + gallery matching). Renumbered Sessions 10-13 to match
  what the actual prompt files already contained: 10 = simulated dispense flow
  (previously going to be part of the old Session 11), 11 = µT-Kernel migration
  (previously 12), 12 = hardening (previously 13), 13 = final polish/demo (previously
  14) — **13 is now the last session in the plan.** The optional Session 12B
  (idiomatic µT-Kernel deepening), 15 (boot-demo selector), and 16 (audio output)
  stretch goals from v6/v7 are dropped along with this renumbering — none exist as
  prompt files; treat them only as genuinely optional post-submission ideas if time
  allows, not as part of the 13-session core plan. Updated `SOFTWARE_ARCHITECTURE.md`
  (removed `dispenser.c`, `hopper_id`, and the physical-jam edge case; corrected the
  patient-profile struct to match what Session 08B/09 actually implemented — no
  phone number field, no per-medicine schedule array, since neither was ever built),
  `AI_PIPELINE.md` (single-model pipeline), and `HARDWARE_ARCHITECTURE.md` (hopper/
  motor/IR peripherals marked not-built) to match. This whole realignment was
  triggered by user feedback that several docs still described a hardware build that
  had already been decided against — see this file's and the affected files' own
  text for the full reasoning, not just this changelog line.
- **v7:** Added optional Session 16 for Audio Output. Given the presence of an onboard audio jack, it makes sense to utilize the SAI peripheral and audio codec to give the mascot a voice for alerts and warnings, functioning similarly to a smart assistant.
- **v6:** Added Session 12B — the Session 12 migration deliberately
  used only lowest-common-denominator OSAL primitives (task/queue/mutex/delay) to
  keep that swap mechanical and low-risk, which means the firmware satisfied
  contest compliance without demonstrating any µT-Kernel-specific capability.
  Session 12B is a recommended (not required) follow-on that replaces three
  specific interactions with idiomatic µT-Kernel mechanisms: event flags
  (`tk_wai_flg` with `TWF_ANDW`) for the dispense flow's dual-condition consumption
  check, a fixed-size memory pool (`tk_cre_mpf`) for camera frame buffers, and a
  deliberately-reasoned task priority scheme instead of numbers carried over
  unexamined from FreeRTOS. Clarified for the record: the OSAL (`ms_osal.c`) is not
  itself µT-Kernel — it's a thin naming layer whose internals are rewritten in
  Session 12 to call the real `mtk3_bsp2` kernel API, with FreeRTOS fully removed;
  post-Session-12 the firmware genuinely runs µT-Kernel 3.0, this was a
  clarification, not an architecture change. *(Superseded by v9: this
  Session 12B scope was dropped in v8 and has now been folded into
  Session 12 itself — see the v9 entry for why the contest's own evaluation
  criteria made it worth doing after all.)*
- **v5:** Evaluated TouchGFX for the UI layer and decided against it — X-CUBE-AI and
  TouchGFX cannot coexist in one CubeMX project on the STM32N6570-DK (a confirmed ST
  toolchain limitation), and the main app needs the camera+AI pipeline running
  simultaneously with the UI throughout the dispense flow. **TouchGFX is not used
  anywhere in this project.** This does not reduce GPU usage — the hand-rolled UI in
  Sessions 04/05/09/11 already drives the NeoChrom GPU directly via DMA2D HAL calls
  (TouchGFX would have been a second framework layered on top of the same GPU, not a
  prerequisite for using it). Session 15's optional boot-demo screen reverts to the
  same hand-rolled DMA2D approach as the rest of the app rather than a separate
  toolchain. Confirmed the "convert FreeRTOS to µT-Kernel via an OS wrapper" idea is
  already the project's existing OSAL architecture (Sessions 07/12) — no new work
  needed there. Also completed a full renumbering consistency sweep across all docs
  and session prompts following the v4 registration-flow insertion (several stale
  Session 09/10 references from the mechanical-design stepper-motor swap and the
  registration insertion had been missed and are now fixed).
- **v4:** Inserted Session 09 (Registration Flow — one-shot face enrollment, on-screen
  name/phone entry, medicine/quantity/time selection, SD profile storage), shifting
  all later sessions accordingly (final sequence: 01–14 mandatory, 15 optional).
  Repurposed Session 08B into face recognition + multi-patient gallery matching (moved
  earlier since registration depends on it) and Session 08C into action/consumption
  recognition, replacing pill classification entirely (rationale: each hopper already
  dispenses a known medicine by construction, making pill-type classification largely
  redundant). Rewrote the Session 11 integration prompt around the actual dispense
  flow: camera off by default, face-gallery match gates dispensing, local-only
  intruder alert (not a phone push — flagged as a real architecture conflict with the
  zero-network design, resolved by keeping notifications local for the prototype),
  action recognition + manual confirm both required for consumption logging. Added
  prototype-only fast timer (schedule) and no-auth delete-user-data as explicit,
  clearly-scoped simplifications. Swapped the per-hopper motor from N20 DC+DRV8833 to
  a 28BYJ-48 stepper + ULN2003 (matching the Mr Innovative/UPV reference designs
  actually being duplicated per hopper). Added `ENGINEERING_LESSONS.md` capturing real
  GT911/I2C2/.ioc/Makefile lessons from prior board bring-up, referenced from Sessions
  03, 05, and 12. Added optional Session 15 (boot-time demo/project selector stretch
  goal).
- **v3:** Replaced the single-medicine carousel with a multi-hopper architecture —
  MedSight is a shared device for multiple people on multiple medications, not a
  single-medicine dispenser. Design target 6–8 independently addressable hoppers (one
  medication type each, own motor + own IR sensor); contest-demo build target 2–3
  physically wired hoppers. `dispense_dose()` now takes `(hopper_id, count)`.
- **v2:** Added Session 05 (interactive touch GUI, mascot state enum) per revised
  project brief; generalized the dispenser from carousel-only to an abstracted
  motor-control layer; shifted all subsequent session numbers accordingly; reaffirmed
  original-mascot decision against a "Pokémon-inspired" phrase that resurfaced in the
  revised brief; clarified the touch panel is existing on-board hardware, not a new
  BOM item.
- **v1:** Initial plan superseding the original camera-only Program Plan submission —
  added mechanical dispensing, original mascot UI, staged FreeRTOS→µT-Kernel migration.
