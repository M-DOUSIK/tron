# Session 07 — OSAL + FreeRTOS Multitasking

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Transition from the bare-metal super-loop to a multitasking environment using FreeRTOS,
wrapped entirely behind an OS Abstraction Layer (OSAL), so the eventual µT-Kernel 3.0
migration in Session 11 only requires rewriting one file.

BACKGROUND AND CONTEXT
Building on tron/session_06/. We need concurrent tasks for camera/UI (including the
touch-driven interactive GUI from Session 05), logging, and (soon) AI and mechanical
control. Per SOFTWARE_ARCHITECTURE.md §4, define a minimal OSAL API surface now and use
it everywhere going forward.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ (new ms_osal.c/.h)
- SOFTWARE_ARCHITECTURE.md §4 (OSAL API surface table — implement exactly this set)
- tron/session_06/ as the base

REQUIRED INPUTS
- None beyond the existing modules (anime_ui, touch_driver,
  interactive_gui, sd_logger) to be moved into tasks.

EXPECTED OUTPUTS
- ms_osal.c/.h implementing: task creation, mutex create/lock/unlock, queue
  create/send/receive, and millisecond delay — all backed by FreeRTOS underneath.
- Camera/preview, touch polling + interactive GUI, mascot animation, and SD logging
  each running as separate RTOS tasks, communicating via OSAL queues rather than shared
  globals where they need to exchange data.

CONSTRAINTS
- NO direct calls to osThreadNew, osDelay, or any other FreeRTOS API anywhere outside
  ms_osal.c. Enforce this — it's the entire point of this session.
- Preserve all existing functionality (camera preview, touch input, interactive
  buttons, mascot state animation, SD logging) — this session is a structural change,
  not a feature change.

CODING STANDARDS
- ms_osal.h's public API must exactly match the table in SOFTWARE_ARCHITECTURE.md §4 so
  the Session 11 migration has a fixed contract to satisfy.

FOLDER STRUCTURE TO FOLLOW
- FSBL/Src/ms_osal.c
- FSBL/Inc/ms_osal.h

FILES TO CREATE
- FSBL/Src/ms_osal.c
- FSBL/Inc/ms_osal.h

FILES TO MODIFY
- FSBL/Src/main.c (now just creates tasks via OSAL and starts the scheduler)
- FSBL/Src/main.c, anime_ui.c, touch_driver.c, interactive_gui.c, sd_logger.c (wherever
  they currently do blocking work, convert to RTOS task bodies communicating via OSAL
  queues)

DOCUMENTATION TO UPDATE
- docs/milestones/session_07_notes.md: task list, priorities, and stack sizes chosen
  (note: touch polling likely needs a tighter period than SD logging — document the
  priority scheme deliberately).
- Confirm SOFTWARE_ARCHITECTURE.md §4's OSAL table matches what was actually
  implemented (update the doc if reality diverged).

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly with FreeRTOS included.
- (Manual) All Session 01-06 features (LED, UART log, camera preview, touch input,
  interactive buttons, mascot states, SD logging) still work correctly, now running
  concurrently under the scheduler.

COMPLETION CHECKLIST
- [ ] ms_osal.c/.h implements the full API surface from SOFTWARE_ARCHITECTURE.md §4
- [ ] Camera, touch/GUI, animation, and logging run as separate tasks
- [ ] Main loop replaced by RTOS scheduler start
- [ ] No direct FreeRTOS API calls outside ms_osal.c (verified by search)
- [ ] session_07_notes.md written with task/priority/stack details

COMMON PITFALLS
- Stack overflows from converting a bare-metal loop's local variables into task-local
  stack usage without checking stack sizes.
- Priority inversion between the camera task (timing-sensitive), touch-polling task
  (needs to feel responsive), and logging task (can tolerate more latency) — pick
  priorities deliberately, don't leave them all equal.
- Touch responsiveness regressing once it's behind a task scheduler instead of a tight
  bare-metal poll loop — verify button-tap responsiveness still feels immediate.

DEFINITION OF DONE
All prior features work concurrently under FreeRTOS via the OSAL, including touch
responsiveness, and a search of the codebase confirms zero direct FreeRTOS API usage
outside ms_osal.c.

SELF-REVIEW BEFORE DECLARING COMPLETE
Grep for "osThreadNew", "osDelay", "xQueue", etc. across the whole FSBL/Src tree —
confirm hits exist only inside ms_osal.c. Re-test touch responsiveness specifically.
```

## Expected Deliverables
`ms_osal.c/.h`, refactored camera/touch/GUI/logging modules as RTOS tasks, session
notes.

## Manual Verification Steps
1. Flash and observe: LED, UART log, camera preview, touch input + button taps, mascot
   state animation, and SD logging all still functioning.
2. Specifically re-test touch responsiveness — confirm no perceptible lag introduced
   by task scheduling.
3. Let it run for several minutes checking for any new instability (task
   starvation, stack overflow resets) that wasn't present in Session 06.

## Acceptance Criteria
All previously-working features function identically under the new task-based
architecture, with no regressions or new instability, including touch responsiveness.

## Next Prompt
Copy to `tron/session_07/`, proceed to `session_08A.md`.
