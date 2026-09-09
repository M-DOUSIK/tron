# Session 13 — UI/UX Overhaul, Visual Polish & Glitch Fixes

## How to Start This Session

Hello! We are starting Session 13 for the MedSight project. Session 12 is
complete and hardware-verified: the whole application runs on µT-Kernel 3.0,
the AI is a proper kernel task, the CPU sleeps ~90% of the time, and the
system hardening and third-party inventory are done.

**This session is a design session.** The functionality works; it does not yet
look like a product. Session 13 rebuilds the UI as one coherent visual system
and fixes the accumulated visual defects — including one that has been deferred
since Session 08B and is the worst thing a judge will see.

**Before writing any code or taking any action**, acquire full context:

1. **READ ALL DOCUMENTATION** in `MedSight_Docs/` — especially
   `MASTER_PROJECT_PLAN.md`, `SOFTWARE_ARCHITECTURE.md`, `MASCOT_UI_DESIGN.md`,
   `ENGINEERING_LESSONS.md`, and `COMPLIANCE_PRIVACY_POSTURE.md`.

2. **READ `UI_SCREEN_INVENTORY.md` IN FULL.** This is your primary input. It
   lists every screen the device can show, what draws it, and eight specific
   known problems with evidence. It was written from the code, not from memory.

3. **READ PAST SESSION PROMPTS** `session_01.md` through `session_12.md`.

4. **READ `milestones/session_12_notes.md` IN FULL**, especially:
   - **Addendum 1** — the idle path's `WFI` masking. Do not touch
     `ms_osal_low_power_idle()`.
   - **Addendum 2** — `disk_ioctl()`'s six-session-old bug, and the lesson
     about unchecked return values.
   - **Part A1's frame-buffer ownership analysis** — this is the constraint
     that shapes the single most valuable fix in this session. Read it before
     designing anything that draws during a face capture.

5. **READ THE WORKING CODE** in `sessions/session_12/FSBL/`:
   `Src/ui/gui_draw.c`, `Src/ui/registration_ui.c`, `Src/ui/anime_ui.c`,
   `Src/ui/state_machine.c`, and `Inc/ui/gui_draw.h`.

---

## Project Rules (non-negotiable, carried forward)

| Rule | Detail |
|---|---|
| No `.ioc` files | Manual HAL only. Never use STM32CubeMX. |
| No networking | No Wi-Fi/Ethernet/BLE stack, ever. |
| OSAL boundary | Application code calls `ms_osal.h` only. **Zero `tk_*` calls outside `ms_osal.c`.** |
| No embeddings over UART | Never `printf`/log raw face-embedding bytes. Names, indices, dose counts and confidence scores are fine. |
| No copyrighted character IP | The mascot is and stays an original design — `MASCOT_UI_DESIGN.md`. This applies to every new asset this session creates. |
| No `printf` on a hot path | Nothing inside an ISR, cyclic handler, the dispatcher or the idle hook. `session_11_notes.md` Addendum 8 explains the cost. |
| Keep Session 12's fixes | The PRIMASK-guarded `WFI`, `disk_ioctl()`'s card-ready wait, the versioned `patients.dat` header, the event-flag AI handshake. None of them are yours to simplify. |
| New session = new folder | Copy `sessions/session_12` → `sessions/session_13`, per `ENGINEERING_LESSONS.md`. The folder-copy procedure and the headless-build command are both documented there. |

---

## Part A — Fix the frame-buffer glitch (highest value in the session)

**The problem.** Both NPU networks' activation scratch is hardcoded by ST's
code generator to `0x34200000` — which is this project's one and only
framebuffer. Every face capture therefore scribbles visible garbage across the
display for 1–4 seconds, at the exact moment the user is being asked to hold
still and look at the camera. It has been known since Session 08B and deferred
by every session since.

**The fix is already half-built.** `GUI_BUFFER_ADDRESS` is defined in
`FSBL/Inc/main.h` (`BUFFER_ADDRESS + FRAME_BUFFER_SIZE`) and is completely
unused. Give the UI its own framebuffer there and point the LTDC layer at
whichever buffer should currently be visible:

- Camera preview → LTDC shows `BUFFER_ADDRESS` (the DCMIPP writes it directly).
- Everything else, including during inference → LTDC shows
  `GUI_BUFFER_ADDRESS`, which the NPU never touches.

