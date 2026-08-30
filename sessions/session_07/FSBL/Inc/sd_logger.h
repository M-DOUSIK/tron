#ifndef __SD_LOGGER_H
#define __SD_LOGGER_H

#include <stdint.h>
#include <stdbool.h>

/* ── Log message structure used by the async queue ──────────────────────── */
/* Keep this compact: the queue stores copies of this struct.               */
#define SD_LOG_MSG_MAX_LEN  128

typedef struct {
    char text[SD_LOG_MSG_MAX_LEN];
} ms_log_msg_t;

/* ════════════════════════════════════════════════════════════════════════════
 * Synchronous API
 * Caller blocks until the SD operation completes.
 * Use ONLY from main() (before scheduler starts) or from task_logger itself.
 * ════════════════════════════════════════════════════════════════════════════ */

/** @brief Mount FATFS and open the log infrastructure. Call before scheduler. */
bool SD_Logger_Init(void);

/** @brief Append a text event line to events.log. Blocking. */
bool SD_Log_Event(const char* event_string);

/** @brief Write a binary blob to dummy_face.bin. Blocking. */
bool SD_Log_Binary(const uint8_t* data, uint32_t length);

/* ════════════════════════════════════════════════════════════════════════════
 * Asynchronous API (Session 07+)
 * Non-blocking: copies the message into the log queue and returns.
 * task_logger drains the queue and writes to SD in its own context.
 * Safe to call from any task or interrupt context that is at or below
 * configMAX_SYSCALL_INTERRUPT_PRIORITY.
 * ════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Initialise the inter-task log queue.
 *         Call once from main() before creating tasks.
 */
void SD_Logger_Queue_Init(void);

/**
 * @brief  Post a log message to the async queue.
 *         Non-blocking (OSAL_NO_WAIT). If the queue is full the message is
 *         silently dropped — log queue overflow is a diagnostic symptom, not
 *         a fatal error (the device keeps running).
 * @param  event_string  Null-terminated string ≤ SD_LOG_MSG_MAX_LEN-1 chars.
 */
void SD_Log_Event_Async(const char* event_string);

/**
 * @brief  Task body for the logger task.
 *         Blocks on the queue and calls SD_Log_Event() for each message.
 *         Never returns — create as an RTOS task via osal_task_create().
 * @param  arg  Unused (NULL).
 */
void task_logger_fn(void *arg);

#endif /* __SD_LOGGER_H */
