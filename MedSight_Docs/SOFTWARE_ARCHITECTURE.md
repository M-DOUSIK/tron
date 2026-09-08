# SOFTWARE_ARCHITECTURE.md — MedSight

## 1. Firmware Lifecycle Strategy

Bare-metal (Sessions 01–06) → FreeRTOS behind an OS Abstraction Layer, OSAL
(Sessions 07–10) → µT-Kernel 3.0 swapped in behind the same OSAL (Session 11,
done and hardware-verified — see `milestones/session_11_notes.md`) →
µT-Kernel-idiomatic integration, power saving and hardening (Session 12, done —
see `milestones/session_12_notes.md`) → final polish/demo packaging
(Session 13). Session 13 is the last planned session —
see `MASTER_PROJECT_PLAN.md`'s Changelog for the renumbering history (this
used to run through Session 16 with optional stretch sessions; those were
dropped along with the physical-hardware cut below).

**Physical dispensing hardware was cut from this project entirely** (decision
recorded in `MASTER_PROJECT_PLAN.md`'s Changelog, first reflected in
`prompts/session_10.md`). No `dispenser.c` module exists, no hopper concept exists
in the patient data model, and no GPIO/motor/IR pins are assigned. Everywhere this
document previously described hopper hardware, it now describes the software-only
simulated dispense flow instead.

The OSAL is the whole point of this staging: FreeRTOS lets you build and debug quickly
with a mature, well-documented API, while guaranteeing the final TRON-mandated swap to
µT-Kernel 3.0 only touches one file (`ms_osal.c`), not the whole application.

## 2. Folder Structure

```
tron/
  session_01/ ... session_14/      -- one full project snapshot per completed session
                                       (session_15/ optional, post-submission)
Core/
  Inc/
  Src/
    main.c
    ms_osal.c / ms_osal.h          -- OS Abstraction Layer (Session 07+)
    camera_lcd.c/.h                 -- Session 03 (built on the jpcano/STM32N6-digits
                                        LCD reference, see ENGINEERING_LESSONS.md)
    ui/
      anime_ui.c/.h                 -- Session 04 (mascot rendering, states driven Session 05+)
      touch_driver.c/.h             -- Session 05 (GT911 touch controller, I2C2, PD14/PD4)
      gui_draw.c/.h                 -- Session 05+ (screen drawing primitives, the
                                        elderly-friendly palette and every full-screen
                                        layout; Session 05's interactive_gui.c was
                                        folded into this plus state_machine.c and no
                                        longer exists as a file)
      registration_ui.c/.h          -- Session 09 (enrollment flow: face capture hand-off,
                                        on-screen keyboard, pill count, confirm)
    sd_logger.c/.h                  -- Session 06 (event log), extended Session 09
                                        (patient-profile read/write/list/delete)
    ai_vision.c/.h                  -- Sessions 08A-B (toolchain proof, one-shot face
                                        recognition + gallery matching). Action/
                                        consumption recognition (originally planned
                                        as Session 08C) was dropped before it was
                                        ever built — no session_08C.md exists, and
                                        no second model runs on the NPU. Consumption
                                        is confirmed by a manual "I Took It" button
                                        instead (Session 10).
    schedule_time_source.c/.h       -- Session 10 (swappable fast-timer/RTC abstraction)
    state_machine.c/.h              -- Session 10 (dispense-flow orchestration —
                                        simulated dispense only, see §7);
                                        Session 12 added the error/alert states and
                                        made the capture states non-blocking (§9)
  Inc/
    patient_profile.h               -- Session 09 (shared patient-record struct)
docs/                               -- this documentation set, kept current
```

## 3. Module Boundary Rules

- `camera_lcd`, `anime_ui`, `touch_driver`, `gui_draw`, `registration_ui`,
  `sd_logger`, `ai_vision` never call OS primitives directly — only through
  `ms_osal.h`. This is what makes the Session 11 µT-Kernel migration mechanical
  rather than a rewrite.
- `state_machine` is the only module allowed to orchestrate calls across the other
  functional modules — individual modules don't call each other directly. (Session 09's
  `registration_ui` is a partial, deliberate exception: it directly calls `ai_vision`'s
  embedding-capture function and `sd_logger`'s profile-write function, since
  registration is its own self-contained flow that predates `state_machine.c`'s
  existence in the session order — document this explicitly as an accepted exception
  in Session 09's notes, not silently.)
