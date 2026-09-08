# Session 12 — µT-Kernel-Idiomatic Integration, Power Saving & System Hardening

## How to Start This Session

Hello! We are starting Session 12 for the MedSight project. Session 11 is
complete and verified on hardware: the whole application runs on µT-Kernel 3.0
with FreeRTOS fully removed.

**Before writing any code or taking any action**, acquire full context:

1. **READ ALL DOCUMENTATION**: every markdown file in `MedSight_Docs/` —
   especially `MASTER_PROJECT_PLAN.md`, `SOFTWARE_ARCHITECTURE.md`,
   `HARDWARE_ARCHITECTURE.md`, `ENGINEERING_LESSONS.md`, `AI_LESSONS.md`,
   `AI_PIPELINE.md`, and `COMPLIANCE_PRIVACY_POSTURE.md`.

2. **READ PAST SESSION PROMPTS**: `session_01.md` through `session_11.md`.

3. **READ `milestones/session_11_notes.md` IN FULL.** This is the single most
   important document for this session. It records nine addenda of real,
   hardware-confirmed bugs found during the µT-Kernel migration, several of
   which constrain what you may safely change here. In particular you must
   understand, before touching anything:
   - **Addendum 7** — the kernel's runtime-built vector table must be cleaned
     from the D-cache (`ms_osal_clean_dcache()`) because this project runs
     with both CPU caches enabled and the vendored BSP does no cache
     maintenance. Two call sites. **Do not remove them.**
   - **Addendum 8** — a SysTick ISR slower than its own tick period starves
     PendSV permanently. This is why there is no `printf()` anywhere on a
     per-tick or per-dispatch path, and why you must not add one.
   - **Addendum 9** — `CNF_TIMER_PERIOD` is 1 ms and `HAL_IncTick()` adds
     exactly one per call. `ms_osal.c`'s `OSAL_HAL_TICK_PERIOD_MS` is derived
     from that macro on purpose. If you change the kernel tick, HAL's
     millisecond clock must still advance one millisecond per real
     millisecond.
   - **Addendum 5a** — newlib's `_sbrk()` and µT-Kernel's `knl_init_Imalloc()`
     both start at `&_end` and are kept apart by an 8 KB reserve that
     `sysmem.c` and `sys_start.c` must agree on. **Any change to dynamic
     allocation in this session has to respect that split.**
   - The **"Session 12 closeout"** section at the end, which lists exactly
     what Session 11 left open — three of those items are Part 0 below.

4. **READ `milestones/session_10_notes.md` IN FULL** — the simulated dispense
   flow (face match → dispense animation → "I Took It"/"Skip" → SD log →
   `pills_remaining` decrement) that this session must keep working
   unchanged, exactly as Session 11 had to.

5. **READ THE ACTUAL WORKING CODE** in `sessions/session_11/FSBL/` before
   writing anything — at minimum:
   - `Src/ms_osal.c` and `Inc/ms_osal.h` — the OSAL, including the
     deferred-object-creation design (`usermain()`), the HAL tick bridge, and
     `ms_osal_clean_dcache()`. **This is the only file that may call
     `tk_*` APIs.**
   - `Src/main.c` — the four tasks and their priorities.
   - `Src/ui/state_machine.c` — the full dispense/registration state machine.
   - `Src/ai_vision.c` — and read its **memory-hazard comment carefully**
     (both NPU networks' activation scratch overlaps `BUFFER_ADDRESS`, the
     live camera framebuffer, and part of `GUI_BUFFER_ADDRESS`). This
     constrains Part A below more than anything else in the codebase.
   - `Src/sd_logger.c` — the logger task and its OSAL queue.

---

## Project Rules (non-negotiable, carried forward)

| Rule | Detail |
|---|---|
| No `.ioc` files | Manual HAL only. Never use STM32CubeMX. |
| No new hardware | No physical motors, servos, or IR sensors. The dispenser is 100% software-simulated. This is a firm cut, not a "not yet" — see `MASTER_PROJECT_PLAN.md` §6. |
| No networking | No Wi-Fi/Ethernet/BLE stack, ever. |
| OSAL boundary | Application code calls `ms_osal.h` only. **Zero `tk_*` calls outside `ms_osal.c`** — verify by grep before declaring done, as every session since 07 has. |
| No embeddings over UART | Never `printf`/log raw face-embedding bytes. Names, indices and confidence scores are fine. |
| µT-Kernel API spec unchanged | TRON Contest rule 1.3 permits modifying µT-Kernel **as long as the OS API specification is not changed**. You may edit the vendored BSP (Session 11 did, five times), but you may **never** change the signature or semantics of any `tk_*` call. Every such modification must be documented — see Part D. |
| No `printf` on a hot path | No `printf` inside any ISR, cyclic handler, timer handler, or the dispatcher. Addendum 8 explains what that costs. |
| New session = new folder | Copy `sessions/session_11` → `sessions/session_12`. Details below. |

