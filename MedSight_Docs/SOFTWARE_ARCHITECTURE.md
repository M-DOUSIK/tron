# SOFTWARE_ARCHITECTURE.md — MedSight

## 1. Firmware Lifecycle Strategy

Bare-metal (Sessions 01–06) → FreeRTOS behind an OS Abstraction Layer, OSAL
(Sessions 07–10) → µT-Kernel 3.0 swapped in behind the same OSAL (Session 11,
done and hardware-verified — see `milestones/session_11_notes.md`) →
µT-Kernel-idiomatic integration, power saving and hardening (Session 12) →
final polish/demo packaging (Session 13). Session 13 is the last planned session —
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
      interactive_gui.c/.h          -- Session 05 (Register / Dispense Medicine buttons,
                                        MASCOT_STATE management)
      registration_ui.c/.h          -- Session 09 (enrollment flow: photo, name, phone,
                                        medicine/quantity/time selection)
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
                                        simulated dispense only, see §7)
  Inc/
    patient_profile.h               -- Session 09 (shared patient-record struct)
docs/                               -- this documentation set, kept current
```

## 3. Module Boundary Rules

- `camera_lcd`, `anime_ui`, `touch_driver`, `interactive_gui`, `registration_ui`,
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
- No module outside `ai_vision.c` touches the NPU/STM32Cube.AI runtime.
- No module outside `sd_logger.c` touches FATFS/SDMMC directly — everything else logs
  or reads profile data through its API.
- No module outside `touch_driver.c` touches the touch controller's I2C2 bus directly —
  `interactive_gui.c` and `registration_ui.c` consume touch events through
  `touch_driver.h`'s API only.
- No module outside `schedule_time_source.c` reads the fast-timer/RTC directly —
  `state_machine.c` asks "what's due now" through this module's API only, which is
  what makes the prototype-timer/real-RTC swap a single-file change later.

## 4. OSAL API Surface (defined Session 07, remapped Session 11)

Minimum surface needed, mapped to both backends:

| OSAL call | FreeRTOS backend | µT-Kernel 3.0 backend |
|---|---|---|
| `osal_task_create` | `osThreadNew` | `tk_cre_tsk` / `tk_sta_tsk` |
| `osal_queue_create` / `osal_queue_send` | FreeRTOS queue API | `tk_cre_mbf` / `tk_snd_mbf` |
| `osal_mutex_create` / lock / unlock | FreeRTOS mutex API | µT-Kernel semaphore/mutex API |
| `osal_delay_ms` | `osDelay` | `tk_dly_tsk` |

**Session 12 widens this surface deliberately.** The four primitives above were
chosen as the lowest common denominator between the two backends, precisely so
the Session 11 swap would be mechanical. That worked — but it also means the
finished firmware uses µT-Kernel only through primitives every RTOS has, which
scores weakly against TRON Contest rule 1.4's "high degree of relevance to
µT-Kernel 3.0". Session 12 adds an event-flag primitive
(`osal_flag_create`/`set`/`clear`/`wait`, backed by `tk_cre_flg`/`tk_set_flg`/
`tk_wai_flg` with `TWF_ANDW`/`TWF_ORW`) for the genuine multi-condition waits in
the dispense flow, and evaluates a fixed-size memory pool (`tk_cre_mpf`) against
real allocation sites. There is no FreeRTOS column for these — FreeRTOS was
removed in Session 11 — so the table above is now a historical record of the
migration mapping, not a live two-backend contract. See `prompts/session_12.md`
for the scope and the explicit instruction to skip any idiom that has no real
consumer rather than force it.

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

`interactive_gui.c` and `registration_ui.c` own state transitions triggered by touch;
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
