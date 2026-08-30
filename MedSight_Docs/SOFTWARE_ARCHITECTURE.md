# SOFTWARE_ARCHITECTURE.md — MedSight

## 1. Firmware Lifecycle Strategy

Bare-metal (Sessions 01–06) → FreeRTOS behind an OS Abstraction Layer, OSAL
(Sessions 07–11) → µT-Kernel 3.0 swapped in behind the same OSAL (Session 12) →
hardening/polish (Sessions 13–14) → optional stretch (Session 15).

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
    ai_vision.c/.h                  -- Sessions 08A-C (toolchain proof, one-shot face
                                        recognition + gallery matching, action/
                                        consumption recognition)
    dispenser.c/.h                  -- Session 10 (multi-hopper stepper motor control +
                                        per-hopper IR sensors)
    schedule_time_source.c/.h       -- Session 11 (swappable fast-timer/RTC abstraction)
    state_machine.c/.h              -- Session 11 (dispense-flow orchestration)
  Inc/
    patient_profile.h               -- Session 09 (shared patient-record struct)
docs/                               -- this documentation set, kept current
```

## 3. Module Boundary Rules

- `camera_lcd`, `anime_ui`, `touch_driver`, `interactive_gui`, `registration_ui`,
  `sd_logger`, `ai_vision`, `dispenser` never call OS primitives directly — only
  through `ms_osal.h`. This is what makes the Session 12 migration mechanical rather
  than a rewrite.
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
- No module outside `dispenser.c` issues raw motor-driver/GPIO calls — everything
  else calls the single abstracted `dispense_dose(hopper_id, count)` entry point (see
  `MECHANICAL_DESIGN.md` for the abstraction rationale).
- No module outside `schedule_time_source.c` reads the fast-timer/RTC directly —
  `state_machine.c` asks "what's due now" through this module's API only, which is
  what makes the prototype-timer/real-RTC swap a single-file change later.

## 4. OSAL API Surface (defined Session 07, remapped Session 12)

Minimum surface needed, mapped to both backends:

| OSAL call | FreeRTOS backend | µT-Kernel 3.0 backend |
|---|---|---|
| `osal_task_create` | `osThreadNew` | `tk_cre_tsk` / `tk_sta_tsk` |
| `osal_queue_create` / `osal_queue_send` | FreeRTOS queue API | `tk_cre_mbf` / `tk_snd_mbf` |
| `osal_mutex_create` / lock / unlock | FreeRTOS mutex API | µT-Kernel semaphore/mutex API |
| `osal_delay_ms` | `osDelay` | `tk_dly_tsk` |

No application code calls `osThreadNew`, `osDelay`, etc. directly — enforce this in code
review during every session from 07 onward. See `ENGINEERING_LESSONS.md` for the
`mtk3bsp2_samples` reference used to validate correct µT-Kernel API usage in Session 12.

## 5. Mascot State Enum (introduced Session 05, driven by real events from Session 11)

```c
typedef enum {
    MASCOT_IDLE,
    MASCOT_ACTIVE,
    MASCOT_SUCCESS,
    MASCOT_ERROR
} mascot_state_t;
```

`interactive_gui.c` and `registration_ui.c` own state transitions triggered by touch;
`state_machine.c` (from Session 11 onward) owns state transitions triggered by system
events (face-match results, action-recognition results, dispense/jam results). All
funnel through the same OSAL queue into `anime_ui.c`'s renderer — `anime_ui.c` itself
never decides *why* the state changed, only *how* to render it.

## 6. Patient Profile Data Model (Session 09)

```c
typedef struct {
    uint32_t patient_id;
    uint8_t  face_embedding[EMBEDDING_SIZE];   // from ai_vision.c, Session 08B
    char     name[NAME_MAX_LEN];
    char     phone_number[PHONE_MAX_LEN];      // stored, never transmitted — see
                                                 // COMPLIANCE_PRIVACY_POSTURE.md §4
    medicine_schedule_entry_t schedule[MAX_MEDICINES_PER_PATIENT];
    uint8_t  schedule_count;
} patient_profile_t;

