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

/** @brief Write a binary blob to an arbitrary named file (overwrite). Blocking.
 *  Session 08B: used by ai_vision.c's gallery_save() for patients.dat — kept
 *  here (not in ai_vision.c) per SOFTWARE_ARCHITECTURE.md's rule that only
 *  sd_logger.c touches FATFS/SDMMC directly. */
bool SD_Write_File(const char* filename, const uint8_t* data, uint32_t length);

/** @brief Read a named file fully into out_data (capacity max_length). Blocking.
 *  Returns false if the file does not exist, can't be opened, or is larger
 *  than max_length. On success, *out_bytes_read is the file size. */
bool SD_Read_File(const char* filename, uint8_t* out_data, uint32_t max_length,
                   uint32_t* out_bytes_read);

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
