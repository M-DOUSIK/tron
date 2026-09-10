# SOFTWARE_ARCHITECTURE.md — MedSight

## 1. Firmware Lifecycle Strategy

Bare-metal (Sessions 01–06) → FreeRTOS behind an OS Abstraction Layer, OSAL
(Sessions 07–10) → µT-Kernel 3.0 swapped in behind the same OSAL (Session 11,
done and hardware-verified — see `milestones/session_11_notes.md`) →
µT-Kernel-idiomatic integration, power saving and hardening (Session 12, done —
see `milestones/session_12_notes.md`) → UI/UX overhaul (Session 13, done) →
physical dispensing hardware (Session 17) and carer mode + scheduled dosing +
the memory map + the submission materials (Session 15). **Sessions 15 and 17 are
independent and may be run in either order**; each records at the top of its
notes which folder it was based on, so the chain is reconstructable whichever
way round they went. **15 is the last session in the plan** — see
`MASTER_PROJECT_PLAN.md`'s Changelog for the renumbering history.

**Physical dispensing was cut in v8 of the plan and un-cut in v11**, when a
teammate able to build the hardware joined. Session 17 interfaces one hopper: a
28BYJ-48 stepper turntable and an IR break-beam that counts pills as they drop.
The 6–8 hopper architecture in `MECHANICAL_DESIGN.md` remains design intent, and
the simulated dispense path is kept working behind a build switch so the hardware
stays cuttable. **The no-networking rule is unaffected and permanent.**

Where this document still describes the software-only simulated dispense flow, it
is describing what a build without `MEDSIGHT_PHYSICAL_DISPENSER` does — which is
the build Session 15 was developed against, since Session 17 had not run at the
time it was written.

The OSAL is the whole point of this staging: FreeRTOS lets you build and debug quickly
with a mature, well-documented API, while guaranteeing the final TRON-mandated swap to
µT-Kernel 3.0 only touches one file (`ms_osal.c`), not the whole application.

## 2. Folder Structure

**Corrected in Session 12 against the files that actually exist.** Earlier
revisions of this tree listed four modules that were designed and never
written — `camera_lcd.c`, `interactive_gui.c`, `schedule_time_source.c` and
`patient_profile.h` — and used a `tron/` root that was never the real path.
Where a planned module was absorbed elsewhere, this now says where.

```
sessions/
  session_01/ ... session_15/      -- one full project snapshot per completed
                                       session; the rollback trail
FSBL/
  Inc/
  Src/
    main.c                          -- clock/peripheral bring-up, task creation.
                                        Also holds the camera + LTDC init that an
                                        early plan put in a camera_lcd.c that was
                                        never written.
    ms_osal.c / ms_osal.h          -- OS Abstraction Layer (Session 07+)
    ui/
      anime_ui.c/.h                 -- Session 04 (mascot rendering; idle state only,
                                        by decision — see §5)
      touch_driver.c/.h             -- Session 05 (GT911 touch controller, I2C2, PD14/PD4)
      gui_draw.c/.h                 -- Session 05+ (screen drawing primitives, the
                                        elderly-friendly palette and every full-screen
                                        layout; Session 05's interactive_gui.c was
                                        folded into this plus state_machine.c and no
                                        longer exists as a file)
      registration_ui.c/.h          -- Session 09 (enrollment flow: face capture hand-off,
                                        on-screen keyboard, pill count, confirm)
      carer_ui.c/.h                 -- Session 15. Two things, deliberately together:
                                        the ONE passcode prompt and the ONE
                                        validation routine shared by carer-mode
                                        entry and by REGISTER PATIENT (§7), plus
                                        carer mode's own screens - set clock,
                                        per-patient schedule and dose, log review,
                                        delete patient, change passcode.
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
    schedule_time_source.c/.h       -- Session 15 (WRITTEN AT LAST). Listed here
                                        as "designed, not yet written" from
                                        Session 10 until Session 15, because
                                        nothing needed the time of day. Backed by
                                        the STM32N6's internal RTC, with a
                                        compressed-day demo mode behind the same
                                        interface (see §11).
    ms_memtest.c/.h                 -- Session 15. Pattern-tests the AI_ARENA
                                        linker region from a cold boot and
                                        reports the byte count. See
                                        MEMORY_MAP.md §3.
    dispenser.c/.h                  -- Session 17 (stepper turntable + IR pill
                                        counter; closed-loop count)
    state_machine.c/.h              -- Session 10 (dispense-flow orchestration —
                                        simulated dispense only, see §7);
                                        Session 12 added the error/alert states and
                                        made the capture states non-blocking (§9)
```

