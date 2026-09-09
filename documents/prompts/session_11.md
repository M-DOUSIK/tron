# Session 11 - uT-Kernel 3.0 OSAL Migration

## Context (for Antigravity to read)
Sessions 09-10 are complete. The full dispense flow works end-to-end in software.
The project has used `ms_osal.h` as an abstraction layer throughout — all tasks,
delays, semaphores, and message buffers go through OSAL wrappers, never direct FreeRTOS.

**Hardware decision (FINAL):** No physical motors/servos/IR. Software-only prototype.

## Prompt for Antigravity (copy-paste as-is)
```
OBJECTIVE
Swap the OSAL backend from FreeRTOS to μT-Kernel 3.0 by rewriting ms_osal.c only.
The rest of the codebase must NOT need any changes — that's the whole point of the
abstraction layer.

BACKGROUND AND CONTEXT
The project uses ms_osal.h everywhere. The FreeRTOS implementation is in ms_osal.c.
Reference: https://github.com/tron-forum/mtk3bsp2_samples for correct μT-Kernel BSP2
API usage patterns on STM32.

IMPLEMENTATION STEPS
1. Add the μT-Kernel 3.0 source tree to Middlewares/Third_Party/uTKernel3/.
   Use the STM32N6 BSP2 port if available, else the Cortex-M55 generic port.
2. Rewrite ms_osal.c:
   - osal_task_create()  → tk_cre_tsk() + tk_sta_tsk()
   - osal_delay_ms()     → tk_dly_tsk()
   - osal_sem_create()   → tk_cre_sem()
   - osal_sem_wait()     → tk_wai_sem()
   - osal_sem_signal()   → tk_sig_sem()
   - osal_msgbuf_*       → tk_cre_mbf() / tk_snd_mbf() / tk_rcv_mbf()
3. Update FatFS OS locking in Middlewares/Third_Party/FatFs/src/ffsystem.c
   to use μT-Kernel mutexes instead of FreeRTOS mutexes.
4. Remove FreeRTOS from the build (Middlewares/Third_Party/FreeRTOS/).
   Update subdir.mk accordingly.
5. Build and verify the system boots, displays the home screen, runs the camera,
   and completes a full dispense cycle on μT-Kernel.

CONSTRAINTS
- No .ioc file changes.
- ms_osal.h API signature must remain IDENTICAL — zero changes to callers.
- No face embeddings in UART logs.
- Confirm FreeRTOS is fully removed from the binary (check map file).
```
