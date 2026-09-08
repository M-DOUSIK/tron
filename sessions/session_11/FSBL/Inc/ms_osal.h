/* ms_osal.h — MedSight OS Abstraction Layer
 *
 * PUBLIC API — defined Session 07 (FreeRTOS backend), unchanged since.
 * Session 11 swapped ms_osal.c's internals to µT-Kernel 3.0 calls without
 * changing a single signature here — that was the entire point of staging
 * the project behind this header from Session 07 onward.
 *
 * CONTRACT:
 *   No application module (.c file) outside ms_osal.c may include RTOS
 *   headers (FreeRTOS or µT-Kernel's <tk/tkernel.h>) or call any RTOS API
 *   directly. Everything goes through the functions declared here.
 *
 * API surface matches SOFTWARE_ARCHITECTURE.md §4 exactly.
 */

#ifndef MS_OSAL_H
#define MS_OSAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>   /* size_t */

/* ── Opaque handle types ─────────────────────────────────────────────────── */
/* Declared as void* so the backend (FreeRTOS or µT-Kernel) can store
 * whatever pointer type it needs without leaking RTOS headers upward.       */

typedef void *osal_task_handle_t;
typedef void *osal_queue_handle_t;
typedef void *osal_mutex_handle_t;

/* Max length (incl. NUL) of the name passed to osal_task_create(), truncated
 * beyond this — sized for this backend's internal task-name storage. */
#define OSAL_TASK_NAME_MAX  16u

/* Sentinel: wait forever */
#define OSAL_WAIT_FOREVER   ( (uint32_t)0xFFFFFFFFu )
/* Sentinel: do not wait (non-blocking) */
#define OSAL_NO_WAIT        ( (uint32_t)0u )

/* ── Task entry function type ────────────────────────────────────────────── */
typedef void (*osal_task_fn_t)(void *arg);

/* ═══════════════════════════════════════════════════════════════════════════
 * Task Management
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Create an RTOS task and immediately make it ready to run.
 *
 * @param  fn           Task entry function — must never return.
 * @param  name         Human-readable task name (truncated to 15 chars).
 * @param  stack_words  Stack depth in 32-bit WORDS (not bytes).
 * @param  arg          Opaque argument passed to fn(arg).
 * @param  priority     Task priority, 1 = lowest, higher numbers = higher priority
 *                      (this backend maps the ordering onto µT-Kernel's own
 *                      priority scale internally — see ms_osal.c).
 *                      Suggested values:
 *                        1 = heartbeat / background
 *                        2 = logger
 *                        4 = UI / touch
 *                        5 = camera / ISP
 * @return Handle to the created task, or NULL on failure.
 */
osal_task_handle_t osal_task_create(osal_task_fn_t fn,
                                    const char    *name,
                                    uint32_t       stack_words,
                                    void          *arg,
                                    uint32_t       priority);

/**
 * @brief  Start the RTOS scheduler. Does NOT return on success.
 *         Call this at the end of main() after all tasks are created.
 */
void osal_scheduler_start(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * Queue (message-passing FIFO)
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Create a fixed-depth, fixed-item-size queue.
 *
 * @param  item_count  Maximum number of items the queue can hold.
 * @param  item_size   Size of each item in bytes.
 * @return Handle, or NULL on failure.
 */
osal_queue_handle_t osal_queue_create(uint32_t item_count,
                                      uint32_t item_size);

/**
 * @brief  Send one item to the back of a queue.
 *
 * @param  q           Queue handle returned by osal_queue_create().
 * @param  item        Pointer to the item to copy into the queue.
 * @param  timeout_ms  Max wait in ms. OSAL_NO_WAIT = non-blocking.
 *                     OSAL_WAIT_FOREVER = block until space available.
 * @return true if item was posted, false if timeout expired.
 */
bool osal_queue_send(osal_queue_handle_t q,
                     const void         *item,
                     uint32_t            timeout_ms);

/**
 * @brief  Receive one item from the front of a queue.
 *
 * @param  q           Queue handle.
 * @param  item_out    Buffer to copy the item into. Must be at least
 *                     item_size bytes (as given to osal_queue_create).
 * @param  timeout_ms  Max wait in ms. OSAL_WAIT_FOREVER = block indefinitely.
 * @return true if an item was received, false on timeout.
 */
bool osal_queue_receive(osal_queue_handle_t q,
                        void               *item_out,
                        uint32_t            timeout_ms);

/* ═══════════════════════════════════════════════════════════════════════════
 * Mutex
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Create a mutual exclusion mutex (non-recursive).
 * @return Handle, or NULL on failure.
 */
osal_mutex_handle_t osal_mutex_create(void);

/**
 * @brief  Acquire (lock) the mutex.
 *
 * @param  m           Mutex handle.
 * @param  timeout_ms  Max wait in ms. OSAL_WAIT_FOREVER = block until locked.
 * @return true if mutex was acquired, false on timeout.
 */
bool osal_mutex_lock(osal_mutex_handle_t m, uint32_t timeout_ms);

/**
 * @brief  Release (unlock) the mutex.
 *         Must be called by the same task that locked it.
 * @param  m  Mutex handle.
 */
void osal_mutex_unlock(osal_mutex_handle_t m);

/* ═══════════════════════════════════════════════════════════════════════════
 * Time
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Block the calling task for at least ms milliseconds.
 *         Other tasks run during the delay (cooperative/preemptive).
 *         Do NOT use HAL_Delay() from task context — use this instead.
 *
 * @param  ms  Delay duration. 0 yields the CPU for the remainder of
 *             the current tick (equivalent to taskYIELD).
 */
void osal_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* MS_OSAL_H */