- No module outside `ai_vision.c` touches the NPU/STM32Cube.AI runtime. As of
  Session 12 that file also owns the NPU **task** (`task_ai_fn`), the same way
  `sd_logger.c` has owned the logger task since Session 07: `main.c` creates
  it, but the body and everything it touches live in the module that owns the
  hardware. `state_machine.c` reaches it only through `ai_vision.h`'s
  request/wait pair, never by calling the pipeline directly.
- No module outside `sd_logger.c` touches FATFS/SDMMC directly — everything else logs
  or reads profile data through its API.
- No module outside `touch_driver.c` touches the touch controller's I2C2 bus directly —
  `state_machine.c` and `registration_ui.c` consume touch events through
  `touch_driver.h`'s API only.
- No module outside `schedule_time_source.c` reads the fast-timer/RTC directly —
  `state_machine.c` asks "what's due now" through this module's API only, which is
  what makes the prototype-timer/real-RTC swap a single-file change later.

## 4. OSAL API Surface (defined Session 07, remapped Session 11)

Minimum surface needed, mapped to both backends:

| OSAL call | FreeRTOS backend (historical) | µT-Kernel 3.0 backend (current) |
|---|---|---|
| `osal_task_create` | `osThreadNew` | `tk_cre_tsk` / `tk_sta_tsk` |
| `osal_queue_create` / `send` / `receive` | FreeRTOS queue API | `tk_cre_mbf` / `tk_snd_mbf` / `tk_rcv_mbf` |
| `osal_mutex_create` / lock / unlock | FreeRTOS mutex API | `tk_cre_mtx` / `tk_loc_mtx` / `tk_unl_mtx` |
| `osal_delay_ms` | `osDelay` | `tk_dly_tsk` |
| `osal_flag_create` / `set` / `clear` / `wait` **(Session 12)** | — (never existed) | `tk_cre_flg` / `tk_set_flg` / `tk_clr_flg` / `tk_wai_flg` with `TWF_ANDW`/`TWF_ORW`/`TWF_BITCLR` |
| `ms_osal_low_power_idle` **(Session 12)** | — (never existed) | called from the BSP's `low_pow()`, which µT-Kernel's dispatcher invokes on its idle path |
| `ms_osal_clean_dcache` (Session 11) | — (never existed) | called from the BSP's `sys_start.c` / `interrupt.c` |

**Session 12 widened this surface, once, deliberately (done).** The original
four primitives were the lowest common denominator between the two backends,
chosen precisely so the Session 11 swap would be mechanical. That worked — but
it also left the finished firmware talking to µT-Kernel only through primitives
every RTOS has, which scores weakly against TRON Contest rule 1.4's "high
degree of relevance to µT-Kernel 3.0". FreeRTOS was removed in Session 11, so
there is no longer a second backend to keep the surface narrow for, and the
FreeRTOS column above is now a historical record of the migration mapping, not
a live two-backend contract.

What Session 12 added, and what it deliberately did **not**:

- **Added: event flags.** They have a genuine consumer — the AI
  request/response handshake between the UI task and the new NPU task (§9).
  The UI's wait has three outcomes to distinguish in one blocking call (face
  found / no face / the AI task never answered), which is an OR-wait with a
  timeout: exactly what an event flag expresses and what a queue, a semaphore
  or a mutex cannot.
- **Evaluated and skipped: a fixed-size memory pool (`tk_cre_mpf`).** This
  codebase has no fixed-size runtime allocation site. Frame buffers are
  linker-placed constants; the NPU's activation pools are addresses baked into
  ST's generated code; the log records that looked like candidates are passed
  by *copy* through a message buffer and never allocated at all. Adopting a
  pool would have meant inventing an allocation in order to have something to
  pool.
- **Evaluated and skipped: an event flag for `STATE_CONFIRM_TAKEN`'s
  "confirmed OR skipped OR timed out" wait.** All three conditions are
  produced by the *same* task that would wait on them (the UI task polls touch
  itself), so the flag would have been set and waited on by one task — a
  synchronisation object with nothing to synchronise.