typedef struct {
    uint8_t hopper_id;      // maps to a physical hopper, wired in Session 10
    uint8_t quantity;
    // time representation intentionally left to Session 09/11's schedule_time_source
    // design — don't hardcode a format here ahead of that decision
} medicine_schedule_entry_t;
```

Defined once in `patient_profile.h` so `sd_logger.c` (storage), `ai_vision.c`
(embedding shape), `registration_ui.c` (population), and `state_machine.c` (Session 11,
consumption) all agree on its shape without duplicating the definition.

## 7. State Machine (Session 11)

```
IDLE (camera OFF, main screen shown)
  -> user taps "Dispense Medicine"
IDLE -> CAMERA_ON_FACE_CHECK
  -> match against enrolled gallery (Session 08B/09 data)
CAMERA_ON_FACE_CHECK -> INTRUDER_ALERT (no match)      -> local alert, log, camera OFF, IDLE
CAMERA_ON_FACE_CHECK -> DISPENSING (match found)
DISPENSING -> AWAITING_CONSUMPTION_CONFIRMATION
  -> requires BOTH action-recognition detection AND a manual Confirm tap
AWAITING_CONSUMPTION_CONFIRMATION -> SUCCESS   (both satisfied within timeout)
AWAITING_CONSUMPTION_CONFIRMATION -> MISSED_CONFIRMATION (timeout)
SUCCESS / INTRUDER_ALERT / MISSED_CONFIRMATION -> camera OFF, log, IDLE
```

Edge cases (handled explicitly, not as afterthoughts), each driving `mascot_state_t`
above to `MASCOT_ERROR` unless noted:
- Missed dose (fast-timer/RTC schedule window elapses with no Dispense Medicine tap) →
  `MASCOT_ERROR` + SD log, remain IDLE.
- Unrecognized/no face at the face-check step → `MASCOT_ERROR`, local alert (LCD +
  buzzer, not a phone push — see `MASTER_PROJECT_PLAN.md` §7), SD log, no dispense,
  camera OFF, return to IDLE.
- Possible jam on a specific hopper (IR beam never confirms the expected count) → see
  `MECHANICAL_DESIGN.md` §7; scoped to that hopper only, `MASCOT_ERROR`, log and
  surface to UI rather than silently retrying or halting other hoppers in the same
  dose event.
- Missed consumption confirmation (action recognition and/or the manual Confirm tap
  don't both complete within a timeout) → `MASCOT_ERROR` + SD log, camera OFF, IDLE.
- Successful cycle → `MASCOT_SUCCESS`, SD log, camera OFF, then back to `MASCOT_IDLE`.

## 8. Pin Map

| Peripheral | Pins/Bus | Session assigned | Notes |
|---|---|---|---|
| Debug UART | USART1, via ST-LINK VCP | Session 02 | |
| Camera / LCD | DCMIPP + LTDC (RGB888 parallel bus across PA, PB, PD, PE, PG, PH), PSRAM framebuffers | Session 03 | Based on jpcano/STM32N6-digits reference. Hardware ISP configured in camera_lcd.c. |
| Touch controller (GT911) | I2C2 — PD14 (SCL), PD4 (SDA), FSBL context in `.ioc` | Session 05 | Confirmed via real prior bring-up on this board, not guessed — see `ENGINEERING_LESSONS.md` |
| Per-hopper stepper (28BYJ-48) | 4 GPIO per hopper to its ULN2003 board, repeated per hopper | Session 10 | See `HARDWARE_ARCHITECTURE.md` §2 for the GPIO-expander note once hopper count grows past ~3 |
| Per-hopper IR break-beam | 1 EXTI input per hopper | Session 10 | Not shared across hoppers |

This table stays a living document — record actual pin assignments here as each
session finalizes them, don't write speculative pin numbers ahead of the hardware work.
