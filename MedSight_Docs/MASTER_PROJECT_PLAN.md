# MASTER_PROJECT_PLAN.md — MedSight Smart Pill Dispenser

**Target contest:** TRON Programming Contest 2026, RTOS Application category
**Build deadline:** September 30, 2026
**Board:** STM32N6570-DK (Cortex-M55 @ 800MHz + Neural-ART NPU @ 1GHz, ~600 GOPS)
**Developer:** Solo, working with Antigravity as coding agent
**Status note:** The originally filed Program Plan (submitted for the March 31 2026
proposal round) described a camera-only verification device with no physical dispensing.
**This plan supersedes that scope.** MedSight now includes full mechanical dispensing
(multi-hopper modules, one medication type per hopper, per-hopper motor + IR
break-beam — see `MECHANICAL_DESIGN.md`), an original animated mascot UI, one-shot
face recognition, and pill classification — all running on µT-Kernel 3.0 for contest
compliance. MedSight is a **shared device serving multiple people on multiple
medications**, not a single-medicine dispenser — see `MECHANICAL_DESIGN.md` §1 for the
capacity target (6–8 hoppers designed for, 2–3 physically built for the contest demo).

---

## 1. What MedSight Is

MedSight is an AI-powered, fully offline smart pill dispenser serving multiple people
and multiple medications from one shared unit. It:
- Physically dispenses pills from independently addressable hopper modules on a
  schedule — one medication type per hopper, per-hopper motor and drop count.
- Confirms each hopper's pills actually dropped, individually counted (IR break-beam
  per hopper).
- Verifies the correct patient is present (one-shot face recognition, on-device NPU) —
  note: with multiple people sharing the device, this is really a small face gallery
  match rather than a single hardcoded reference face; the AI sessions (08A-C) will
  need to account for that (see AI_PIPELINE.md — flagged there as a follow-up, not yet
  fully specified).
- Verifies the correct pill was dispensed (quantized CNN classification, on-device NPU).
- Gives friendly, animated visual/audio feedback via an original mascot character on the
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

## 3. Timeline (Aug 8 → Sep 30, 2026 ≈ 7.5 weeks)

One session ≈ one focused evening/weekend block. 17 session files total (Session 08
split into 08A/08B/08C for risk management; Session 15 is an optional post-submission
stretch, not counted against the deadline), roughly 2–3 per week with slack for
hardware debugging and the known "boss fight" sessions (08A-C AI, 10 mechanical).

| Week | Sessions | Focus |
|---|---|---|
| 1 | 01, 02 | Bring-up: blink, UART debug |
| 2 | 03, 04 | Camera→LCD pipeline (built on the jpcano LCD reference), mascot idle animation |
| 2–3 | 05 | Interactive touch GUI (GT911, Register/Dispense Medicine buttons, mascot state enum) |
| 3 | 06, 07 | SD logging, OSAL + FreeRTOS |
| 4–5 | 08A, 08B, 08C | AI: toolchain proof → face recognition + gallery matching → action recognition (budget 1.5–2 weeks — hardest session) |
| 5–6 | 09 | Registration flow (enrollment UI, SD profile storage) |
| 6 | 10 | Multi-hopper dispenser modules (2-3 physical hoppers, stepper + break-beam) |
| 6–7 | 11 | Full dispense-flow state machine (face gate, action recognition + confirm, fast-timer schedule, delete-data) |
| 7 | 12 | µT-Kernel 3.0 migration (contest compliance gate) |
| 7 | 12B (recommended, not required) | Idiomatic µT-Kernel: event flags, memory pool, real priority design |
| 7 | 13 | Edge-case hardening & regression testing |
| 7–8 | 14 | Demo polish, packaging, submission prep |
| — | 15 (optional) | Boot-time demo/project selector — only after submission, skip if short on time |
| — | 16 (optional) | Audio Output (SAI + Audio Codec) — give the mascot a voice to announce dispensing/warnings |

If Session 08A-C or 12 overruns, cut scope from Session 13/14 first — never ship
without a successful µT-Kernel migration (Session 12 is the actual contest
requirement).

## 4. Execution Workflow (repeats every session)

1. You open the saved project from the previous session's folder (`tron/session_NN/`).
2. You paste that session's prompt (from `/prompts/session_NN.md`) into Antigravity
   unmodified.
3. Antigravity edits/generates code and docs only within that session's stated scope.
4. **You** compile in STM32CubeIDE, flash the STM32N6570-DK, and manually verify against
   that session's checklist. Antigravity never touches hardware.
