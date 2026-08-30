#include "sd_logger.h"
#include "ff.h"
#include <string.h>
#include <stdio.h> // For printf

static FATFS SDFatFs;
static bool is_mounted = false;

bool SD_Logger_Init(void) {
    printf("SD_Logger_Init: Attempting to mount SD card...\r\n");
    FRESULT res = f_mount(&SDFatFs, "", 1);
    if (res == FR_OK) {
        is_mounted = true;
        printf("SD_Logger_Init: SD card mounted successfully.\r\n");
        return true;
    }
    printf("SD_Logger_Init: f_mount failed with error %d\r\n", res);
    return false;
}

bool SD_Log_Event(const char* event_string) {
    if (!is_mounted) {
        printf("SD_Log_Event: SD card not mounted.\r\n");
        return false;
    }
    
    FIL file;
    FRESULT res = f_open(&file, "events.log", FA_OPEN_APPEND | FA_WRITE);
    if (res != FR_OK) {
        printf("SD_Log_Event: f_open failed with error %d\r\n", res);
        return false;
    }
    
    UINT bytes_written;
    f_write(&file, event_string, strlen(event_string), &bytes_written);
    f_write(&file, "\n", 1, &bytes_written); // Add newline
    f_close(&file);
    
    printf("SD_Log_Event: Event logged to SD card: %s\r\n", event_string);
    return true;
}

bool SD_Log_Binary(const uint8_t* data, uint32_t length) {
    if (!is_mounted) {
        printf("SD_Log_Binary: SD card not mounted.\r\n");
        return false;
    }
    
    FIL file;
    // Overwrite the dummy binary file for this test
    FRESULT res = f_open(&file, "dummy_face.bin", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        printf("SD_Log_Binary: f_open failed with error %d\r\n", res);
        return false;
    }
    
    UINT bytes_written;
    f_write(&file, data, length, &bytes_written);
    f_close(&file);
    
    printf("SD_Log_Binary: Binary data logged (%lu bytes).\r\n", length);
    return (bytes_written == length);
}