`state_machine.c` already calls `switch_ltdc_buffer()` — the mechanism exists,
it is just only ever pointed at one address.

**Verify the memory is real before relying on it.** `GUI_BUFFER_ADDRESS` is
`0x342BB800` and needs 750 KB. Confirm from the linker script and the
STM32N657 memory map that the region is genuinely backed by RAM and is not
claimed by anything else — in particular check it against the NPU activation
pools documented in `milestones/session_08B_notes.md` Addenda 2 and 4
(`0x34200000`, `0x70380000`, `0x72000000`, `0x90000000`). Write down what you
found. **If it does not fit, say so and stop** — do not shrink the framebuffer
or move the NPU's pools to make it fit.

**What this unlocks, and why it is worth doing first.** Session 12 established
that the UI task stays responsive during inference but cannot *draw*, because
the AI task owns the framebuffer. With a second buffer that constraint
disappears: the mascot can keep animating, and Part B's "Checking…" state
becomes possible. Both of those are listed below as if they were separate
items; in practice they all depend on this one.

---

## Part B — Rebuild the UI as one visual system

`UI_SCREEN_INVENTORY.md` §2.5 lists the specific inconsistencies. The goal is
not to redraw every screen for its own sake — it is that a judge should not be
able to tell which session each screen was written in.

1. **One design system, defined once.** A single header owning the palette,
   the type scale, the standard title bar, the standard button, and the
   standard spacing grid. Today geometry is spread across `gui_draw.h` and
   `registration_ui.c`'s private defines, with two different "two big buttons"
   layouts that do not match.
2. **One title-bar treatment** with a colour that carries meaning (neutral /
   success / warning / alert), rather than three ad-hoc variants.
3. **Fix the text rendering.** `gui_draw_text()` scales an 8×8 bitmap font by
   integer replication, and callers centre text by hardcoding x-offsets they
   eyeballed for one specific string. At minimum add measured centring
   (`gui_draw_text_centered()`); a larger, cleaner font for headings is worth
   the flash if it fits.
4. **Differentiate Register from Dispense.** Screens 2 and 7 are currently the
   same function with the same words.
5. **Add a "Checking…" state** for the capture window — needs Part A.
6. **Clean the boot sequence** so nothing stale is visible before the home
   screen.

**Constraint: do not regress the elderly-friendly goal.** Large touch targets
and high contrast are the point, not a style to modernise away. While you are
here, check the palette against WCAG AA contrast ratios and check that no touch
target is smaller than the QWERTY keys' current 70×56 px — and preferably raise
those, since they are the smallest targets in the product.

---

## Part C — The mascot stays idle-only (do not build the other states)

`mascot_state_t` has four values, but `anime_ui_set_state()` is never called
from anywhere and the three non-idle state functions all render the identical
idle animation. The mascot has been frozen in `MASCOT_IDLE` since Session 04.

**This was decided in Session 12 and the answer is: leave it that way.** The
documents that claimed otherwise — `MASCOT_UI_DESIGN.md` §4 and
`SOFTWARE_ARCHITECTURE.md` §5, both of which said Session 11 wires
`MASCOT_SUCCESS`/`MASCOT_ERROR` to real events — have been corrected and now
state the idle-only design as intentional.

The reasoning, so nobody re-opens it: the UI already signals every one of those
outcomes with a **full-screen state change** — the dispensing screen, the
"I Took It" confirmation, the FACE NOT RECOGNISED retry screen, the alert
screens. A mascot animation saying the same thing again is a second channel
carrying no additional information, and it would compete for exactly the
framebuffer bandwidth Part A is trying to free up.

**What this session should do about the mascot instead:** make sure the idle
animation is *well integrated* into the new visual system — correct placement,
correct background, no tearing against the redesigned screens — and that it
still runs at its intended 4 FPS. Nothing more.

**Do not** add new mascot animations, and do not "helpfully" wire
`anime_ui_set_state()` while you are in the file.

---

## Part D — Production hygiene

1. **Debug output behind a switch.** Guard every `printf` on a normal path with
   `#if MEDSIGHT_DEBUG`, default off for a release build. Follow the pattern
   Session 12 established with `MEDSIGHT_DEBUG_DISKIO` and
   `MS_BOOT_LED_CHECKPOINTS`: gate it, do not delete it. **Fault reporting is
   not debug output** — `exc_hdr.c`'s handlers, `knl_default_handler()`, SD
   media faults and NPU init failures all stay unconditional.