`prompts/session_12.md` is explicit that a forced idiom reads worse to an expert
judge than an absent one; `milestones/session_12_notes.md` records each of these
decisions with its evidence.

No application code calls `osThreadNew`, `osDelay`, etc. directly — enforce this in code
review during every session from 07 onward. See `ENGINEERING_LESSONS.md` for the
`mtk3bsp2_samples` reference used to validate correct µT-Kernel API usage in Session 11.

**Session 11 addendum (done — see `milestones/session_11_notes.md`):** the
table above describes the steady-state mapping once the kernel is running.
µT-Kernel's object-creation calls (`tk_cre_tsk`, `tk_cre_mbf`, `tk_cre_mtx`)
can only be issued after the kernel itself has started — but every
`osal_*_create()` call in this codebase happens in `main()` *before*
`osal_scheduler_start()`. `ms_osal.c`'s µT-Kernel backend bridges this by
deferring real object creation to its own `usermain()` (called by the kernel
once it's up, before any application task runs). This is purely an
`ms_osal.c`-internal detail — the OSAL API and every caller are unaffected.

## 5. Mascot State Enum (introduced Session 05, driven by real events from Session 10)

```c
typedef enum {
    MASCOT_IDLE,
    MASCOT_ACTIVE,
    MASCOT_SUCCESS,
    MASCOT_ERROR
} mascot_state_t;
```

`registration_ui.c` owns state transitions triggered by touch;
`state_machine.c` (from Session 10 onward) owns state transitions triggered by system
events (face-match results, simulated-dispense/OK-confirmation results). All funnel
through the same OSAL queue into `anime_ui.c`'s renderer — `anime_ui.c` itself never
decides *why* the state changed, only *how* to render it.

## 6. Patient Profile Data Model (Session 09, as actually implemented)

```c
/* ai_vision.h — the real, shipped struct. Simpler than an earlier draft of this
 * doc's patient_profile_t (which had a per-medicine hopper_id schedule array and
 * a phone_number field) — that draft assumed physical multi-hopper hardware and a
 * phone-notification feature, both since cut (see MASTER_PROJECT_PLAN.md's
 * Changelog). With no hopper to map a schedule entry to, "one daily pill count per
 * patient" is all the data model needs. */
#define EMBEDDING_SIZE    128
#define MAX_PATIENTS      10
#define PATIENT_NAME_MAX  32

typedef struct {
    uint8_t   valid;
    char      name[PATIENT_NAME_MAX];
    int8_t    embedding[EMBEDDING_SIZE];   // from ai_vision.c, Session 08B
    uint8_t   pill_count;                  // daily pill count, set at registration
    uint8_t   pills_remaining;             // decremented per confirmed dose (Session 10+)
} PatientRecord;
```

Defined once in `ai_vision.h` so `sd_logger.c` (storage, via `gallery_save()`/
`gallery_init()`), `ai_vision.c` (embedding shape, gallery matching), and
`registration_ui.c`/`state_machine.c` (population and consumption, Session 09/10)
all agree on its shape without duplicating the definition.

## 7. State Machine (Session 10 — simulated dispense, no physical actuators)

Per `prompts/session_10.md`'s "Hardware decision (FINAL)": no motors, servos, or IR
sensors are interfaced, at any session. "Dispensing" is an on-screen animation, and
consumption is confirmed by the patient tapping a button — not by any sensor or a
second NPU model (action recognition was evaluated and dropped, see
`MASTER_PROJECT_PLAN.md` §8).

```
STATE_HOME (camera OFF, main screen shown)
  -> user taps "Dispense Medicine"
STATE_INSTRUCT_DISPENSE -> STATE_CAMERA_DISPENSE
  -> run ai_vision_run_pipeline(), match against enrolled gallery (Session 08B/09 data)
STATE_CAMERA_DISPENSE -> (no match / no face) -> local alert, log, camera OFF, STATE_HOME
STATE_CAMERA_DISPENSE -> STATE_DISPENSING (match found)
  -> show patient name + pill count, ~2-3s animated countdown (simulates the
     mechanical action), log "DISPENSE: <name> <count> pills"
STATE_DISPENSING -> STATE_CONFIRM_TAKEN
  -> large "✓ I Took It" button; a smaller "Skip" button is available for
     caretaker use and returns home without logging a confirmation
STATE_CONFIRM_TAKEN -> (button tapped) -> log "CONFIRMED: <name> took pills",
  decrement and re-save pills_remaining, camera OFF, STATE_HOME
STATE_CONFIRM_TAKEN -> (timeout, no tap) -> MASCOT_ERROR, SD log, camera OFF, STATE_HOME
```

**Session 12 additions to this diagram.** Two states were added, both on error
or alert paths rather than in the happy path above, which is unchanged:

```
STATE_FACE_RETRY   -- reached instead of dead-ending home when a capture finds
                      no face after 3 attempts, or finds a face that is not in
                      the gallery. Offers TRY AGAIN (back to the capture state
                      it came from) and CANCEL (home); times out to home after
                      30 s so it can never strand the device.
STATE_ALERT        -- one-button screen for conditions the user must act on
                      outside the device: gallery full (also now refused up
                      front, before a face is captured), no pills remaining,
                      and "the dose was dispensed but could not be saved".
```

Edge cases (handled explicitly, not as afterthoughts), each driving `mascot_state_t`
above to `MASCOT_ERROR` unless noted:
- Missed dose (fast-timer/RTC schedule window elapses with no Dispense Medicine tap) →
  `MASCOT_ERROR` + SD log, remain `STATE_HOME`.
- Unrecognized/no face at the face-check step → `MASCOT_ERROR`, local alert (LCD +
  buzzer, not a phone push — see `MASTER_PROJECT_PLAN.md` §7), SD log, no dispense,
  camera OFF, return to `STATE_HOME`.
- Missed consumption confirmation (neither "I Took It" nor "Skip" tapped within a
  timeout) → `MASCOT_ERROR` + SD log, camera OFF, `STATE_HOME`. There is no jam/sensor
  edge case — that only existed under the physical-hopper design and no longer applies.
- Successful cycle → `MASCOT_SUCCESS`, SD log, camera OFF, then back to `MASCOT_IDLE`.

## 8. Pin Map

| Peripheral | Pins/Bus | Session assigned | Notes |
|---|---|---|---|
| Debug UART | USART1, via ST-LINK VCP | Session 02 | |
| Camera / LCD | DCMIPP + LTDC (RGB888 parallel bus across PA, PB, PD, PE, PG, PH), PSRAM framebuffers | Session 03 | Based on jpcano/STM32N6-digits reference. Hardware ISP configured in camera_lcd.c. |
| Touch controller (GT911) | I2C2 — PD14 (SCL), PD4 (SDA), FSBL context in `.ioc` | Session 05 | Confirmed via real prior bring-up on this board, not guessed — see `ENGINEERING_LESSONS.md` |

This table stays a living document — record actual pin assignments here as each
session finalizes them, don't write speculative pin numbers ahead of the hardware work.
No dispenser-actuator pins are listed here because none are built — see §1's note on
the physical-hardware cut.

## 9. Task Set and Priority Scheme (Session 12)

Sessions 07-11 ran four tasks with priorities 5/4/2/1, argued informally
("camera must never be starved", "touch should feel immediate"). The ordering
was right, but the numbers had never been justified. Session 12 re-derived them
rate-monotonically — shortest period gets the highest priority — added the NPU
task, and left the existing numbers alone because the derivation agreed with
them. The previously vacant level 3 is now occupied.

| Pri | Task | Where its body lives | Period | Deadline / rationale |
|---|---|---|---|---|
| 5 | `cam_isp` | `main.c` | 1 ms | Shortest period and the only task tied to external hardware timing: `ISP_BackgroundProcess()` must consume each frame's statistics before the next VSYNC (~33 ms at 30 fps). Missing it degrades AE/AWB convergence visibly. |
| 4 | `ui` | `main.c` → `state_machine.c` | 10 ms | Touch-to-response budget. A 10 ms poll plus the 4 FPS mascot frame gate stays an order of magnitude inside the ~100 ms at which input lag becomes noticeable. Session 11's Addendum 9 is the evidence: when a wrong kernel tick stretched these deadlines 10×, the device immediately "felt slower than FreeRTOS". |
| 3 | `ai` | `ai_vision.c` | on demand | **No deadline.** Hundreds of milliseconds of solid NPU/CPU work per request, a few times per session, in response to a button press the user already expects to take a moment. Deliberately below the UI so it is preemptible — that is what keeps touch and the physical USER1 button alive during inference. Above the logger because a person is waiting on its result and nobody waits on a log line. |
| 2 | `logger` | `sd_logger.c` | event-driven | Tolerates seconds of latency by construction; the async queue exists so no caller ever waits on a 10-50 ms SD write. |
| 1 | `heartbeat` | `main.c` | 500 ms | No deadline at all. Deliberately lowest, so "the LED stopped blinking" means "something above me is starving the system" — which is exactly the signal it should carry. Also carries the periodic idle/power report (§10). |

`ms_osal.h`'s convention is 1 = lowest; `ms_osal.c` inverts it onto µT-Kernel's
opposite scale (1 = highest) as `OSAL_PRI_CEILING - priority`, so 5/4/3/2/1
become `itskpri` 11/12/13/14/15. Only the ordering is load bearing; the absolute
numbers leave headroom on both sides inside `mtk3_bsp2`'s `CNF_MAX_TSKPRI` of 32.

### Frame-buffer ownership

`BUFFER_ADDRESS` (0x34200000) is written by three different things — the DCMIPP
camera DMA, the UI's drawing code, and (because ST's codegen hardcodes both
networks' activation scratch to overlap it) the NPU. Until Session 11 they could
not collide, because inference ran synchronously inside the UI task. With a
separate AI task that is no longer automatic, so ownership is now explicit:

```
UI task                                   AI task
────────────────────────────────────────  ──────────────────────────────────
camera_stop()            (DMA stops)
ai_vision_capture_request()  ──flag──▶    wakes; owns BUFFER_ADDRESS
   [draws NOTHING while waiting]          detect → crop → embed, ×3 retries
ai_vision_capture_wait()  ◀──flag──       sets DONE or FAIL; releases
   [safe to draw again]
```

The UI task stays fully responsive across that window — it keeps polling touch
and the USER1 button — but it must not *draw*. A USER1 press mid-capture is
therefore recorded as a pending cancel and honoured the moment the capture
completes, rather than transitioning home and redrawing into memory the NPU is
still writing. This is the invariant to preserve in any future change to the
capture states.

## 10. Power Saving (Session 12)

µT-Kernel's dispatcher calls the BSP's `low_pow()` from its idle path
(`dispatch.S`, label `l_dispatch_110`) whenever no task is runnable. The
vendored BSP ships `low_pow()` as an empty function, so through Session 11 the
Cortex-M55 spun at full clock whenever the system had nothing to do — which, in
an application whose five tasks are all periodic sleepers, is most of the time.

`low_pow()` now forwards to `ms_osal_low_power_idle()` in `ms_osal.c`, which
executes a race-free `WFI` and accumulates the cycles spent asleep via the DWT
cycle counter.

**The masking around that `WFI` is load bearing, and the first hardware flash
proved it.** The idle path runs with `BASEPRI = 0x10`, and SysTick's priority is
*also* `0x10`. A WFI wake-up event must be an exception that would preempt the
current execution priority — the Arm ARM excludes PRIMASK from that judgement
but **not** BASEPRI — so SysTick could not wake the core, and a plain `WFI`
there froze the whole system the instant it first had nothing to run. The hook
now does `PRIMASK = 1` (closing the check/sleep race), `BASEPRI = 0` (making
every enabled interrupt a valid wake-up event), `WFI`, then restores BASEPRI
*before* PRIMASK so the woken exception is still taken where the dispatcher
expects it. Do not "simplify" that sequence. The body lives on this project's side of the vendored-code
boundary — the same arrangement Session 11 used for `ms_osal_clean_dcache()` —
so the CMSIS dependency stays out of third-party code and the diff against
upstream mtk3_bsp2 is a single forwarding call (`THIRD_PARTY_SOFTWARE.md` §4,
row 8).

Constraints on anything added to that function, all of them hard:

- It runs in **handler mode (PendSV)** with `BASEPRI` masked. No `printf`
  (`session_11_notes.md` Addendum 8 documents what a slow dispatcher path costs
  on this board), no `tk_*` call, nothing unbounded.
- The measurement is read out from **task** context — `task_heartbeat_fn()`
  prints an idle-percentage figure every 10 s — never from the hook itself.
