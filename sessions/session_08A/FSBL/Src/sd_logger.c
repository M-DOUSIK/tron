/* sd_logger.c — MedSight SD card logging (Session 07: OSAL async queue)
 *
 * Two-layer design:
 *   Layer 1 (synchronous): SD_Log_Event(), SD_Log_Binary() — direct FATFS
 *     writes. Called only from main() before scheduler, or from task_logger.
 *   Layer 2 (async queue): SD_Log_Event_Async() → posts to ms_log_queue →
 *     task_logger_fn() drains and calls Layer 1.
 *
 * No FreeRTOS API is called here. All RTOS operations go through ms_osal.h.
 */

#include "sd_logger.h"
#include "ms_osal.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

/* ── FATFS state ─────────────────────────────────────────────────────────── */
static FATFS  SDFatFs;
static bool   is_mounted = false;

/* ── Async queue handle ──────────────────────────────────────────────────── */
/* Created by SD_Logger_Queue_Init() before the scheduler starts.            */
#define LOG_QUEUE_DEPTH  16   /* 16 slots × 128 bytes = 2 KB               */
static osal_queue_handle_t s_log_queue = NULL;

/* ════════════════════════════════════════════════════════════════════════════
 * Synchronous API (Layer 1)
 * ════════════════════════════════════════════════════════════════════════════ */

bool SD_Logger_Init(void)
{
    printf("SD_Logger_Init: Mounting SD card...\r\n");
    FRESULT res = f_mount(&SDFatFs, "", 1);
    if (res == FR_OK) {
        is_mounted = true;
        printf("SD_Logger_Init: SD card mounted OK.\r\n");
        return true;
    }
    printf("SD_Logger_Init: f_mount failed, error %d\r\n", res);
    return false;
}

bool SD_Log_Event(const char *event_string)
{
    if (!is_mounted) {
        printf("SD_Log_Event: SD not mounted.\r\n");
        return false;
    }

    FIL file;
    FRESULT res = f_open(&file, "events.log", FA_OPEN_APPEND | FA_WRITE);
    if (res != FR_OK) {
        printf("SD_Log_Event: f_open failed %d\r\n", res);
        return false;
    }

    UINT bw;
    f_write(&file, event_string, strlen(event_string), &bw);
    f_write(&file, "\n", 1, &bw);
    f_close(&file);

    printf("SD_Log_Event: logged: %s\r\n", event_string);
    return true;
}

bool SD_Log_Binary(const uint8_t *data, uint32_t length)
{
    if (!is_mounted) {
        printf("SD_Log_Binary: SD not mounted.\r\n");
        return false;
    }

    FIL file;
    /* NOTE: this session uses FA_CREATE_ALWAYS (overwrite) for the dummy blob.
     * Session 08C will change this to FA_OPEN_APPEND when real embeddings land. */
    FRESULT res = f_open(&file, "dummy_face.bin", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        printf("SD_Log_Binary: f_open failed %d\r\n", res);
        return false;
    }

    UINT bw;
    f_write(&file, data, length, &bw);
    f_close(&file);

    printf("SD_Log_Binary: %lu bytes written.\r\n", length);
    return (bw == length);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Async API (Layer 2) — backed by OSAL queue
 * ════════════════════════════════════════════════════════════════════════════ */

void SD_Logger_Queue_Init(void)
{
    s_log_queue = osal_queue_create(LOG_QUEUE_DEPTH, sizeof(ms_log_msg_t));
    /* configASSERT inside osal_queue_create handles NULL — no need to re-check */
}

void SD_Log_Event_Async(const char *event_string)
{
    if (s_log_queue == NULL) {
        /* Queue not initialised — scheduler not started yet.
         * Fall back to synchronous write. */
        SD_Log_Event(event_string);
        return;
    }

    ms_log_msg_t msg;
    /* Safe truncating copy — always NUL-terminates */
    size_t len = strlen(event_string);
    if (len >= SD_LOG_MSG_MAX_LEN) {
        len = SD_LOG_MSG_MAX_LEN - 1u;
    }
    memcpy(msg.text, event_string, len);
    msg.text[len] = '\0';

    /* Non-blocking send — drop silently if queue is full.
     * Log-queue overflow means the logger task is starved, which is a
     * priority-tuning issue, not a fatal error. */
    (void)osal_queue_send(s_log_queue, &msg, OSAL_NO_WAIT);
}

/**
 * @brief  Logger task body — blocks on the queue and writes to SD.
 *
 * Priority: 2 (below UI/camera, above idle).
 * Stack:    1024 words — FATFS local buffers are significant.
 */
void task_logger_fn(void *arg)
{
    (void)arg;
    ms_log_msg_t msg;

    printf("task_logger: started.\r\n");

    /* Session 07: Mount the SD card asynchronously here (after scheduler starts)
     * because HAL_SD_Init/FATFS requires SysTick to be running for timeouts.
     * SysTick is inactive during main() in FreeRTOS V11. */
    if (SD_Logger_Init()) {
        SD_Log_Event("System Boot - Session 07 SD Logger initialized in task");
        uint8_t dummy_face[16] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
        SD_Log_Binary(dummy_face, sizeof(dummy_face));
    } else {
        printf("task_logger: SD mount failed.\r\n");
    }

    for (;;) {
        /* Block until a log message arrives (or wake every 5s as a watchdog) */
        if (osal_queue_receive(s_log_queue, &msg, 5000u)) {
            /* If SD init failed previously, we might just drop it or try again.
             * For now, SD_Log_Event handles unmounted state by doing nothing. */
            SD_Log_Event(msg.text);
        }
        /* Nothing else to do when queue is empty — go back to blocking */
    }
}
