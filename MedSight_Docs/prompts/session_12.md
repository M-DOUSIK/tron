# Session 12 — µT-Kernel 3.0 Migration (TRON Compliance Gate)

*This session is the actual contest requirement. Everything before it was legitimate
architecture; this is where it becomes a valid TRON Programming Contest submission.
Protect the time budget for this session per MASTER_PROJECT_PLAN.md §3.*

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Replace the FreeRTOS backend behind the OSAL (ms_osal.c) with µT-Kernel 3.0, so the
final firmware is TRON-contest-compliant, while every module above the OSAL stays
completely unchanged.

BACKGROUND AND CONTEXT
Building on tron/session_11/ — the full system works end-to-end on FreeRTOS. Because
every application module (camera/UI, AI, dispenser, logging, state machine) talks only
through ms_osal.h (enforced since Session 07), this migration should only require
rewriting ms_osal.c's internals, not the application logic.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ms_osal.c (the ONLY file this session should need to substantially rewrite)
- The µT-Kernel 3.0 BSP for STM32 (mtk3_bsp2 / mtkernel_3) — integrate as the new RTOS
  dependency, replacing FreeRTOS in the build.
- Reference sample projects: https://github.com/tron-forum/mtk3bsp2_samples — use
  these as the source of truth for correct tk_cre_tsk/tk_cre_mbf/tk_snd_mbf usage
  patterns on STM32 rather than inferring API usage from documentation text alone.
- SOFTWARE_ARCHITECTURE.md §4 (the OSAL-to-backend mapping table — implement the
  µT-Kernel column exactly)
- ENGINEERING_LESSONS.md (the .ioc/Makefile/workspace-refresh rules apply here too —
  BSP integration is a hardware-adjacent build-system change)
- tron/session_11/ as the base

REQUIRED INPUTS
- µT-Kernel 3.0 BSP source integrated into the project (as a library/subfolder,
  following whatever structure the BSP's own documentation specifies for STM32Cube
  integration), cross-checked against the mtk3bsp2_samples reference projects above.

EXPECTED OUTPUTS
- ms_osal.c reimplemented entirely on µT-Kernel 3.0 primitives per
  SOFTWARE_ARCHITECTURE.md §4:
  - osal_task_create -> tk_cre_tsk / tk_sta_tsk
  - osal_queue_create / osal_queue_send -> tk_cre_mbf / tk_snd_mbf
  - osal_mutex_create/lock/unlock -> µT-Kernel semaphore/mutex equivalents
  - osal_delay_ms -> tk_dly_tsk
- FreeRTOS completely removed from the build (dependency, include paths, linked
  library all gone).
- Identical application behavior to Session 10 — this is a backend swap, not a
  feature change.

CONSTRAINTS
- Do not modify FSBL/Src/main.c, anime_ui.c, ai_vision.c, dispenser.c, sd_logger.c, or
  state_machine.c — if the OSAL boundary from Session 07 was done correctly, none of
  these need to change. If you find yourself needing to touch one of them, stop and
  flag it — that indicates a boundary violation from an earlier session that needs to
  be understood before proceeding, not silently patched over.
- ms_osal.h's public function signatures must stay identical — only ms_osal.c's
  internals change.

FILES TO MODIFY
- FSBL/Src/ms_osal.c (full internal rewrite)
- Build configuration (remove FreeRTOS, add µT-Kernel 3.0 BSP)

FILES NOT TO MODIFY (unless a genuine boundary violation is found and documented)
- main.c (FSBL main loop), anime_ui.c/.h, ai_vision.c/.h, dispenser.c/.h, sd_logger.c/.h,
  state_machine.c/.h

DOCUMENTATION TO UPDATE
- docs/milestones/session_10_notes.md: any friction points in the FreeRTOS→µT-Kernel
  mapping, any semantic differences that required care (e.g. message buffer vs. queue
  semantics), confirmation FreeRTOS was fully removed.
- Update SOFTWARE_ARCHITECTURE.md §4 if any mapping differed from the original table.

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly with zero FreeRTOS references remaining.
- (Manual) Run the FULL hardware test suite from Session 10 (normal cycle + all three
  edge cases) again, unchanged, and confirm identical behavior to the FreeRTOS
  version.

COMPLETION CHECKLIST
- [ ] µT-Kernel 3.0 BSP integrated into the build
- [ ] ms_osal.c fully reimplemented on µT-Kernel primitives per the mapping table
- [ ] FreeRTOS fully removed (grep-verified: zero remaining references)
- [ ] All application modules unchanged (diff-verified against session_10)
- [ ] Full Session 10 test suite re-run and passing identically
- [ ] session_10_notes.md written

COMMON PITFALLS
- µT-Kernel message buffers (tk_snd_mbf/tk_rcv_mbf) have different blocking/sizing
  semantics than FreeRTOS queues — verify behavior under the same load patterns used
  in Session 10, don't assume a 1:1 semantic match.
- Priority number direction/range differences between FreeRTOS and µT-Kernel task
  priorities — re-verify the task priority scheme from Session 07 still produces the
  same relative ordering.
- Stack size units or requirements differing between the two kernels.

DEFINITION OF DONE
Firmware builds with zero FreeRTOS dependency, runs entirely on µT-Kernel 3.0, and
reproduces every behavior validated in Session 10's full test suite — normal cycle and
all three edge cases — identically on physical hardware. This is the TRON contest
compliance milestone.

SELF-REVIEW BEFORE DECLARING COMPLETE
Diff the entire application-layer file set (everything except ms_osal.c and build
config) against tron/session_11/ and confirm zero differences. Grep the whole project
for "FreeRTOS", "osThreadNew", "xQueue" etc. and confirm zero remaining references.
```

## Expected Deliverables
µT-Kernel 3.0-backed `ms_osal.c`, FreeRTOS fully removed, identical application
behavior, session notes.

## Manual Verification Steps
1. Build and flash.
2. Re-run the complete Session 10 test suite (normal cycle + missed dose + wrong
   patient + jam) and confirm identical results.
3. Let it run for an extended soak period (longer than any prior session) to catch
   any timing differences the two kernels might expose.

## Acceptance Criteria
Byte-for-byte identical application code (outside ms_osal.c), zero FreeRTOS
references, full test suite passing on µT-Kernel 3.0.

## Next Prompt
Copy to `tron/session_12/`, proceed to `session_13.md`. **This is your contest-
compliant baseline — treat this snapshot as sacred; all further work should be
regression-tested against it.**
