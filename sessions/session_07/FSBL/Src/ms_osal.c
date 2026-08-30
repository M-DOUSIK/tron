/* ms_osal.c — MedSight OS Abstraction Layer (FreeRTOS backend)
 *
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║  THIS IS THE ONLY FILE IN THE PROJECT THAT MAY CALL FreeRTOS API.      ║
 * ║  Session 12: replace internals with µT-Kernel 3.0 calls.               ║
 * ║  The public API in ms_osal.h must not change between sessions.         ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 *
 * FreeRTOS port: GCC / ARM_CM55_NTZ / non_secure
 * Heap:          heap_4.c (best-fit with coalescing)
 *
 * HAL tick compatibility:
 *   FreeRTOS owns SysTick. vApplicationTickHook() calls HAL_IncTick() so
 *   HAL_GetTick() and HAL_Delay() still work correctly at all times.
 *
 * SysTick handler routing:
 *   The FreeRTOS port provides xPortSysTickHandler(). We provide
 *   SysTick_Handler() here (as a weak override) that calls the port handler
 *   AND then calls HAL_IncTick() via the tick hook.
 */

#include "ms_osal.h"

/* ── FreeRTOS includes — only allowed in this file ───────────────────────── */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* ── HAL for HAL_IncTick ─────────────────────────────────────────────────── */
#include "stm32n6xx_hal.h"

#include <stddef.h>
#include <stdio.h>

/* ════════════════════════════════════════════════════════════════════════════
 * ms_to_ticks — helper: convert milliseconds to FreeRTOS tick count
 * ════════════════════════════════════════════════════════════════════════════ */
static inline TickType_t ms_to_ticks(uint32_t ms)
{
    if (ms == OSAL_WAIT_FOREVER) {
        return portMAX_DELAY;
    }
    return (TickType_t)(ms / portTICK_PERIOD_MS);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Task Management
 * ════════════════════════════════════════════════════════════════════════════ */
osal_task_handle_t osal_task_create(osal_task_fn_t fn,
                                    const char    *name,
                                    uint32_t       stack_words,
                                    void          *arg,
                                    uint32_t       priority)
{
    TaskHandle_t handle = NULL;
    BaseType_t ret = xTaskCreate(
        (TaskFunction_t)fn,
        name,
        (configSTACK_DEPTH_TYPE)stack_words,
        arg,
        (UBaseType_t)priority,
        &handle
    );
    if (ret != pdPASS) {
        /* Task creation failed — typically out of heap.
         * configASSERT triggers Error_Handler() on failure. */
        configASSERT(ret == pdPASS);
        return NULL;
    }
    return (osal_task_handle_t)handle;
}

void osal_scheduler_start(void)
{
    vTaskStartScheduler();
    /* Should never reach here. If it does, heap is too small. */
    configASSERT(0);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Queue
 * ════════════════════════════════════════════════════════════════════════════ */
osal_queue_handle_t osal_queue_create(uint32_t item_count,
                                      uint32_t item_size)
{
    QueueHandle_t q = xQueueCreate((UBaseType_t)item_count,
                                   (UBaseType_t)item_size);
    configASSERT(q != NULL);
    return (osal_queue_handle_t)q;
}

bool osal_queue_send(osal_queue_handle_t q,
                     const void         *item,
                     uint32_t            timeout_ms)
{
    return (xQueueSend((QueueHandle_t)q, item, ms_to_ticks(timeout_ms))
            == pdTRUE);
}

bool osal_queue_receive(osal_queue_handle_t q,
                        void               *item_out,
                        uint32_t            timeout_ms)
{
    return (xQueueReceive((QueueHandle_t)q, item_out, ms_to_ticks(timeout_ms))
            == pdTRUE);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Mutex
 * ════════════════════════════════════════════════════════════════════════════ */
osal_mutex_handle_t osal_mutex_create(void)
{
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    configASSERT(m != NULL);
    return (osal_mutex_handle_t)m;
}

bool osal_mutex_lock(osal_mutex_handle_t m, uint32_t timeout_ms)
{
    return (xSemaphoreTake((SemaphoreHandle_t)m, ms_to_ticks(timeout_ms))
            == pdTRUE);
}

void osal_mutex_unlock(osal_mutex_handle_t m)
{
    xSemaphoreGive((SemaphoreHandle_t)m);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Time
 * ════════════════════════════════════════════════════════════════════════════ */
void osal_delay_ms(uint32_t ms)
{
    if (ms == 0u) {
        taskYIELD();
    } else {
        vTaskDelay(ms_to_ticks(ms));
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * FreeRTOS Hook Implementations
 * ════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Tick hook — called every 1 ms by the FreeRTOS tick ISR.
 *         Calls HAL_IncTick() so HAL_GetTick() / HAL_Delay() still work.
 */
void vApplicationTickHook(void)
{
    /* Nothing — HAL_IncTick() is now handled directly by SysTick_Handler. */
}

/**
 * @brief  Idle hook — not used, but required if configUSE_IDLE_HOOK == 1.
 *         configUSE_IDLE_HOOK is 0 in FreeRTOSConfig.h so this is never called.
 */
void vApplicationIdleHook(void)
{
    /* Nothing */
}

/**
 * @brief  malloc-failed hook — called when heap is exhausted.
 *         Triggers the global error handler.
 */
void vApplicationMallocFailedHook(void)
{
    printf("FATAL: FreeRTOS heap exhausted!\r\n");
    configASSERT(0);
}

/**
 * @brief  Stack overflow hook — called when stack overflow is detected.
 *         Method 2 (pattern check) is enabled in FreeRTOSConfig.h.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("FATAL: Stack overflow in task: %s\r\n", pcTaskName);
    configASSERT(0);
}

/* ════════════════════════════════════════════════════════════════════════════
 * SysTick Handler
 *
 * We modified FreeRTOS port.c to rename its handler to xPortSysTickHandler.
 * This allows us to define SysTick_Handler here. We must only call FreeRTOS's
 * tick handler IF the scheduler is actually running, otherwise FreeRTOS crashes.
 * ════════════════════════════════════════════════════════════════════════════ */
void SysTick_Handler(void)
{
    /* Always increment the HAL tick for HAL_Delay() to work */
    HAL_IncTick();

    /* Only increment FreeRTOS tick if the scheduler has been started */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        extern void xPortSysTickHandler(void);
        xPortSysTickHandler();
    }
}
