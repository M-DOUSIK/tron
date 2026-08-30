# ENGINEERING_LESSONS.md — Hard-Won STM32CubeIDE Rules

These come from a real, already-encountered failure on this exact board (STM32N6570-DK)
while bringing up the LCD + GT911 capacitive touch controller. They are not
hypothetical — ignoring them already cost real time once. Every session prompt that
touches hardware peripherals references this document; read it before Session 03.

## What Went Wrong Before

An earlier attempt to add I2C-based touchscreen support tried to shortcut the proper
STM32CubeMX flow:
- Manually copied `stm32n6xx_hal_i2c.c/.h` into the project instead of enabling I2C
  through the `.ioc` file.
- Hand-edited the auto-generated `subdir.mk` Makefiles to force the compiler to build
  the I2C driver files — STM32CubeIDE regenerates these on every build from its
  internal project state, silently wiping the manual edits.
- Copied driver files onto disk without accounting for STM32CubeIDE/Eclipse's cached
  workspace — the IDE didn't know the new files existed until the workspace was
  explicitly refreshed.

Result: linker errors (`undefined references to HAL_I2C_Init`, missing toolchain
references) that looked like a toolchain problem but were actually a process problem.

**The actual fix:** open the `.ioc` file in STM32CubeMX, enable the I2C2 peripheral
(confirmed wired to the touch controller on pins **PD14/PD4** on this specific board),
confirm the **FSBL** context is selected, click **Generate Code**. This single step
correctly pulled in the HAL I2C drivers, generated the right `HAL_I2C_MODULE_ENABLED`
define, and updated the Makefiles — the build succeeded immediately.

## Hard Rules for Every Session (Antigravity and human alike)

1. **MANUAL PERIPHERAL INITIALIZATION (UPDATED RULE):** The base project for Session 04 & 05 was derived from an ST example project and does **not** have an associated `.ioc` file. Therefore, you cannot rely on STM32CubeMX to generate peripheral initialization code. If a future session prompt asks to initialize peripherals using STM32CubeMX, **ignore that instruction**. Instead, proactively pull the necessary HAL drivers, configure the clocks, pins, and initialize the peripherals manually in code (e.g. `stm32n6xx_hal_msp.c`, `main.c`, or a dedicated driver file).
2. **Never hand-edit generated Makefiles.** `subdir.mk`, `Makefile`, and anything else under `Debug/`/`Release/` belong to the IDE. If a build isn't picking up a file you expect it to, the fix is in the project settings, not the Makefile.
3. **Workspace refresh awareness.** If custom `.c`/`.h` files (application logic, not HAL-generated) are added directly to the filesystem outside the IDE, refresh the STM32CubeIDE workspace (F5) before building — the indexer won't see new files otherwise.
4. **Trace hardware dependencies before writing drivers.** Before implementing a session that touches a peripheral (camera, touch, SD, etc.), check the board schematic/BSP headers first to confirm exactly which internal peripheral it's wired to (e.g. the touch controller here is a **GT911 on I2C2**) — don't guess or assume from a different board's pinout.

## Confirmed Board-Specific Facts (from real bring-up on this board)