The patient record lives in `ai_vision.h`, not in a separate
`patient_profile.h` — an early plan named that file and it was never created,
because the gallery and the record are owned by the same module. See §6.

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
- No module outside `schedule_time_source.c` reads the RTC directly —
  `state_machine.c` and `carer_ui.c` ask "what minute of the day is it" and "how
  long until HH:MM" through this module's API only. **This is now real rather than
  planned**: Session 15 built both backends behind that interface, and a build
  switch (`MEDSIGHT_FAST_CLOCK`) chooses between a wall clock and a day compressed
  into four minutes without a single conditional anywhere above this module. The
  substitution point was the deliverable, not the timer.
- `ai_overlay.c` (Session 16) reads `ai_vision.h`'s capture view and
  `ai/intake.h`'s live view directly, and draws through `gui_draw.h`. It is a
  render-only module with no state beyond a refresh throttle, called from
  `state_machine.c` — so it does not breach the rule that `state_machine`
  orchestrates. It touches no OS primitive and no hardware register, and it
  deliberately avoids DMA2D so that it adds no bus master and therefore no new
  `LPEN` obligation (§10).
- `carer_ui.c` is a second documented exception of the same shape as
  `registration_ui.c`'s (above): it calls `ai_vision.h`'s
  `gallery_set_schedule()` / `gallery_set_dose()` / `gallery_delete_patient()`
  directly, and `sd_logger.h`'s file API for the passcode store and the log
  review. Carer mode is a self-contained flow over the gallery, exactly as
  registration is, and routing every edit through `state_machine.c` would have
  made that file the owner of six screens' worth of form state for no benefit.

## 4. OSAL API Surface (defined Session 07, remapped Session 11)

Minimum surface needed, mapped to both backends:

| OSAL call | FreeRTOS backend (historical) | µT-Kernel 3.0 backend (current) |
|---|---|---|
| `osal_task_create` | `osThreadNew` | `tk_cre_tsk` / `tk_sta_tsk` |
| `osal_queue_create` / `send` / `receive` | FreeRTOS queue API | `tk_cre_mbf` / `tk_snd_mbf` / `tk_rcv_mbf` |
| `osal_mutex_create` / lock / unlock | FreeRTOS mutex API | `tk_cre_mtx` / `tk_loc_mtx` / `tk_unl_mtx` — **first real consumer in Session 15**, see below |
| `osal_delay_ms` | `osDelay` | `tk_dly_tsk` |
| `osal_flag_create` / `set` / `clear` / `wait` **(Session 12)** | — (never existed) | `tk_cre_flg` / `tk_set_flg` / `tk_clr_flg` / `tk_wai_flg` with `TWF_ANDW`/`TWF_ORW`/`TWF_BITCLR` |
| `osal_alarm_create` / `start` / `stop` **(Session 15)** | — (never existed) | `tk_cre_alm` / `tk_sta_alm` / `tk_stp_alm` |
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

**The mutex primitive got its first consumer in Session 15, eight sessions
after it was defined.** Session 07 declared four primitives and
`session_12_notes.md` records that nothing in the codebase ever locked
anything — there was only ever one writer for each piece of shared state.
Session 15 broke that: reading the RTC calendar is a *pair* of HAL calls
(`HAL_RTC_GetTime()` locks the shadow registers, `HAL_RTC_GetDate()` unlocks
them), and the logger task (priority 2) now stamps every log line while the UI
task (priority 4) reads the clock to draw it and to schedule. The UI preempts
the logger by construction, so the interleaving is the normal case rather than
an unlucky one. `schedule_time_source.c` serialises the pair.