---

## Your First Action — Create the Working Folder

Follow `ENGINEERING_LESSONS.md` exactly; this project has been bitten three
separate times by shortcuts here.

```powershell
Copy-Item -Path "C:\Users\Dousik\Workspace\TRON\sessions\session_11" `
          -Destination "C:\Users\Dousik\Workspace\TRON\sessions\session_12" -Recurse
```

Then, in order:

1. **Delete every generated build artifact** under `Debug/` and `Release/`:
   `*.d`, `*.o`, `*.su`, `*.cyclo`, `*.list`, `*.map`, `objects.list`. Never
   text-replace inside them — see `ENGINEERING_LESSONS.md`'s "Never sed the
   Whole Tree" lesson, which describes exactly how that corrupts `.o` files.
2. **Text-replace `session_11` to `session_12` in text files only**:
   `subdir.mk`, `makefile`, `.project`, `.cproject`, `.launch`. Note there are
   **112 `mtk3_bsp2` `subdir.mk` files** under `Debug/` carrying absolute
   paths, plus the top-level ones — do not miss them.
3. **Rename the build-artifact identity** `MedSight_Session11_FSBL` to
   `MedSight_Session12_FSBL` in `Debug/makefile`, `Release/makefile`,
   `.project` and the `.launch` file.
4. **Regenerate `objects.list`** — a plain command-line `make all` does not
   write it. Scan every `subdir.mk` under `Debug/sources.mk`'s `SUBDIRS` for
   `OBJS +=` entries. Session 11's had **321 objects**; diff yours against it
   and account for any difference before building.
5. **Build once, unchanged, before editing anything**, and confirm it is as
   clean as Session 11's final build
   (`text 734320, data 3992, bss 656824`, zero errors, zero warnings). If it
   is not, stop and fix the copy before writing a single line of new code.

Also note the newest lesson in `ENGINEERING_LESSONS.md`: **after restoring or
copying any source file from an older location, `touch` it before building and
then verify the compiled artifact, not the build log.** `Copy-Item` preserves
the source's timestamp, and make will silently skip a file that looks older
than its object.

No external NPU flash re-programming is needed unless you change a model file
— the OSPI weight data from Session 08B carries over.

---

## What Session 11 Leaves You

- **Everything runs on µT-Kernel 3.0.** Verified against the linked `.elf`:
  zero FreeRTOS symbols; PendSV maps to `knl_dispatch_entry`; SysTick maps to
  `knl_systim_inthdr`; zero `tk_*` calls outside `ms_osal.c`.
- **The full application works end-to-end on hardware** — registration (face
  capture → keyboard → pill count → confirm → `patients.dat`) and dispense
  (face match → animation → "I Took It" → log → `pills_remaining` decrement),
  both confirmed from real UART captures.
- **A clean, instrumentation-free kernel tree.** Seven vendored files are
  byte-identical to upstream again. Only `sys_start.c`, `interrupt.c` and
  `exc_hdr.c` differ, and only by documented fixes.
- **An OSAL with exactly four primitives**: `osal_task_create`,
  `osal_queue_create/send/receive`, `osal_mutex_create/lock/unlock`,
  `osal_delay_ms` (plus `osal_scheduler_start`). Deliberately the
  lowest-common-denominator set, chosen to make the Session 11 swap
  mechanical. **Widening it is a large part of this session's job.**

---

## Why This Session Exists (read this before deciding what to build)

`MASTER_PROJECT_PLAN.md`'s v6 changelog proposed an optional "Session 12B"
that would replace generic primitives with idiomatic µT-Kernel mechanisms,
and v8 dropped it. It is being revived here for a concrete reason found by
reading the contest rules directly.

TRON Programming Contest 2026, **rule 1.4 (RTOS Application category
evaluation criteria)** says:

- programs will be highly evaluated for *"realizing the features of an RTOS
  program such as **real-time performance, power saving, and small memory
  footprint**"*; and
- for AI: *"You are free to use AI technology as you like, but the **high
  degree of relevance to µT-Kernel 3.0** will be highly evaluated."*

As of Session 11 this project satisfies the **requirement** (rule 1.1: an
application running on µT-Kernel 3.0) but scores weakly on both of those
**evaluation criteria**:

- The AI's coupling to µT-Kernel is minimal. `ai_vision_run_pipeline()` is
  called synchronously from the UI task and the ST AI runtime is configured
  `LL_ATON_OSAL_BARE_METAL` — it spins in a polling loop and never calls an
  OS primitive. The NPU work happens *alongside* the RTOS, not *through* it.
- There is no power saving at all. `sysdepend/stm32_cube/power_save.c`'s
  `low_pow()` is an **empty function**, so the dispatcher's idle loop spins
  the Cortex-M55 at full clock whenever no task is runnable.

Both are fixable, both are honest engineering improvements rather than
box-ticking, and both are exactly what the criteria ask for.

**A warning about how to do this.** The point is to make the system genuinely
better, not to sprinkle `tk_` calls around for the judges. If you evaluate one
of the mechanisms below and find it has no real consumer in this codebase,
**say so in the session notes and skip it.** A forced, artificial use of a
memory pool reads worse to an expert judge than not using one. This project's
convention has always been to record deviations rather than let them go
silent — that applies here more than anywhere.

---

## Part 0 — Close out Session 11 (do this first, before any new code)

These are the three items `session_11_notes.md`'s closeout section left open.
None require new code; all must pass before you start Part A, because
otherwise you will not know whether a later failure is yours or inherited.

1. **STM32CubeIDE GUI clean build, both configurations.** Everything since
   Session 11's Addendum 1 has been built from the command line.
   `Project → Clean...` then build **Debug** and **Release** in the real IDE.
   - `.cproject` is already correct for both configs (verified in Session 11:
     ten `mtk3_bsp2` include paths and both `_STM32CUBE_DISCOVERY_N657_`
     defines in each).
   - `Release/` currently has **zero** `mtk3_bsp2` `subdir.mk` files against
     `Debug`'s 112, and its `Application/User/subdir.mk` is missing
     `registration_ui.c` — a **Session 09** file. Release has not been built
     since Session 08B; this is not a Session 11 regression.
   - **The fix is the IDE regenerating the tree from `.cproject`, not hand-
     writing 112 makefiles.** `ENGINEERING_LESSONS.md` hard rule #2 and
     Session 11's Addendum 1 both say so explicitly. If the IDE build still
     fails afterwards, the bug is in `.project`/`.cproject`, and that is where
     to fix it.
2. **Flash and confirm the stripped build still runs.** Session 11's final
   instrumentation strip was verified against the `.elf` but the resulting
   binary was never flashed. Confirm boot → home screen → one full dispense
   cycle. If something now fails, suspect a removed `printf` that was
   accidentally load-bearing (a timing side effect) rather than a real logic
   change, and say so.
3. **Long-running stability soak** — several minutes of continuous operation
   with the camera live, matching the bar Session 07 set at the previous OS
   swap. Watch specifically for anything the deferred object-creation design
   in `ms_osal.c` could have gotten subtly wrong that a single pass cannot
   catch: a priority collision, the cyclic tick bridge drifting (compare
   `HAL_GetTick()` against a stopwatch over 60 s — it should track real time
   within a percent or so), or a slow leak in the kernel heap.

Record the results of all three in the session notes. **If the soak fails,
stop and fix that before anything else in this session** — it invalidates
Session 11's completion claim.

---

## Part A — Make the AI genuinely µT-Kernel-integrated

This is the highest-value work in the session and also the riskiest. Read the
constraint below before designing anything.

### The constraint that shapes this whole part

`ai_vision.c` carries a documented memory hazard from Session 08B: **both NPU
networks' activation scratch overlaps `BUFFER_ADDRESS` (the live camera
framebuffer) and part of `GUI_BUFFER_ADDRESS`.** `ai_vision_run_pipeline()`
handles this today by being called with the camera stopped, snapshotting what
it needs, and running to completion before anything else touches those
buffers. Sessions 09 and 10 both explicitly forbid "simplifying" that code
without re-reading why it is structured that way.

So: moving inference into its own concurrent task is **not** a free
refactor. If the AI task can run while the UI task draws into
`GUI_BUFFER_ADDRESS`, or while the camera task restarts the ISP, you will get
corrupted frames or a fault — and it will be intermittent, which is the worst
kind.

### A1. Decide the architecture, with evidence

Work out, from the actual code, whether the NPU pipeline can be moved into
its own µT-Kernel task with **explicit buffer ownership** enforced through OS
primitives. Write down the answer either way.

- **If yes** — implement it: a dedicated AI task; the UI task posts a request
  and waits on an event flag; the AI task runs inference and sets the
  completion flag; buffer ownership between the AI task, the camera/ISP task
  and the UI's drawing is enforced by an OSAL mutex or by the same flag set.
  This is the strongest outcome: it removes a multi-millisecond stall from
  the UI task (**real-time performance**, rule 1.4), lets the mascot keep
  animating during inference, and makes the AI genuinely mediated by
  µT-Kernel (**relevance to µT-Kernel**, rule 1.4).
- **If no** — do **not** force it. Fall back to A2 alone and document the
  buffer-ownership analysis that ruled it out. That analysis is itself a
  strong thing to have in the submission; "we measured and it does not fit"
  is a better engineering story than a race condition in a demo.

### A2. Event flags (`tk_cre_flg` / `tk_set_flg` / `tk_wai_flg`)

Extend the OSAL with an event-flag primitive and use it where the codebase
genuinely has a **multi-condition wait**. Candidates to evaluate against the
real code:

- the AI-completion handshake from A1 (if A1 is implemented);
- the dispense flow's wait for "animation minimum duration elapsed **AND**
  inference finished" — a true `TWF_ANDW` two-condition wait, which is the
  case the original Session 12B proposal named;
- `STATE_CONFIRM_TAKEN`'s "confirmed **OR** skipped **OR** timed out", which
  is a `TWF_ORW` wait with a timeout and today is a polling loop.

Add to `ms_osal.h` something along the lines of `osal_flag_create`,
`osal_flag_set`, `osal_flag_clear`, and `osal_flag_wait(handle, pattern,
wait_mode, timeout_ms)`. Keep the naming and style consistent with the
existing four primitives, and keep every `tk_*` call inside `ms_osal.c`.

**Mind the deferred-creation design.** `ms_osal.c` defers every
`osal_*_create()` made before `osal_scheduler_start()` into `usermain()`, and
its "create after the kernel is already running" branch **has never been
exercised on hardware**. Create your flags in `main()` alongside the existing
objects, so you stay on the proven path — or, if you deliberately exercise
the post-kernel branch, test it explicitly and say so in the notes, because
you will be the first to do so.

### A3. Task priorities — replace inherited numbers with a reasoned scheme

`main.c`'s priorities (5 cam_isp, 4 ui, 2 logger, 1 heartbeat) were carried
over unexamined from FreeRTOS, and `ms_osal.c` inverts them into µT-Kernel's
convention (`OSAL_PRI_CEILING - priority`, ceiling 16). The **ordering** is
correct; the numbers have never been justified.

Derive a deliberate scheme from each task's actual period and deadline —
camera/ISP at frame rate, UI touch polling at its poll period, logger's
tolerance for latency, heartbeat with none. Rate-monotonic reasoning is the
obvious frame; whatever you use, **write down the reasoning** and note where
the new AI task (if any) sits and why. Adjust the numbers only if the
reasoning actually demands it — renumbering for its own sake is churn.

### A4. Fixed-size memory pool (`tk_cre_mpf`) — evaluate honestly, adopt only if real

The original Session 12B proposal named camera frame buffers. **Check
before implementing**: this project's framebuffers are static and
linker-placed, not dynamically allocated, so a pool there would be
artificial. Look instead for a genuine fixed-size allocation site —
`sd_logger`'s log records are the most plausible candidate. If nothing in the
codebase genuinely allocates fixed-size blocks at runtime, **skip this and
record why.** See the warning above about forced idioms.

---

## Part B — Power saving

Rule 1.4 names power saving explicitly, and this project currently has none.

`sysdepend/stm32_cube/power_save.c`'s `low_pow()` is an **empty function**.
µT-Kernel's dispatcher already calls it from the idle path (`dispatch.S`,
label `l_dispatch_110`) whenever there is no runnable task and
`knl_lowpow_discnt` is zero — so the hook is wired, it just does nothing, and
the Cortex-M55 spins at full clock in the idle loop.

Implement it as a `WFI` (with `DSB` before and `ISB` after). This is a
legitimate BSP modification under contest rule 1.3 — it changes no OS API.

**Verify wake-up on hardware, do not assume it.** `low_pow()` is called with
`BASEPRI` set to `INTPRI_VAL(INTPRI_MAX_EXTINT_PRI)` (0x10), which masks both
SysTick and PendSV. Whether a `WFI` still wakes on a BASEPRI-masked interrupt
is a detail you must confirm empirically on this silicon rather than reason
about from the ARM ARM. **If the board stops ticking after this change, that
is the cause**, and the fix is to clear `BASEPRI` around the `WFI` and
restore it afterwards. Test this change on its own, before stacking anything
else on top of it, so an idle-loop hang is unambiguous.

Then **measure and record something real** — idle-vs-busy current draw if you
can measure it, or at minimum the proportion of time the system spends in the
idle path (e.g. a counter incremented in `low_pow()` versus elapsed ticks,
read out on demand rather than printed per-tick). A number in the submission
beats the claim "we call WFI".

---

## Part C — System hardening (the original Session 12 scope)

Everything below was the whole of Session 12 before Parts A/B were added. It
is still required. If the session runs long, Part C item 6 (the stress test)
is the last thing to cut, not the first — it is the only item here that
validates the others.

1. **SD card hot-plug resilience.** If the card is removed mid-use, log the
   error, show a non-crashing error screen, and auto-retry the mount on the
   next operation. **Never call `Error_Handler()` for an SD failure** —
   degrade gracefully. Note that `ENGINEERING_LESSONS.md`'s Session 06
   findings already warn that FatFs polling loops can hang forever without a
   timeout; check every such loop on this path.
2. **Gallery full.** At `MAX_PATIENTS` (10), show "Gallery full — please
   contact admin" rather than failing silently. Session 09 already returns
   `-1` from `gallery_add_patient()`; make the UI path honest about it.
3. **Face-recognition retry.** The 3-retry loop already exists. Replace the
   dead end after three failures with a "Face not recognised" screen offering
   **Try Again** and **Cancel**, instead of returning home unconditionally.
4. **Pill count tracking.** `pills_remaining` is already decremented and
   persisted (Session 10). Add the missing piece: when it reaches 0, show a
   "Refill needed" alert. Keep the existing underflow guard.
5. **Full SD log review.** Confirm every state transition, dispense and
   confirmation is logged. Then **cut the debug UART volume**: the
   `disk_read:` / `disk_write:` tracing from Sessions 06/08B prints four
   lines per sector operation — roughly 13 ms of blocking UART each at
   115200 — in the logger task's hot path. Guard it behind a single
   `#if MEDSIGHT_DEBUG_DISKIO` (default off). While there, note that
   concurrent `printf` from two tasks can interleave mid-line (this build
   links `--specs=nano.specs` with no retargetable locking; a garbled line is
   visible in Session 11's capture). Either serialise debug output through
   the existing logger task or document it as a known cosmetic limitation —
   **do not** add a lock on a hot path without re-reading Addendum 8.
6. **Stress test on hardware.** Register 3 patients; dispense to each in
   sequence; verify the SD log is complete and correct; verify face matching
   still works after **10 repeated dispense cycles**. This is also the
   regression test for Parts A and B — if the AI restructure has a race, this
   is what will find it.

---

## Part D — Third-party software inventory (contest rule 1.3)

TRON Contest rule 1.3 requires, for **every** piece of existing software by
others used in the program:

1. name, rights holder, acquisition method, and function, in the
   documentation;
2. provision of that software so the organizer can evaluate the program for
   free, in usable form, until about one week after the awards ceremony;
3. a written guarantee that copyrights and other rights have been handled per
   the Application Rules.

Create `MedSight_Docs/THIRD_PARTY_SOFTWARE.md` and inventory what this project
actually links. Gather it from the build, not from memory — walk
`Debug/objects.list`, the `-l` flags in the link line, and the licence headers
in the source tree. At minimum it will include: CMSIS and STM32N6xx CMSIS
(Arm/ST), STM32N6xx HAL/LL drivers and the STM32N6570-DK BSP (ST, BSD-3-Clause),
FatFs (ChaN), ST's STM32_ISP_Library, the X-CUBE-AI / ST Edge AI runtime
(`NetworkRuntime1200_CM55_GCC.a`, the `ll_aton_*` sources), the eVision AE/AWB
libraries, the µT-Kernel 3.0 BSP2 tree itself (TRON Forum, T-License 2.1), and
the face-recognition model wrappers.

Two specifics worth care:

- **The face models.** `Src/ai/stai_fd.c`, `stai_faceid.c` and `faceid.c` are
  ST Edge AI generated wrappers, `(c) 2024 STMicroelectronics`, obtained via
  the STM32N6 face-recognition reference project in `scratch/PeleAB_repo/`,
  whose own `LICENSE.md` places its `Model/` directory under **ST SLA0044**.
  Record the model architectures accurately too — `AI_PIPELINE.md` says
  CenterFace detector + FaceID embedder while `session_13.md` currently says
  "SCRFD + MobileFaceNet"; **check the actual headers and fix whichever doc is
  wrong.**
- **Modifications to µT-Kernel.** Rule 1.3 permits them provided the OS API
  specification is unchanged. List every vendored-BSP file this project has
  edited and why, with an explicit statement that no `tk_*` API signature or
  semantic was altered. Session 11's addenda 5–9 already contain the
  reasoning; this is a summary table, not new analysis. Add Session 12's own
  changes (`power_save.c`, and anything Part A touches).

This document is a Session 13 packaging input, but it must be written here,
while the code is fresh.

---

## Definition of Done

- [ ] `sessions/session_12/` created from `session_11` per the folder rules;
      clean baseline build reproduced **before** any edits.
- [ ] **Part 0**: IDE clean build passes for **both** Debug and Release; the
      stripped Session 11 binary confirmed running on hardware; multi-minute
      soak passed with `HAL_GetTick()` tracking real time.
- [ ] **Part A**: OSAL extended with event flags; the AI/UI handshake either
      restructured around them **or** explicitly ruled out with a written
      buffer-ownership analysis; task priorities justified in writing;
      memory-pool question answered either way.
- [ ] **Part B**: `low_pow()` implemented and **hardware-verified not to stall
      the system**; a real idle-time or current-draw measurement recorded.
- [ ] **Part C**: all six hardening items done; `disk_read`/`disk_write`
      tracing behind a default-off guard.
- [ ] **Part D**: `MedSight_Docs/THIRD_PARTY_SOFTWARE.md` written, including
      the µT-Kernel modification table and the API-unchanged statement.
- [ ] Build 100% clean — zero errors, zero warnings.
- [ ] **The Session 10 dispense flow still works end-to-end on hardware**,
      exactly as it did after Session 11. This is the regression bar for the
      whole session.
- [ ] Zero `tk_*` calls outside `ms_osal.c` (grep-verified).
- [ ] No face-embedding bytes in a real UART capture.
- [ ] `MedSight_Docs/milestones/session_12_notes.md` written, recording every
      deviation, every idiom evaluated-and-skipped and why, and the measured
      numbers from Parts B and C.
- [ ] `SOFTWARE_ARCHITECTURE.md` §4's OSAL table updated to match the widened
      API; `MASTER_PROJECT_PLAN.md` changelog updated.

---

## What This Session Does NOT Do

- **No physical motors, servos or IR hardware.** Still a firm cut.
- **No networking**, of any kind.
- **No new AI models.** The NPU pipeline's *models* are untouched; only how
  the application waits on them may change.
- **No `.ioc` files.**
- **No changes to any `tk_*` API signature or semantic** — contest rule 1.3.
- **No new UI features.** Screens added in Part C are error/alert paths, not
  new functionality. Cosmetic polish belongs to Session 13.
- **No removal of Session 11's fixes** — `ms_osal_clean_dcache()` and its two
  call sites, the newlib heap reserve, `N_INTVEC 195`, `USE_TMONITOR 0`,
  `CNF_SYSTEMAREA_END`, `CNF_TIMER_PERIOD 1`, and `exc_hdr.c`'s fault
  printing all stay. Every one was found the hard way; `session_11_notes.md`
  says how.
