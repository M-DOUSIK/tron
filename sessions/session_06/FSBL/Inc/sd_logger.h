#ifndef __SD_LOGGER_H
#define __SD_LOGGER_H

#include <stdint.h>
#include <stdbool.h>

/* Initializes the SD Logger (Mounts FATFS) */
bool SD_Logger_Init(void);

/* Appends a text event to the log file */
bool SD_Log_Event(const char* event_string);

/* Writes a binary blob to the SD card */
bool SD_Log_Binary(const uint8_t* data, uint32_t length);

#endif /* __SD_LOGGER_H */

