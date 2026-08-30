# Session 07 Notes — FreeRTOS + OSAL Integration

## Overview

Session 07 transitions MedSight from a bare-metal super-loop to a preemptive
multitasking environment using **FreeRTOS 10.x**, wrapped entirely behind the
**MedSight OS Abstraction Layer (OSAL)** defined in `ms_osal.h / ms_osal.c`.

The OSAL is the architectural key to Session 12: when the µT-Kernel 3.0 migration
happens, only `ms_osal.c` is rewritten. The rest of the application (main.c,
state_machine.c, sd_logger.c, anime_ui.c, etc.) changes **zero lines**.

---

## FreeRTOS Setup

| Item | Value |
|---|---|
| FreeRTOS version | 10.x (upstream, `FreeRTOS-Kernel`) |
| Port | `GCC/ARM_CM55_NTZ/non_secure` |
| Heap manager | `heap_4.c` (best-fit, coalescing) |
| Heap size | 64 KB (`configTOTAL_HEAP_SIZE`) |
| Tick rate | 1000 Hz (1 ms tick) |
| Max priorities | 8 |
| Stack overflow check | Method 2 (pattern fill, enabled) |

### FreeRTOS Source Tree Location

```
Middlewares/Third_Party/FreeRTOS/Source/
  tasks.c
  queue.c
  list.c
  stream_buffer.c
  timers.c
  event_groups.c
  include/           ← FreeRTOS.h, task.h, queue.h, semphr.h …
  portable/
    GCC/
      ARM_CM55_NTZ/non_secure/
        port.c
        portasm.c
        portmacro.h
    MemMang/
      heap_4.c
```

**To set up:** Download the FreeRTOS Kernel from
`https://github.com/FreeRTOS/FreeRTOS-Kernel/releases` and copy the `Source/`
directory to `Middlewares/Third_Party/FreeRTOS/Source/`.
Then refresh the STM32CubeIDE workspace (F5) — the linked resources in `.project`
will pick up the files automatically.

---

## Task Inventory

| Task name | Function | Priority | Stack (words) | Stack (bytes) | Period |
|---|---|---|---|---|---|
| `cam_isp` | `task_camera_isp_fn()` | **5** (highest) | 1024 | 4096 | 1 ms sleep between ISP calls |
| `ui` | `task_ui_fn()` | **4** | 512 | 2048 | 10 ms poll period |
| `logger` | `task_logger_fn()` | **2** | 1024 | 4096 | Blocks on queue (5s watchdog) |
| `heartbeat` | `task_heartbeat_fn()` | **1** (lowest) | 256 | 1024 | 500 ms LED toggle |

### Priority Rationale (deliberate — not arbitrary)

**Priority 5 — cam_isp (camera / ISP background)**
ISP_BackgroundProcess() must be called on every camera frame period (~30fps = 33ms).
If it's delayed by logging (SD writes can take 10–50ms), the ISP's internal
auto-exposure/white-balance statistics fall out of sync. Highest priority ensures
it is never starved by lower-priority work.

**Priority 4 — ui (touch polling + mascot animation)**
Touch responsiveness target: ≤20ms (perceptible threshold for button feel).
The UI task polls every 10ms, giving at most 10ms between a touch and recognition.
It must be above the logger task — an SD write must not delay a button tap response.

**Priority 2 — logger (SD writer)**
SD card writes using FATFS's polling mode can block for 10–50ms per write.
Placing the logger below UI ensures touch events are never delayed by logging.
Placing it above idle means it drains the queue promptly when the SD is idle.
The 16-slot queue absorbs burst log events without blocking the UI task.

**Priority 1 — heartbeat (liveness LED)**
Lowest priority. If the LED stops toggling, it means a higher-priority task has
deadlocked or is in an infinite busy-loop. Acts as a scheduler watchdog.

### Priority Inversion Risk Assessment

FreeRTOS uses non-recursive mutexes with priority inheritance disabled by default
(`configUSE_MUTEXES = 1`, no `configUSE_MUTEXES_PRIORITY_INHERITANCE`). The OSAL
mutex is only used by sd_logger for FATFS access. The only mutex holder is
`task_logger_fn()` (priority 2), which is never called from higher-priority tasks.
Therefore there is **no priority inversion risk** in this session. Session 11
will revisit this when state_machine calls SD through the async queue (no mutex
crossing required by the queue design).

---

## Inter-task Communication Design

### Log Queue

```
SD_Log_Event_Async("msg")          SD card
    │                                  ▲
    ▼                                  │
[osal_queue_send]  →→→→→→→→  task_logger_fn  →  SD_Log_Event()
    (non-blocking,               (blocking on queue,   (FATFS write)
     OSAL_NO_WAIT)               priority 2)
```

- Queue depth: **16 slots** × **128 bytes** = 2 KB
- If the queue is full (logger starved), messages are silently dropped.
  This is a deliberate design: logging failure should never crash the device.
  Stack overflow check (Method 2) and the heartbeat task will indicate starvation.

---

## HAL Tick Compatibility Under FreeRTOS

FreeRTOS owns SysTick for its tick. `ms_osal.c` provides `SysTick_Handler()`
which calls `xPortSysTickHandler()`. The port's handler in turn calls
`vApplicationTickHook()` (also in ms_osal.c), which calls `HAL_IncTick()`.

