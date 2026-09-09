# Session 03 — Camera → LCD Live Preview

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Route the live camera feed to the 5-inch LCD in real time, proving out the vision
hardware path before any AI or UI overlay work begins.

BACKGROUND AND CONTEXT
Building on tron/session_02/. The STM32N6570-DK has a DCMIPP (Digital Camera Memory
Interface Pixel Pipeline) and a NeoChrom 2.5D GPU. We need the hardware ISP to convert
raw Bayer camera data to RGB, land it in a framebuffer, and display it via LTDC.
Rather than configuring everything from scratch, we will use the official STM32Cube FW
`DCMIPP_ContinuousMode` example as our foundation. This example runs entirely in the
First Stage Boot Loader (FSBL) context without an Appli layer, providing a solid baseline
for camera and LCD initialization.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/main.c (orchestrator for the pipeline)
- tron/session_03/ (copied from STM32Cube FW `DCMIPP_ContinuousMode` example)

REQUIRED INPUTS
- Camera module physically connected to the DK board's camera connector (your task,
  not Antigravity's).

EXPECTED OUTPUTS
- A live, real-time camera preview visibly rendered on the LCD.

CONSTRAINTS
- Use the hardware ISP for Bayer→RGB conversion — don't do this in software on the CPU.
- The LCD framebuffer will reside in internal AXI SRAM (`0x34200000`) per the ST example.
  *Note: This consumes internal SRAM budget, which may require us to place future AI models in external PSRAM.*
- Maintain the FSBL-only architecture.
- Do not introduce RTOS, mascot UI, or AI this session — raw preview only.

CODING STANDARDS
- Leverage the existing `main.c` orchestrator from the ST example.

FOLDER STRUCTURE TO FOLLOW
Continue standard structure, operating within the `FSBL/` directory.

FILES TO CREATE
- None (use existing ST example files).

FILES TO MODIFY
- FSBL/Src/main.c (if any modifications are needed beyond the example)

DOCUMENTATION TO UPDATE
- docs/milestones/session_03_notes.md: AXI SRAM memory map for the framebuffers, DCMIPP
  pipe configuration used, any camera module specifics.
- Update SOFTWARE_ARCHITECTURE.md's pin map section with camera/LTDC pins now assigned.

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly.
- (Manual) Live camera image visible and color-accurate on the LCD, no visible tearing.

COMPLETION CHECKLIST
- [ ] AXI SRAM mapped and reserved for LCD framebuffers (`0x34200000`)
- [ ] LTDC initialized and driving the panel
- [ ] DCMIPP pipe configured for continuous capture via DMA
- [ ] Project successfully builds using the `DCMIPP_ContinuousMode` FSBL base
- [ ] docs updated (session notes + pin map)

COMMON PITFALLS
- Framebuffer stride/format mismatch between DCMIPP output and LTDC input causing
  color shifts or diagonal tearing artifacts.
- AXI SRAM timing/config errors that only show up as visual corruption, not build errors —
  budget bench time for this.

DEFINITION OF DONE
Waving a hand in front of the camera produces a smooth, correctly-colored reflection on
the LCD with no visible tearing.

SELF-REVIEW BEFORE DECLARING COMPLETE
Confirm no RTOS or AI code introduced. Confirm main.c stays a thin orchestrator, not a
dumping ground for camera/LCD register-level code.
```

## Expected Deliverables
Working Eclipse project based on `DCMIPP_ContinuousMode`, updated pin map, session notes.

## Manual Verification Steps
1. Connect the camera module (hardware task).
2. Flash and observe the LCD.
3. Wave a hand/object in front of the camera; confirm smooth, accurate live preview.
4. Watch for tearing across at least a minute of continuous operation.

## Acceptance Criteria
Stable, tear-free, color-accurate live preview sustained continuously.

## Next Prompt
Copy to `tron/session_03/`, proceed to `session_04.md`.

# Session 03 Notes

## Architecture & Hardware Allocation
- **Display Controller (LTDC):** Configured to output RGB565 to the 5-inch panel (800x480).
- **Framebuffer Memory:** Mapped to Hexadeca-SPI (XSPI1) PSRAM starting at `0x70000000`. This is critical for offloading the internal SRAM so it remains available for AI tasks later.
- **Camera Pipeline (DCMIPP):** Configured to capture CSI data, utilizing the hardware Image Signal Processor (ISP) for Bayer-to-RGB conversion, dumping frames directly into the PSRAM via DMA.

## Initialization Strategy
Due to the dual-context nature of the STM32N6570 and RIF security privileges, the CubeMX Appli generation left out the complete hardware initialization code for the LTDC and XSPI (which are sometimes handled in the FSBL). Following the project rule to isolate camera/LCD code, all peripheral initialization (LTDC timings, DCMIPP DMA start) has been modularized in `camera_lcd.c`. `main.c` simply calls `camera_lcd_init()` and `camera_lcd_start()`.

## Pin Assignments
The LTDC utilizes a standard 24-bit RGB888 parallel output mapping, spanning multiple GPIO ports (PA, PB, PD, PE, PG, PH). For a detailed pin breakdown, refer to the generated `.ioc` file. `SOFTWARE_ARCHITECTURE.md` has been updated to reflect the bus allocation.

## Verification
- Clean build in STM32CubeIDE.
- Ensure the external IMX335 camera module is connected before testing.
- Manual testing will confirm smooth, tear-free video rendering.
