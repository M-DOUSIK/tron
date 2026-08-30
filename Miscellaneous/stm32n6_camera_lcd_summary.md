# STM32N6570-DK Project Summary
**Date:** August 7, 2026
**Hardware:** STM32N6570-DK Discovery Kit, MB1854B AI Camera Module (IMX335 sensor), RK050HR18 LCD panel.

This document summarizes the steps taken to configure, run, and modify projects on the STM32N6570-DK board.

## 1. LED Blinking (The `test` project)
**Goal:** Blink LD1 (Green) and LD2 (Orange) on the board.

**Key Learnings - FSBL vs Appli:**
The STM32N6570 uses a dual-project architecture for security (TrustZone / RIF isolation):
*   **FSBL (First Stage Boot Loader):** Runs in the secure world. Its job is to initialize system clocks, configure the security firewall (RIF/RISAF), set up external memory (PSRAM via XSPI), and then hand control over to the application. **You should generally not put infinite `while(1)` application loops here**, or it will never boot the main application.
*   **Appli:** This is the main application where user logic goes.

**What we did:**
We started with a blank CubeIDE project (`Downloads\test`). We initially placed the LED code in `FSBL`, which prevented it from working correctly. We then cleaned the `FSBL` and injected the GPIO initialization (PO1 and PG10) and the `while(1)` toggle logic with `HAL_Delay(500)` into `Appli/Core/Src/main.c`.

## 2. Live Camera to LCD Preview
**Goal:** Stream live video from the IMX335 camera sensor directly to the LCD panel.

**The Challenge:**
Unlike blinking an LED, the camera-to-display pipeline is extremely complex. It requires:
1.  **Hardware Peripherals:** DCMIPP (camera pipeline), CSI (MIPI interface), LTDC (Display controller), and XSPI (PSRAM for the frame buffer).
2.  **Drivers:** BSP (Board Support Package) drivers for the IMX335 sensor and the LCD panel.
3.  **Middleware:** The ISP (Image Signal Processor) library to process raw image data from the camera.
Because the original `test` project was generated as a "blank" project, it lacked all of these drivers and configurations.

**The Solution (`camera_preview` standalone project):**
Instead of manually injecting dozens of files and hacking the Eclipse `.cproject` XML of the empty `test` project, we created a fully standalone project.

1.  We located ST's official working example (`DCMIPP_ContinuousMode`) deep in the STM32 firmware repository.
2.  We copied this example into `C:\Users\Dousik\Downloads\camera_preview`.
3.  We copied the `Drivers` and `Middlewares` folders from the STM32 repository directly into the `camera_preview` folder so the project would be 100% self-contained.
4.  We ran a script to patch the `.project` and `.cproject` files, replacing the relative paths (which originally pointed 7-8 levels deep back into the ST repository) with `../../../Drivers` and `../../../Middlewares` to point to the local copies.
5.  **Result:** A fully functional, out-of-tree camera preview project that builds and runs instantly in CubeIDE without modifying the original STM32 repository.

## Future Reference
*   **Your basic code sandbox:** Use `Downloads\test\Appli`.
*   **Your camera/LCD reference:** Use `Downloads\camera_preview\STM32CubeIDE\FSBL`. (Note: The camera example runs entirely in FSBL because setting up the DCMIPP, LTDC, and PSRAM requires secure-world privileges).