**Session 15 widened the API a second time, once, for the same kind of
reason.**
Alarm handlers (`osal_alarm_*` → `tk_cre_alm`/`tk_sta_alm`/`tk_stp_alm`) have a
genuine consumer that nothing already in this API expresses: a dose window is a
one-shot deadline at an absolute time of day, three or four times a day. A task
polling the clock every ten seconds would wake 8,640 times a day to act four
times, spending the ~89% idle figure Session 12 measured in order to do it;
`osal_delay_ms()` until the next dose blocks a whole task on nothing; an event
flag has no notion of time at all. An alarm costs nothing until it fires.

It is also the mechanism the **original Program Plan named** — "The Camera Task
wakes either on a scheduled µT-Kernel alarm (aligned to dose times) or on user
button press" — unimplemented from March until Session 15. See
`PROGRAM_PLAN_RECONCILIATION.md` §2.

The constraint that shapes its use: an alarm handler runs in **handler context**,
so it may not block, may not `printf`, and may not call anything unbounded. The
only correct shape is "set an event flag and return", and
`schedule_alarm_handler()` in `state_machine.c` is three lines long for exactly
that reason. Every piece of real work — reading the gallery, drawing, logging —
is done by the UI task. That is the same division of labour Session 12
established between the UI and the AI task, applied to time instead of inference.

`prompts/session_12.md` is explicit that a forced idiom reads worse to an expert
judge than an absent one; `milestones/session_12_notes.md` records each of these
decisions with its evidence, and `session_15_notes.md` does the same for the
alarm.

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

## 5. Mascot State Enum (introduced Session 05; IDLE and ERROR are live)

```c
typedef enum {
    MASCOT_IDLE,
    MASCOT_ACTIVE,
    MASCOT_SUCCESS,
    MASCOT_ERROR
} mascot_state_t;
```

**Two of the four are real.** `MASCOT_IDLE` is the 8-frame idle loop
(214×213, home and instruction screens). `MASCOT_ERROR` is the designer's
three-frame crying pose (150×153), animated on the FACE NOT RECOGNISED
screen; `state_machine.c` selects it on entry to `STATE_FACE_RETRY` and
resets to `MASCOT_IDLE` on all three exits from that screen.

`MASCOT_ACTIVE` and `MASCOT_SUCCESS` still render the idle loop — no artwork
exists for them yet. Sessions 12 and 13 originally recorded "idle-only" as a
closed design decision; the project owner reopened it in Session 13 and
`MASCOT_ERROR` was built. The remaining two are unbuilt, not forbidden.

**Session 15 selects `MASCOT_ACTIVE` when a dose window opens**, and
`MASCOT_ERROR` when one closes unserved. The error state animates for real; the
active state currently renders the idle loop, so **the visible reminder is the
home screen's banner, not the mascot**. Recorded here rather than left to be
discovered, because "the mascot goes to MASCOT_ACTIVE" is otherwise a sentence
that sounds like something happens on screen. New artwork is the only thing
missing, and Session 15 deliberately did not add it — out of scope, and it is
the designer's work rather than the firmware's.

The one hard constraint on any new state is memory, not policy: mascot frames
are CPU-drawn into `BUFFER_ADDRESS`, which the NPU also uses as activation
scratch, so nothing may animate while a capture is in flight. See
`MASCOT_UI_DESIGN.md` §4 and §6.

## 6. Patient Profile Data Model (Session 09, as actually implemented)

```c
/* ai_vision.h — the real, shipped struct, as of Session 15 (file format v3). */
#define EMBEDDING_SIZE    128
#define MAX_PATIENTS      10
#define PATIENT_NAME_MAX  32
#define MAX_DOSE_TIMES     4

typedef struct {
    uint8_t   valid;
    char      name[PATIENT_NAME_MAX];
    int8_t    embedding[EMBEDDING_SIZE];   // from ai_vision.c, Session 08B
    uint8_t   pill_count;                  // pills per DOSE - fixed, never decremented
    uint8_t   dose_time_count;             // Session 15: 0..MAX_DOSE_TIMES
    uint16_t  dose_time[MAX_DOSE_TIMES];   // Session 15: minute-of-day, ascending
} PatientRecord;
```