5. On success: copy the whole working project into a new folder `tron/session_NN/`
   (don't overwrite the previous one — this is your rollback trail).
6. On failure: use the session's "Common pitfalls / rollback strategy" section, fix, or
   discard the session's changes and retry from the last good `tron/session_(NN-1)/`.
7. Move to the next prompt only after that session's Definition of Done is met.

## 5. Non-Negotiable Rules for Every Antigravity Session

- Stay inside the session's stated file scope. Don't let Antigravity "helpfully" refactor
  unrelated modules.
- No cloud/network code, ever. "Notifications" in the prototype are local (LCD +
  buzzer) only — see §9.
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

Two things are deliberately simplified for the prototype/contest build and must be
called out explicitly wherever they matter, so nobody mistakes the shortcut for the
real design:

- **Time source:** the prototype uses a fast, configurable countdown timer standing in
  for real wall-clock scheduling, so a full dosing cycle can be tested in minutes
  instead of waiting real hours/days. The real RTC-driven daily schedule is the
  intended final behavior — implemented in the state machine as a swappable time
  source (same pattern as the OSAL: one clearly isolated point of substitution), not
  hardcoded to the fast-timer path.
- **Data deletion:** the prototype includes an on-device "delete user data" option
  with **no authentication** — appropriate for a bench prototype, not appropriate for
  a real deployed device. If this project continues past the contest, this needs a PIN
  or similar gate before it's used with real patient data.

## 7. Notification Architecture (Prototype vs. Future Work)

The scheduling flow calls for notifying the patient's phone when a dose is due. That
requires a radio (GSM/SMS, BLE-to-companion-app, or Wi-Fi) that doesn't exist in this
project's zero-network design — adding one is a real scope and privacy decision, not a
firmware detail. **Prototype behavior: local alert only** — LCD message + buzzer,
triggered by the state machine when a scheduled dose window opens. Real phone
notification is documented here as explicit future work requiring a connectivity
choice, not something Antigravity should attempt to add via any hidden or "lightweight"
networking shortcut.

## 8. AI Model Change: Action Recognition Replaces Pill Classification

Pill classification (verifying dispensed pill type via CNN) is dropped: each hopper is
already one known medicine by construction (see `MECHANICAL_DESIGN.md`), so classifying
it mostly re-confirms what the mechanism already guarantees, at real NPU/dev cost. In
its place: **action/consumption recognition** — a lightweight model confirming the
patient actually took the pill (hand-to-mouth type action over a short frame sequence),
which is genuine new information the mechanism can't provide on its own. See
`AI_PIPELINE.md` for the updated model list.

## 9. Risk Register

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| NPU toolchain (STM32Cube.AI) has a learning curve / model conversion fails | High | High | Start Session 08A early with a trivial pretrained model first, before your real face/action models — de-risk the pipeline before the hard model |
| Turntable/guide fit or stepper reliability insufficient once a hopper is loaded with its real pills | Medium | Medium | Bench-test each hopper loaded, per its own pill type, before Session 10's full integration; keep spare 28BYJ-48/ULN2003 units on hand |
| Scaling to 6-8 hoppers turns out more GPIO/wiring-constrained than expected | Medium | Low (deferred past contest demo) | 2-3 hoppers is the contest build target; the GPIO-expander note in `HARDWARE_ARCHITECTURE.md` §2 addresses this for later scaling, not for the demo build |
| µT-Kernel 3.0 API differs enough from FreeRTOS that OSAL mapping breaks something subtle | Medium | High | This is exactly why Session 07 introduces the OSAL early — isolate the blast radius to one file (`ms_osal.c`); `ENGINEERING_LESSONS.md` links the official `mtk3bsp2_samples` reference for correct API usage |
| Camera/LCD DMA timing conflicts with AI inference DMA to NPU | Medium | Medium | Profile in Session 08A; be ready to drop preview frame rate during inference bursts |
| Registration flow (Session 09) or dispense flow (Session 11) scope grows beyond what fits before the deadline | Medium | Medium | Both are fully specified prompts already — resist adding UX polish beyond what's written until Session 14 |
| Running out of time before Sep 30 | Medium | High | Sessions 13–14 are the first to cut; Session 12 (µT-Kernel compliance) is the hard deadline gate, protect it |

## 10. Document Index

- `HARDWARE_ARCHITECTURE.md` — board, sensors, actuators, power, wiring (includes
  note: touch panel is on-board, GT911 on I2C2, not added hardware)
- `MECHANICAL_DESIGN.md` — multi-hopper dispensing architecture (Mr Innovative
  turntable design, duplicated per hopper), BOM, 3D printing, scaling rationale
- `SOFTWARE_ARCHITECTURE.md` — module boundaries, OSAL, folder structure, patient
  profile data model, mascot state enum, dispense-flow state machine, pin map
- `AI_PIPELINE.md` — face recognition + gallery matching, action/consumption
  recognition, STM32Cube.AI workflow
- `MASCOT_UI_DESIGN.md` — original mascot character brief, touch/button layer, IP
  guardrails (explicitly supersedes any "Pokémon-inspired" language in older drafts)
- `COMPLIANCE_PRIVACY_POSTURE.md` — data-locality design principles, personal-data
  handling (face, name, phone), explicit disclaimers
- `ENGINEERING_LESSONS.md` — hard-won STM32CubeIDE/.ioc/Makefile rules from real
  board bring-up; GT911 pin facts; jpcano LCD reference repo; mtk3bsp2_samples
  reference repo
- `prompts/session_01.md` … `session_14.md` — copy-paste-ready Antigravity prompts
  (Session 08 split into 08A/08B/08C); `session_12B.md` — recommended (not required)
  deepening of µT-Kernel usage beyond bare compliance; `session_15_optional.md` —
  optional post-submission stretch goal

## 11. Changelog

- **v7 (this update):** Added optional Session 16 for Audio Output. Given the presence of an onboard audio jack, it makes sense to utilize the SAI peripheral and audio codec to give the mascot a voice for alerts and warnings, functioning similarly to a smart assistant.
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
  clarification, not an architecture change.
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