- Touch controller part: **GT911** (capacitive touch, on the DK's 5" LCD panel).
- Bus: **I2C2**, pins **PD14 (SCL)** and **PD4 (SDA)** — carry this into
  `SOFTWARE_ARCHITECTURE.md`'s pin map and `touch_driver.c` (Session 05) directly;
  this was confirmed via a working `.ioc` generation on this exact board, not guessed.
- Context: **FSBL** must be selected in the `.ioc` for I2C2 to generate correctly.

## New Findings (Session 06) — Power Domains and Bare-Metal Initialization

- **VddIO Power Domains:** The STM32N6570 has multiple isolated power domains (`VddIO1` through `VddIO5`) that are turned OFF by default. You **MUST** explicitly enable them before initializing GPIOs in those domains. For example, `HAL_PWREx_EnableVddIO5()` must be called before configuring the SDMMC2 pins on GPIOC/GPIOE, or the pins will remain physically unpowered and the driver will hang indefinitely waiting for the hardware to respond.
- **Micro-SD Card Peripheral:** The micro-SD card slot on the STM32N6570-DK is physically wired to **SDMMC2** (Pins PC0, PC2, PC3, PC4, PC5, PE4). 
- **FatFs Polling Traps:** If `HAL_SD_Init()` completes but `HAL_PWREx_EnableVddIO5()` was forgotten, the card is actually unpowered. A blocking function like `HAL_SD_ReadBlocks` might silently fail to read data (or time out), but subsequent polling loops like `while (HAL_SD_GetCardState(&hsd2) != HAL_SD_CARD_TRANSFER) {}` will loop forever. Always include a timeout in hardware polling loops.

## Reference Repository — LCD Bring-Up

A working, already-verified STM32N6570-DK LCD example exists at:
**https://github.com/jpcano/STM32N6-digits/tree/lcd**

This repo succeeded where a from-scratch manual attempt struggled. Session 03
(camera→LCD bring-up) should start from this repo's LCD/LTDC/PSRAM configuration as a
known-good base rather than configuring LTDC entirely from scratch — then add the
DCMIPP camera pipeline on top of it. This doesn't replace Session 03's camera work,
but it de-risks the LCD half of that session significantly.

## Reference Repository — µT-Kernel 3.0 BSP Samples

For Session 12 (µT-Kernel 3.0 migration): **https://github.com/tron-forum/mtk3bsp2_samples**
contains official sample projects for the µT-Kernel 3.0 BSP2. Use these as the
reference for correct `tk_cre_tsk`/`tk_cre_mbf`/etc. usage patterns on STM32 rather
than inferring API usage from documentation alone.

## Project Management Rules

- **Always Create New Folders for New Sessions:** When moving to a new session (e.g., from Session 06 to Session 07), NEVER work directly in the previous session's directory. Always copy the entire working project to a new directory (e.g., copy `session_06` to `session_07`), and import the new project into STM32CubeIDE before making any modifications. This ensures that past sessions remain fully intact and functional, and provides a safe rollback point if the new session's codebase gets corrupted or encounters unrecoverable errors.

## Stale GCC Dependency Files After Folder Rename / Copy

### What Went Wrong (Session 04 & 05)

After the session project folder was renamed from `session_04` to `session_04&05`, the build
produced a cryptic error:

```
make: *** No rule to make target 'C:/.../sessions/session_04/DCMIPP_ContinuousMode/
Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.c', needed by
'Middlewares/STM32_ISP/isp_algo.o'. Stop.
```

The `subdir.mk` files were **correct** (they already contained the `session_04&05` path).
The culprit was the GCC-generated **`.d` dependency files** under `STM32CubeIDE/FSBL/Debug/**/*.d`.
These files are produced by the `-MMD -MP` compiler flags and cache the absolute source paths
discovered during a previous build. After the folder rename they still referenced `session_04/`,
a path that no longer exists, causing make to abort before a single line of compilation ran.

### Hard Rule

**Whenever a session project folder is copied, renamed, or moved to a new path:**

1. Delete all `.d` files inside the `Debug/` tree before the first build:
   ```powershell
   Get-ChildItem -Recurse -Filter "*.d" | Remove-Item -Force
   ```
2. Rebuild — the compiler regenerates correct `.d` files using the new absolute paths.

### Why This Can't Be Fixed in `subdir.mk`

`subdir.mk` controls what gets *compiled*. The `.d` files control what *triggers a recompile* —
they are included by make via the `-include` directive in the top-level `Makefile`. A stale `.d`
file creates a phantom prerequisite that make cannot satisfy, and make stops immediately.
STM32CubeIDE does **not** clean `.d` files on its own when the workspace path changes; this
must be done manually or scripted as a pre-build step.

### Affected Paths (pattern)
```
STM32CubeIDE/<context>/Debug/**/*.d
```
All `.d` files under any `Debug/` subfolder are safe to delete — they are 100% regenerated on
every build and contain no hand-authored content.