**The schedule (Session 15).** Times are minute-of-day (0..1439), the one axis
the whole scheduling path uses — small enough to store four of per patient in a
record written to an SD card in full on every edit, with no timezone and no DST,
and exactly the granularity a prescription is written in. They are kept sorted
ascending, and the invariant is established in `gallery_set_schedule()`, the only
function that writes them, rather than assumed by every reader.

`MAX_DOSE_TIMES` is 4 because that is what real prescriptions use — once, twice,
three or four times a day. `dose_time_count == 0` is a legitimate state, not an
error: that patient can still walk up and tap DISPENSE, the device simply never
reminds them and never records a missed dose for them.

**`pill_count` is the dose (corrected in Session 12).** It is how many pills
this patient takes in one sitting: a fixed property of their prescription, set
once at registration, unchanged by dispensing. A patient registered for 3 pills
gets 3 pills every time.

Sessions 10-12 carried a second field, `pills_remaining`, initialised to
`pill_count` and decremented **by one** per confirmed dose. That was wrong
twice over - it treated a per-dose quantity as a stock level, then drew that
"stock" down one pill at a time regardless of the dose size - and the visible
symptom was the dispense screen announcing "3 pills" while the number fell
3 -> 2 -> 1. It is gone, along with the "refill needed" alerts built on it.

There is **no stock counter in the data model at all**, deliberately. The
firmware has no way to know when a carer tops the hopper up, so any software
count would drift from reality immediately. Real hopper-level knowledge arrives
in Session 17, where the IR break-beam counter measures pills physically
dropping - a short dispense after a full actuator cycle *is* an empty hopper,
measured rather than assumed.

**On-card format.** `patients.dat` is a small versioned header
(`GalleryFileHeader`: magic, format version, record size, record count)
followed by `MAX_PATIENTS` records. Before Session 12 the raw array was written
with no header and accepted only if the file length happened to match, so "file
from older firmware", "truncated file", "file from another device" and "no
file" were indistinguishable and all silently produced an empty gallery. The
header makes a mismatch say which one it is - which mattered immediately, since
dropping `pills_remaining` changed the record size from 163 to 162 bytes and
invalidated every existing card.

**Session 15 bumped the format to v3.** Adding the schedule changed the record
size again, so every v2 card is stale - and this time the header did its job
without anyone having to think about it: a v2 card is reported by name and
version and rejected, rather than silently loading as an empty gallery. Carers
re-register once. The README says so, which is the other half of the feature.

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

**Session 15 changed the way into registration, and added a way in that is not a
button at all.** Both are recorded before the diagram so nobody reads the old
happy path as current:

```
STATE_HOME
  -> user taps "Register Patient"
STATE_PASSWORD  (carer passcode; ui/carer_ui.c)     <- NEW, Session 15 B2a
  -> correct   -> STATE_INSTRUCT_REGISTER (the Session 09 flow, unchanged)
  -> cancelled -> STATE_HOME
  -> wrong x5  -> refuses for 30 s, still STATE_HOME on cancel

STATE_HOME
  -> five taps on the TITLE BAR within 3 s          <- NEW, Session 15 B2
STATE_PASSWORD
  -> correct -> STATE_CARER_MENU -> { STATE_CARER_CLOCK,
                                      STATE_CARER_PATIENTS -> STATE_CARER_PATIENT
                                          -> { STATE_CARER_SCHEDULE,
                                               STATE_CARER_DOSE,
                                               STATE_CARER_DELETE_CONFIRM },
                                      STATE_CARER_LOG,
                                      STATE_CARER_CHANGE_CODE }
```

