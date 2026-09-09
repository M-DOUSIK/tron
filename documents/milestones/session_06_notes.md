# Session 06 Notes — SD Card Logging

## FATFS Configuration Details
- **Version:** FatFs R0.15 (manually integrated).
- **LFN (Long File Names):** Enabled with dynamic working buffer on the STACK (`FF_USE_LFN 2`). This avoids dependencies on dynamic memory allocation (`malloc`/`free`) and is suitable for our FSBL context.
- **RTC / Timestamps:** Fixed timestamps used (`FF_FS_NORTC 1`) since a hardware RTC interface (`get_fattime`) is not yet implemented.
- **Sector Size:** Fixed at 512 bytes (`FF_MAX_SS 512`).
- **Format Requirements:** The microSD card must be formatted as **FAT32**. exFAT is not enabled in this configuration.

## File and Folder Layout

The logging module currently places files directly in the root directory (`/`) of the SD card:

1. **`/events.log`**: 
   - A plain text file storing system events (e.g., "System Boot..."). 
   - Uses append-only semantics. If the file exists, new logs are appended to the end. If it does not exist, it is created.

2. **`/dummy_face.bin`**:
   - A binary file representing a placeholder for future biometric data (e.g., face embeddings).
   - Currently written with a fixed 8-byte dummy payload at system startup. 
   - **Note:** In this iteration, it is overwritten on every boot (`FA_CREATE_ALWAYS`), but will later be adapted in Session 08C when the actual biometric storage schema is finalized.

## Architecture Notes
Due to the absence of a `.ioc` file in the baseline project, the SDMMC and FATFS integration was handled manually:
- The FatFs core files were copied into `FSBL/Src` and `FSBL/Inc`.
- A custom `sd_diskio.c` wrapper was written to bridge FatFs `disk_read`/`disk_write` calls directly to the ST `HAL_SD_*` functions.
- `HAL_SD_Init` is manually invoked in `main.c` during initialization.
