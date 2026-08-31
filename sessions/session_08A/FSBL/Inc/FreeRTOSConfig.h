/*
 * FreeRTOSConfig.h — MedSight Session 07
 *
 * Target: STM32N657X0HxQ, Cortex-M55 @ 800 MHz
 * Port:   GCC / ARM_CM55_NTZ / non_secure
 * FreeRTOS Kernel: V11.3.1
 *
 * Rules:
 *  - No application code should include FreeRTOS headers directly.
 *    All OS access goes through ms_osal.h. This header is only included
 *    by FreeRTOS internals and ms_osal.c.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ── Hardware & Core ──────────────────────────────────────────────────────── */

/* Cortex-M55: __NVIC_PRIO_BITS = 4, so 16 priority levels (0–15).
 * FreeRTOS requires that configMAX_SYSCALL_INTERRUPT_PRIORITY is never
 * exceeded by any ISR that calls FreeRTOS API.                            */
#define configCPU_CLOCK_HZ                  ( 800000000UL )
#define configTICK_RATE_HZ                  ( 1000 )        /* 1 ms tick    */

/* NVIC: Cortex-M55 implements 4 priority bits → 16 levels.
 * FreeRTOS uses the *lowest numerical value* = highest priority rule.
 * We give FreeRTOS syscall ceiling at priority 5 (decimal) which maps to
 * 0x50 in the BASEPRI register (top nibble shifted).                      */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY     5
#define configKERNEL_INTERRUPT_PRIORITY     \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - 4) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - 4) )

/* ── Scheduling ───────────────────────────────────────────────────────────── */
#define configUSE_PREEMPTION                1
#define configUSE_TIME_SLICING              1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0   /* Generic mode — simpler */
#define configMAX_PRIORITIES                8
#define configMINIMAL_STACK_SIZE            256     /* words (not bytes)      */
#define configMAX_TASK_NAME_LEN             16

/* ── V11 ARM_CM55_NTZ Port — Required Defines ────────────────────────────── */
/* FreeRTOS V11 replaced configUSE_16_BIT_TICKS with this macro.              */
#define configTICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS

/* Cortex-M55 has MVE (Helium) — disable because compiler flag is -mfpu=fpv5-d16 */
#define configENABLE_MVE                 0

/* FPU is present on Cortex-M55 (FPv5-D16, hard-float ABI) — must be 1.     */
#define configENABLE_FPU                 1

/* MPU: not used in the FSBL (bare secure-only) — set to 0.                  */
#define configENABLE_MPU                 0

/* TrustZone: FSBL runs fully secure, no NS world — set to 0.               */
#define configENABLE_TRUSTZONE           0

/* Required when TrustZone is disabled — tells the port we are secure-only.  */
#define configRUN_FREERTOS_SECURE_ONLY   1

/* ── Memory ───────────────────────────────────────────────────────────────── */
/* 64 KB heap placed in AXI SRAM via linker (heap_4.c).
 * AXI SRAM starts at 0x34000000 on STM32N6; the framebuffer uses the lower
 * portion (BUFFER_ADDRESS = 0x34200000), so the heap is safe above 0x34280000.
 * The actual placement is controlled by the linker script section .heap.    */
#define configTOTAL_HEAP_SIZE               ( 64 * 1024 )

/* Static allocation: disabled — we use dynamic (heap_4) exclusively.
 * Session 12 can switch to static if µT-Kernel migration needs it.         */
#define configSUPPORT_STATIC_ALLOCATION     0
#define configSUPPORT_DYNAMIC_ALLOCATION    1

/* ── Hook Functions ───────────────────────────────────────────────────────── */
/* vApplicationTickHook: called every tick ISR — we use it to call
 * HAL_IncTick() so HAL_GetTick() / HAL_Delay() still work under FreeRTOS. */
#define configUSE_TICK_HOOK                 1
#define configUSE_IDLE_HOOK                 0
#define configUSE_MALLOC_FAILED_HOOK        1
#define configCHECK_FOR_STACK_OVERFLOW      2   /* Method 2 — pattern check */
#define configUSE_DAEMON_TASK_STARTUP_HOOK  0

/* ── Optional Features ────────────────────────────────────────────────────── */
#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         0
#define configUSE_COUNTING_SEMAPHORES       0
#define configUSE_QUEUE_SETS                0
#define configUSE_TASK_NOTIFICATIONS        1
#define configUSE_TIMERS                    0   /* Not needed yet — Session 11 will reconsider */
#define configUSE_STREAM_BUFFERS            1   /* V11 requires explicit opt-in; stream_buffer.c is in the build */
#define configUSE_CO_ROUTINES               0
#define configUSE_TRACE_FACILITY            0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configGENERATE_RUN_TIME_STATS       0
#define configUSE_TASK_FPU_SUPPORT          1   /* Cortex-M55 has FPU (FPv5-D16) */

/* ── API Inclusion (only what ms_osal.c uses) ─────────────────────────────── */
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_xTaskGetCurrentTaskHandle   1   /* Required by stream_buffer.c in V11 */
#define INCLUDE_uxTaskGetStackHighWaterMark 1   /* useful for stack-size debug */
#define INCLUDE_xQueueGetMutexHolder        0
#define INCLUDE_xSemaphoreGetMutexHolder    0
#define INCLUDE_eTaskGetState               0
#define INCLUDE_xTimerPendFunctionCall      0

/* ── ARM Cortex-M Port Specifics ──────────────────────────────────────────── */
/* Required by the ARM_CM55_NTZ port — do not remove.                         */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif

/* ── FreeRTOS V11 Handler Names ───────────────────────────────────────────── */
/* In FreeRTOS V10, the port used macro aliases to map its internal handler
 * names to the CMSIS-standard names:
 *   #define vPortSVCHandler    SVC_Handler
 *   #define xPortPendSVHandler PendSV_Handler
 *
 * In FreeRTOS V11 (ARM_CM55_NTZ/portasm.c), the handlers are already
 * defined using their CMSIS-standard names directly:
 *   void SVC_Handler(void)   — portasm.c line 522/560
 *   void PendSV_Handler(void) — portasm.c line 289/418
 *   void SysTick_Handler(void) — port.c
 *
 * DO NOT add any #define aliases here. Doing so will cause the preprocessor
 * to mangle portasm.c's inline assembly labels and break the SVC-based
 * context-switch, causing an immediate Hard Fault on scheduler start.
 *
 * stm32n6xx_it.c must NOT define SVC_Handler, PendSV_Handler, or
 * SysTick_Handler — all three are owned by portasm.c / port.c.           */

/* ── Assert ───────────────────────────────────────────────────────────────── */
extern void Error_Handler(void);
#define configASSERT( x ) if( ( x ) == 0 ) { Error_Handler(); }

#endif /* FREERTOS_CONFIG_H */