Every carer state is reachable **only** through `STATE_PASSWORD`, and
`STATE_PASSWORD` is reachable from exactly those two places. There is no third
way in and no shortcut between them. The gate is at the *start* of registration
rather than the end, deliberately — see `COMPLIANCE_PRIVACY_POSTURE.md` §6.

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
STATE_CONFIRM_TAKEN -> (Session 16) camera streams to PSRAM; the AI task
     watches for a pill going to the mouth while this screen stays drawn.
     The verdict NEVER gates the button - it only chooses the log suffix.
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

**Session 15 additions — the scheduled-dose path.** This is not a state; it is a
condition the home screen and the log respond to, driven by a µT-Kernel alarm
(§4). It is written out here because it is the first thing in this device that
happens without anybody touching it:

```
alarm fires (window OPEN)  -> handler sets SCHED_FLAG_OPEN and returns
  UI task, next tick       -> home screen redraws with a banner naming the
                              patient and the time; mascot -> MASCOT_ACTIVE;
                              "SCHEDULE: dose due HH:MM for <name>" to the log
                           -> the SAME alarm is re-armed for the window's close
  patient taps DISPENSE, matches, confirms, inside the window
                           -> "CONFIRMED: <name> took the HH:MM dose (on time)"
                              rather than the generic confirmation line
alarm fires (window CLOSE) -> handler sets SCHED_FLAG_CLOSE and returns
  UI task, next tick       -> if nobody dispensed: "MISSED: <name> did not take
                              the HH:MM dose"; mascot -> MASCOT_ERROR
                           -> the alarm is re-armed for the NEXT dose
```

One alarm object serves both edges, one pending expiry at a time, and nothing
anywhere polls the clock. The window is `DOSE_WINDOW_MINUTES` (30) *schedule*
minutes, so in demo mode it compresses along with everything else.

The `MISSED:` line is the single most valuable entry in the audit trail and the
device has never been able to write it before.

Edge cases (handled explicitly, not as afterthoughts), each driving `mascot_state_t`
above to `MASCOT_ERROR` unless noted:
- Missed dose (the RTC schedule window closes with no Dispense Medicine tap) →
  `MASCOT_ERROR` + SD log, remain `STATE_HOME`. **Built in Session 15**; this line
  described intent from Session 10 until then.
- Unrecognized/no face at the face-check step → `MASCOT_ERROR`, local alert (LCD +
  on-screen only — there is no buzzer and no audio in this project, see
  `MASTER_PROJECT_PLAN.md` §7), SD log, no dispense,
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

**Session 16 added no task either**, and for a stronger reason than Session
15's. Action recognition runs on the **existing** `ai` task at priority 3,
woken by a fourth bit (`AI_FLAG_INTAKE`) on the **same** event flag the
capture handshake already uses. A second AI task would have made the
frame-buffer ownership rule below a race instead of an invariant, because
there would no longer be one thing that owns `BUFFER_ADDRESS` at a time. The
event flag earns its keep again here: the AI task must block on "a capture
request **or** an intake request" in one call, which is what an OR-wait
expresses and what a queue or semaphore cannot.

**Session 15 added no task**, and that was a decision rather than an oversight.
Scheduled dosing looks at first like it wants one, but its work is an alarm
handler (which is not a task) plus a few lines of reaction in the UI task, which
is already running every 10 ms and already owns every screen the reaction
touches. A sixth task would have needed its own stack and its own claim on the
framebuffer for no behaviour that is not already there. `OSAL_MAX_TASKS` is 8 and
five are used, so the room exists — it just is not needed.

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

### The other half of the WFI: `ms_configure_sleep_clocks()`

`WFI` on this part enters CSleep, which stops the CPU **and the clock of every
peripheral, bus and memory whose `LPEN` bit is clear**. That is not a detail of
the idle hook; it is a change to the operating conditions of every DMA master
in the system, and it must be configured before the scheduler ever runs.

Session 12 shipped the `WFI` without it and broke the display for six rounds of
diagnosis. The framebuffer is at `0x34200000` (AXISRAM3-6), so every idle tick
cost the LTDC either its own clock, the AXI bus matrix clock, or the RAM it was
reading. The panel starved and greyed out while every register the CPU could
read said the display was healthy — because the CPU only reads when it is
awake. `session_12_notes.md` Addendum 9 is the full account.