Result: `HAL_GetTick()` and `HAL_Delay()` both work correctly under FreeRTOS.
`HAL_Delay()` should only be used before the scheduler starts (in main() hardware
init). After the scheduler starts, application code must use `osal_delay_ms()`.

### stm32n6xx_it.c changes

The following handler bodies were **removed** from `stm32n6xx_it.c` (Session 07):
- `SVC_Handler()` — FreeRTOS port.c owns this via `#define vPortSVCHandler SVC_Handler`
- `PendSV_Handler()` — FreeRTOS port.c owns this via `#define xPortPendSVHandler PendSV_Handler`
- `SysTick_Handler()` — ms_osal.c owns this

Leaving empty bodies of these in stm32n6xx_it.c would cause **linker duplicate symbol errors**
or silently override the FreeRTOS port's handlers. The file now has comments explaining this.

---

## OSAL API — Software Architecture §4 Confirmation

The implemented API in `ms_osal.h` matches `SOFTWARE_ARCHITECTURE.md §4` exactly:

| OSAL call | ms_osal.h signature | FreeRTOS backend |
|---|---|---|
| `osal_task_create` | `osal_task_handle_t osal_task_create(fn, name, stack_words, arg, priority)` | `xTaskCreate` |
| `osal_queue_create` | `osal_queue_handle_t osal_queue_create(item_count, item_size)` | `xQueueCreate` |
| `osal_queue_send` | `bool osal_queue_send(q, item, timeout_ms)` | `xQueueSend` |
| `osal_queue_receive` | `bool osal_queue_receive(q, item_out, timeout_ms)` | `xQueueReceive` |
| `osal_mutex_create` | `osal_mutex_handle_t osal_mutex_create(void)` | `xSemaphoreCreateMutex` |
| `osal_mutex_lock` | `bool osal_mutex_lock(m, timeout_ms)` | `xSemaphoreTake` |
| `osal_mutex_unlock` | `void osal_mutex_unlock(m)` | `xSemaphoreGive` |
| `osal_delay_ms` | `void osal_delay_ms(uint32_t ms)` | `vTaskDelay` |

Additionally implemented (needed for Session 07):
- `osal_scheduler_start()` — `vTaskStartScheduler()`
- `OSAL_WAIT_FOREVER` / `OSAL_NO_WAIT` sentinels

---

## Files Changed This Session

| File | Change |
|---|---|
| `FSBL/Inc/FreeRTOSConfig.h` | **NEW** — FreeRTOS port configuration for STM32N6570 |
| `FSBL/Inc/ms_osal.h` | **NEW** — OSAL public API (matches SOFTWARE_ARCHITECTURE.md §4) |
| `FSBL/Src/ms_osal.c` | **NEW** — FreeRTOS backend (only file calling FreeRTOS API) |
| `FSBL/Inc/sd_logger.h` | **MODIFIED** — Added `ms_log_msg_t`, `SD_Logger_Queue_Init()`, `SD_Log_Event_Async()`, `task_logger_fn()` |
| `FSBL/Src/sd_logger.c` | **MODIFIED** — Added async queue layer on top of synchronous FATFS layer |
| `FSBL/Src/ui/state_machine.c` | **MODIFIED** — `HAL_Delay(200)` → `osal_delay_ms(200)`, log calls → `SD_Log_Event_Async()` |
| `FSBL/Src/main.c` | **MODIFIED** — Super-loop replaced by task creation + `osal_scheduler_start()` |
| `FSBL/Src/stm32n6xx_it.c` | **MODIFIED** — Removed SVC/PendSV/SysTick handler bodies (now owned by FreeRTOS port + ms_osal.c) |
| `STM32CubeIDE/FSBL/.project` | **MODIFIED** — Added FreeRTOS .c sources + ms_osal.c as linked resources |
| `STM32CubeIDE/FSBL/.cproject` | **MODIFIED** — Added FreeRTOS include paths |

---

## Known Gaps / Future Work

- `HAL_Delay()` is still used in `Error_Handler()` — this is acceptable because
  `Error_Handler()` never returns and the LED-blink loop is intentional.
- FatFs `ffsystem.c` mutex stubs are empty (bare-metal default). If the logger
  task ever races with another task calling FATFS directly (shouldn't happen —
  `sd_logger.c` is the only FATFS user), implement the `ff_mutex_take/give`
  stubs using the OSAL mutex. Not needed this session.
- Stack high-water mark logging: add `uxTaskGetStackHighWaterMark()` calls to
  the heartbeat task for debugging if stack overflows are suspected.

---

## Validation Checklist

```
Build check:
  grep -rn "osThreadNew\|osDelay\|xTaskCreate\|vTaskDelay\|xQueueCreate\|xSemaphoreCreate" \
       FSBL/Src/ --include="*.c" | grep -v ms_osal.c
  → Must return ZERO matches

Manual hardware tests:
  [ ] Build with 0 errors, 0 new warnings
  [ ] LED_RED blinks at 1Hz (heartbeat task running)
  [ ] LED_GREEN toggles rapidly (cam_isp task running)
  [ ] Touch REGISTER → screen transitions immediately (≤100ms felt response)
  [ ] Touch DISPENSE → screen transitions immediately
  [ ] Mascot breathes/blinks at ~4 FPS
  [ ] SD card (after boot): events.log contains "System Boot" entries
  [ ] SD card (after touching REGISTER): events.log contains STATE transitions
```