2. **Re-verify the privacy rule against a real capture**, not by code review:
   no embedding bytes over UART. Names, dose counts and confidence scores are
   permitted.
3. **Fix the Release configuration's `-DDEBUG`.** `.cproject` defines `DEBUG`
   in the *Release* build and not in Debug — backwards, and harmless only
   because nothing currently keys off it. It will stop being harmless the
   moment Part D item 1 introduces a symbol that does.

---

## Part E — Contest documentation packaging

1. **`README.md` at the repository root.** Project overview and the problem it
   solves; the hardware (STM32N6570-DK, IMX335 camera, RK050HR18 touch panel,
   microSD — plus the Session 14 dispensing hardware if that session has run);
   the software stack (µT-Kernel 3.0, FatFs, ST Edge AI on the Neural-ART NPU,
   **CenterFace** detector + **MobileFaceNet** embedder); build instructions
   (STM32CubeIDE, no `.ioc`, manual peripheral config, and the headless build
   command from `ENGINEERING_LESSONS.md`); known limitations and future work.
   Link to `THIRD_PARTY_SOFTWARE.md` and state plainly that no µT-Kernel 3.0
   API specification was changed. Clear English, written for an international
   audience.
2. **`MedSight_Docs/DESIGN_PROTOTYPE.md`** — the physical design story, now
   partly real. Session 14 builds a working single-hopper turntable dispenser;
   `MECHANICAL_DESIGN.md`'s multi-hopper scaling remains design intent.
   Be precise about which is which, and include the 3D renders from
   `RAGNAR_CAD_PROMPT.md`.
3. **`MedSight_Docs/DEMO_SCRIPT.md`** — a shot list for the demo video: boot,
   register a patient, dispense to them (face recognition → dispense →
   confirm), show the SD log on a PC, and — if Session 14 has run — the
   physical dispense. Include the failure paths, because they demonstrate the
   hardening: an unrecognised face offering TRY AGAIN, and the SD card removed.
4. **Quote the measured numbers.** Session 12 measured ~89.6% CPU idle;
   `AI_PIPELINE.md` §5 still asks for NPU inference latency, which nobody has
   recorded. Measure it and fill it in.

---

## Definition of Done

- [ ] `sessions/session_13/` created per the folder rules; clean baseline build
      reproduced **before** any edits.
- [ ] **Part A**: the framebuffer question answered with evidence, and — if the
      memory is there — no visible corruption during a face capture.
- [ ] **Part B**: one design system; no screen visually out of place; text
      centring measured rather than hardcoded; Register and Dispense
      distinguishable.
- [ ] **Part C**: mascot still idle-only; its idle animation integrated
      cleanly into the new visual system. No new states built.
- [ ] **Part D**: debug output gated; privacy rule re-verified against a real
      UART capture; the Release `-DDEBUG` inversion fixed.
- [ ] **Part E**: `README.md`, `DESIGN_PROTOTYPE.md`, `DEMO_SCRIPT.md` written;
      NPU latency measured and recorded in `AI_PIPELINE.md` §5.
- [ ] Build 100% clean in **both** configurations, via the real STM32CubeIDE
      build (headless is fine — see `ENGINEERING_LESSONS.md`).
- [ ] **The full flow still works end-to-end on hardware.** This is the
      regression bar, and after a UI rewrite it is a real risk, not a
      formality.
- [ ] `MedSight_Docs/milestones/session_13_notes.md` written.
- [ ] `UI_SCREEN_INVENTORY.md` updated to describe the new UI.

---

## What This Session Does NOT Do

- **No new features.** Every screen here already exists or is an error path for
  something that already exists. New capability belongs to Sessions 14 and 15.
- **No changes to the AI models, the pipeline, or the gallery matching.**
- **No changes to `ms_osal.c`'s kernel-facing internals**, the vendored
  `mtk3_bsp2/` tree, or anything in `session_12_notes.md`'s "no removal"
  list.
- **No `.ioc` files, no networking.**
- **No physical dispensing hardware** — that is Session 14. If Session 14 has
  already run when you do this, its screens are in scope for the visual system;
  its driver is not.