`ms_configure_sleep_clocks()` in `main.c` sets the `LPEN` bits for exactly the
masters that move data while the CPU sleeps:

| Register | Bits | What it protects |
|---|---|---|
| `BUSLPENR` | `ACLKN`, `ACLKNC` | the AXI bus matrix — without it no master reaches memory at all |
| `MEMLPENR` | `AXISRAM3-6` | the framebuffer and the NPU activation pools sharing it |
| `APB5LPENR` | `LTDC`, `DCMIPP`, `CSI` | display, and the camera DMAing into the same buffer during preview |
| `AHB5LPENR` | `DMA2D`, `SDMMC2`, `NPU` | mascot blitter, audit-log writes, inference |

The set is deliberately wider than the display. Each entry is a master that
moves data with no CPU involvement during a window when every task is blocked
and the CPU is therefore asleep — a face capture, an SD write, a mascot redraw.
The display was simply the one failure visible to the naked eye.

**Rule for future sessions:** anything that adds a new DMA-driven peripheral
must add its `LPEN` bit here in the same change, and must be tested from a
**cold boot** — the only condition under which the original fault appeared.

**Session 16 needed TWO new bits, and they are in `ms_configure_sleep_clocks()`
in the same change as the code that made them necessary.** Action recognition
points the DCMIPP at external PSRAM (`0x90400000`, XSPI1) so the camera can
run while the UI keeps drawing the confirm screen — the first new DMA
destination since Session 12, and outside AXISRAM3–6, so this section's rule
applies in full:

| Register | Bit | What it protects |
|---|---|---|
| `AHB5LPENR` | `XSPI1LPEN` | the PSRAM controller |
| `AHB5LPENR` | `XSPIMLPEN` | the **XSPI manager** — every memory-mapped access routes through it, so gating it stalls the transfer just as completely |

The second is the one easy to miss. **Not yet verified from a cold boot** —
that is the test that matters and it is listed in `session_16_notes.md`.

Also found while checking, and recorded because it is the same fault shape:
`XSPI2LPEN` is clear and the NPU reads its weights from OSPI NOR during
inference. It has never bitten because the ST runtime is built
`LL_ATON_OSAL_BARE_METAL` and **polls**, so the CPU is awake for the whole run.
Latent, not live — but it stops being latent the moment inference blocks on an
OS primitive instead of spinning.

**Session 15 checked this rather than assuming it, and needed no new bit.** The
new `AI_ARENA` region is at `0x34388000`, inside AXISRAM6, which these bits
already ungate. Two things found while checking, both now in
`documents/MEMORY_MAP.md`:

- **AXISRAM5 and AXISRAM6 are not powered at all until `npu_init.c`'s
  `SystemInit_POST()` runs**, on the AI task, after the scheduler starts;
  `stm32n6xx_hal_msp.c` brings up only AXISRAM3 and AXISRAM4. Anything placed in
  the upper two banks must not be touched before `ai_vision_wait_init()` returns.
- **AXISRAM1 and AXISRAM2 — where all the code, `.rodata` and `.bss` live — are
  not in this function's set at all.** Harmless today because no DMA master reads
  from them (the SD path is polling, so the CPU is awake throughout). It becomes
  a live bug the moment anything DMAs to or from a `.bss` buffer.

## 11. Memory Map (Session 15)

The full region table, the NPU-reachability finding, the `AI_ARENA` region and a
runbook for adding a third model now live in **`documents/MEMORY_MAP.md`**, which
is the authority. The two facts most likely to be needed from elsewhere:

- The linker's `ROM` region was re-derived in Session 15 from 511 KB (inherited
  unexamined from the ST example this repo was founded on, and **90.6% full** in
  Debug) to 1024 KB — all of AXISRAM2. Debug now sits at 47.3%.
- The two NPU networks' activations occupy one contiguous block,
  `0x34200000`–`0x34387FFF`. `BUFFER_ADDRESS` is inside it on purpose; Session
  13's `GUI_BUFFER_ADDRESS` was also inside it, which is why the second
  framebuffer was reverted.
