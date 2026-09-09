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

For Session 11 (µT-Kernel 3.0 migration — this repo's `Session 12` was the
pre-renumbering name; see `MASTER_PROJECT_PLAN.md`'s v8 Changelog):
**https://github.com/tron-forum/mtk3bsp2_samples** contains official sample
projects for the µT-Kernel 3.0 BSP2. Its `Examples/prj_stm32n6_cam` targets
this exact board (STM32N6570-DK / STM32N657X0) with a working camera+LCD
demo — Session 11 vendored that project's entire `mtk3_bsp2/` source tree
and its own proven `Debug/mtk3_bsp2/**/subdir.mk` build config wholesale
into `FSBL/mtk3_bsp2/` rather than hand-porting the BSP or re-deriving which
files a from-scratch STM32N6 config would need — see
`milestones/session_11_notes.md` for the full integration story, including
a real structural gotcha this repo's sample surfaced: µT-Kernel's
`tk_cre_tsk`/`tk_cre_mbf`/`tk_cre_mtx` can only be called once the kernel is
already running (inside its `usermain()`), which doesn't fit this project's
existing "create every task/queue in `main()`, then start the scheduler"
FreeRTOS-era flow without an explicit deferred-creation bridge in
`ms_osal.c`.

## Restoring a File From an Older Source Silently Skips the Rebuild (Session 11)

### What went wrong

Session 11 needed to strip temporary debug instrumentation out of five vendored
`mtk3_bsp2/` files by restoring them from the original upstream copy in
`scratch/mtk3bsp2_samples/`. The restore itself was correct. The rebuild was not:
the resulting `.elf` still contained every single probe.

**Why:** `Copy-Item` on Windows (and `cp -p` on POSIX) sets the destination's
modification time to the **source's** — and the pristine upstream files are older
than the `.o` files that had been built from the instrumented versions. `make`
compared timestamps, concluded the sources were older than their objects, and
skipped them entirely. There was no error, no warning, and the `.elf` even shrank
slightly (from unrelated relinking), so the build looked like it had worked.

### Hard rule

After restoring, reverting, or copying any source file from an older location,
either `touch` the restored files or delete the corresponding objects before
building — and then **verify the compiled artifact, not the build log**:

```bash
# force the rebuild
touch path/to/restored_file.c

# then prove it actually took, against the .elf
arm-none-eabi-nm  Debug/<Project>.elf | grep -i <symbol_that_should_be_gone>
strings           Debug/<Project>.elf | grep -c "<format string that should be gone>"
```

### Why this belongs next to the stale-`.d`-file rule

It is the same failure family: make's timestamp model quietly doing the wrong
thing after a file operation that originated outside the build system. The `.d`
rule above covers *renaming or moving* a project folder; this one covers
*restoring an older file in place*. Both produce a build that succeeds while
compiling something other than what is on disk, which is far more expensive to
diagnose than an outright error — in Session 11's case it would have meant
flashing the board with a binary that still had the exact instrumentation the
rebuild was supposed to remove.

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

## Stale Absolute Paths After Folder Copy — Second Bullet: Never `sed` the Whole Tree (Session 09)

**Note:** the folder this incident describes as `session_09B` was Session 08B's actual working
folder, created under that collision-avoiding name because the original `session_08B` folder had
been deleted at the time (see `prompts/session_08B.md`). It has since been renamed to
`session_08B`, its correct, final name — the string literals below (`session_09B`, the exact
`grep`/`sed` commands run) are preserved as an accurate record of what actually happened during
this incident, not as the folder's current name.

Session 08B's notes (see `milestones/session_08B_notes.md`) already documented that every
generated `subdir.mk`/`makefile` bakes in the old folder name as an absolute path, and that a
blind `session_08A → session_09B` string replace across every `.mk`/`makefile` file is required
after a copy. Session 09 repeated that same fix (`session_09B → session_09`) and hit a new,
more damaging variant of the same problem:

**What went wrong:** the replace was scoped with `grep -rl "session_09B" STM32CubeIDE` and then
ran `sed` on every matched file — which matched not just `subdir.mk`/`makefile` text files but
also the *compiled* `.o` object files still sitting in `Debug/`/`Release/` from the copied build
(their embedded DWARF debug info contains the same absolute source paths as strings). Running
`sed 's/session_09B/session_09/g'` against a binary `.o` file shortens it by one byte per match
(`"session_09B"` → `"session_09"` removes the trailing `B`) **without fixing up any of the
length-prefixed offsets elsewhere in the ELF structure that point past that string** — every
byte after the first match is now misaligned. The linker's failure mode was `file too short` /
`missing section headers at <offset>` on the object file — a corrupted-binary symptom that looks
nothing like "we edited a path," which is what made it non-obvious to diagnose.

**The fix:** don't grep-and-sed indiscriminately across a build output tree. Scope the text
replace to only the files that are supposed to contain hand-relevant absolute paths —
`subdir.mk`, `makefile`, `.project`, `.launch` — never `.o`, `.d`, `.su`, `.cyclo`, `.list`,
`.map`, or any other compiler-generated artifact. In practice, the simplest safe fix once this
has already happened is the same as the original `.d`-file rule taken further: delete **all**
generated build artifacts (`.o`, `.d`, `.su`, `.cyclo`, `.list`, `.map`, `objects.list`) under
`Debug/`/`Release/` after a folder copy and path-rename, then do a full rebuild — they are all
100% regenerated from source and none of them should ever be hand-edited or text-replaced in
place.

**One more consequence to know about:** `objects.list` (the linker's response file, listing
every object the link step should include) is *not* regenerated by a plain `make all` — it is
normally written by STM32CubeIDE's own internal build orchestration, which a raw command-line
`make` invocation bypasses. If it's missing (e.g. because it was deleted as part of the cleanup
above, or never existed for a copied project), regenerate it by scanning every real `subdir.mk`
under the subdirectories actually listed in `Debug/sources.mk`'s `SUBDIRS` for their `OBJS +=`
entries — same technique Session 08B's notes describe when a stray duplicate build folder
threatened to poison this same file.

## STM32CubeIDE Can Be Driven Headlessly — the "IDE build" Is Not GUI-Only (Session 12)

### What this unlocks

Session 11's Addendum 1 documented a real and expensive failure: hand-edited
`Debug/` makefiles produced a clean command-line build while the *actual* IDE
build failed, because STM32CubeIDE regenerates `Debug/sources.mk`,
`Debug/makefile` and every `subdir.mk` **from `.project` + `.cproject`** before
every build and silently discards whatever is already sitting in `Debug/`. The
conclusion drawn at the time was that verifying this needs a human in front of
the GUI.

It does not. STM32CubeIDE ships `stm32cubeidec.exe` (the console-mode launcher)
alongside `stm32cubeide.exe`, and it exposes Eclipse CDT's standard headless
managed-build application:

```bash
IDE="C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/stm32cubeidec.exe"
WS="/some/scratch/workspace"      # throwaway; never point this at the repo

# Import the project once, then clean-build one configuration:
"$IDE" --launcher.suppressErrors -nosplash \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
  -data "$WS" \
  -import "<repo>/sessions/session_NN/STM32CubeIDE/FSBL" \
  -cleanBuild "MedSight_SessionNN_FSBL/Debug"

# Subsequent builds don't need -import again (the workspace remembers):
"$IDE" --launcher.suppressErrors -nosplash \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
  -data "$WS" -cleanBuild "MedSight_SessionNN_FSBL/Release"
```

Omit the `/Debug` or `/Release` suffix to build **every** configuration. The
build log goes to stdout in exactly the form the GUI's Console view shows,
ending with a `Build Finished. N errors, M warnings.` line per configuration.

This is the real thing, not an approximation: it runs the same CDT managed-build
machinery the GUI does, regenerates the whole `Debug/`/`Release/` tree from
`.project`/`.cproject` first, and therefore catches exactly the class of bug
Addendum 1 hit. Session 12 used it to close that open item for both
configurations without a human touching the IDE.

### The bug it immediately found

`Release` had never been built since Session 08B, and the headless clean build
failed at link with ~150 `Unknown destination type (ARM/Thumb)` errors out of
`ll_sw_float.o` / `ll_sw_integer.o`. Root cause was in `.cproject`, exactly
where Addendum 1 said to look: the **Release** configuration's linker settings
were missing both the `../../../Middlewares/ST/AI/Lib` library search path and
the `:NetworkRuntime1200_CM55_GCC.a` library entry that **Debug** has had since
Session 08B. The AI runtime archive was simply never on the Release link line.
Adding the two entries to the Release `<tool ...c.linker...>` block fixed it;
`Release` now builds clean.

### Hard rules

1. **Point `-data` at a throwaway workspace outside the repository.** Eclipse
   writes a `.metadata/` tree there — tens of megabytes of indexes and locks
   that are worthless the moment the session ends. Sessions 08A-11 accumulated
   three of these (`temp_workspace`, `temp_workspace2`, `temp_workspace3`,
   51 MB total) inside the session folder and then copied them forward into
   every subsequent session's snapshot, still pointing at `session_08A`'s
   absolute paths. Session 12 excluded them from the folder copy.
2. **A headless build is the check, not a substitute for the code check.**
   It proves the project settings are right. It says nothing about whether the
   firmware runs — that is still a flash-and-watch step on real hardware.
3. **Still verify the artifact, not the log** (see the rule above this one).
   A "Build Finished. 0 errors" line means the build system was happy, not that
   the `.elf` contains what you think.

## A Return Value Nobody Checks Is a Bug Nobody Finds (Session 12)

### The bug

`FSBL/Src/sd_diskio.c`'s `disk_ioctl()` opened with:

```c
if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_TRANSFER) return RES_NOTRDY;
```

`HAL_SD_GetState()` returns the HAL **driver handle's** `State` field — the
software state of the driver object — not the card's status. Grep every
assignment to that field in `stm32n6xx_hal_sd.c` and you find the driver writes
only `RESET`, `READY`, `BUSY` and `PROGRAMMING` to it. **`HAL_SD_STATE_TRANSFER`
is never assigned anywhere in the driver.** The condition was unconditionally
true, so `disk_ioctl()` returned `RES_NOTRDY` for *every command it was ever
given*, from Session 06 to Session 12.

The check that was meant is `HAL_SD_GetCardState() != HAL_SD_CARD_TRANSFER` —
which the `disk_read()` and `disk_write()` paths in the same file already use
correctly, ten lines away. Two similarly-named HAL calls, one of which asks the
card and one of which asks the driver.

### Why it hid for six sessions

FatFs calls `disk_ioctl(CTRL_SYNC)` from `sync_fs()`, at the very end of
`f_close()` — **after** the file data, the dirty sector window and the directory
entry have all already been written — and converts a non-`RES_OK` result into
`FR_DISK_ERR`. So every close of every file reported a disk error over data that
had been written perfectly well. That is exactly why `patients.dat` always
persisted correctly regardless: the bug is downstream of the actual writes.

And the two call sites above it threw the result away. Sessions 06-11's
`SD_Log_Event()` was:

```c
f_write(&file, event_string, strlen(event_string), &bw);   /* result ignored */
f_write(&file, "\n", 1, &bw);                              /* result ignored */
f_close(&file);                                            /* result ignored */
printf("SD_Log_Event: logged: %s\r\n", event_string);
return true;                                               /* always */
```

`SD_Write_File()` was the same shape, checking only `bw == length` and never
`f_close()`. The error had nowhere to surface.

### What found it

Session 12's hardening pass started checking those return values. The bug
appeared on the *first* flash afterwards, as `SD_Log_Event: write failed 1` on
the boot log line — and, because the same pass had also taught `sd_logger.c` to
treat `FR_DISK_ERR` as "the card was pulled", it unmounted a perfectly healthy
card. A six-session-old silent failure became a one-flash diagnosis the moment
something looked at it.

### Hard rules

1. **Check the return value of every filesystem call, including `f_close()`.**
   `f_close()` is where FatFs does the final sync; ignoring it means ignoring
   the one call most likely to report that the media is unhappy. If a wrapper
   returns `bool`, that bool has to mean something.
2. **When two HAL calls have near-identical names, check which object each one
   interrogates.** `HAL_SD_GetState()` (driver) vs `HAL_SD_GetCardState()`
   (card) differ by four characters and by everything that matters. The same
   trap exists across the STM32 HAL for other peripherals.
3. **A guard that can never pass is indistinguishable from no guard at all
   until something depends on it.** If a status check exists, prove on hardware
   that it can return both answers — an enum value the driver never assigns is
   a dead branch wearing a safety check's clothing.
4. This is the same family as the two rules above about verifying the compiled
   artifact rather than the build log: in all three cases the system reported
   success through a channel nobody was actually reading.

## Debug and Release Laid the NPU Weights Out Differently, So Only One Could Ever Run (Session 12)

### What went wrong

Session 12 made the `Release` configuration build for the first time since
Session 08B. Running it produced two failures the `Debug` build never had:

```
Error: BSP_TS_Init failed with status -1
Error: Epoch Controller binary is invalid
assertion "ret == 1" failed: file ".../ll_aton_runtime.c", line 454,
                             function: LL_ATON_RT_Init_Network
```

That assertion is the same one Session 08B hit twice. Both earlier times the
cause was "the weights were never written to external flash". This time they
had been written — but **to a layout the Release binary does not use.**

### Root cause

The NPU's epoch-controller blobs are large `const` arrays tagged
`__attribute__((section(".xspi2")))`, which the linker script maps to OSPI NOR
at `0x71000000` and marks **`(NOLOAD)`** — so a normal Debug/Run never programs
them. They are flashed once, by hand, from an image extracted from a build.

Because the attribute names a single literal section (`.xspi2`, not
`.xspi2.<varname>`), `-fdata-sections` does **not** split them, and every blob
in a translation unit lands in one section whose internal order is simply
GCC's emission order. That order is not the same at `-O0` and `-Os`:

```
Debug   (-O0):  71000000 _ec_blob_faceid_1     71000700 _ec_blob_faceid_6   ...
Release (-Os):  71000000 _ec_blob_faceid_149   71002a80 _ec_blob_faceid_144 ...
```

**Reversed.** So every blob address the Release binary computed pointed at a
different blob's bytes, `ec_get_blob_ptr()` found a bad magic number, and the
runtime asserted. No amount of re-flashing would have fixed it — the two
configurations wanted physically different flash images, and only one image
fits in the flash at a time.

Verified by comparing symbol addresses in the `0x71000000` range across the
Session 08B, 11 and 12 `Debug` ELFs (byte-identical, so the flashed image was
still correct for Debug) against the Session 12 `Release` ELF (completely
different).

### The fix

Add **`-fno-toplevel-reorder`** to the Release C compiler settings in
`.cproject`. It forbids GCC reordering top-level definitions, which restores
source order and makes the Release layout identical to Debug:

```
Release+noreorder: 71000000 _ec_blob_faceid_1  71000700 _ec_blob_faceid_6 ...
```

One flashed weight image now serves both configurations. The optimisation cost
is negligible — it constrains emission order, not code generation — and it buys
a correctness guarantee that is otherwise impossible to hold onto.

### Hard rules

1. **Whenever anything about the model files or their compilation changes,
   re-check the `.xspi2` layout against the flashed image** before assuming a
   runtime failure is a code bug:
   ```bash
   arm-none-eabi-nm -n <build>/<Project>.elf | grep '^71' | head
   ```
   Compare against the build the flashed image was extracted from. Different
   first symbol = the flash is wrong for this binary, full stop.
2. **A `(NOLOAD)` section is invisible to every normal build check.** It links,
   it produces no warning, `size` reports it, and nothing verifies that what is
   physically in flash corresponds to it. Treat the flashed image as a build
   artifact with its own version, not as something that "is just there".
3. **An optimisation level is not a neutral choice when absolute addresses are
   involved.** Anywhere a section's *internal* ordering is load-bearing —
   hand-placed data, external memory images, anything programmed separately
   from the ELF — pin the order explicitly rather than assuming the compiler
   will be consistent across configurations.

### The diagnostic that should have existed from Session 08B

`main.c` now defines `__assert_func()`, overriding newlib's, so this failure
prints its actual cause and the exact recovery procedure instead of an
expression and a line number. Providing the symbol in an object file wins over
libc because the linker resolves objects before searching libraries.

The same class of failure had already cost this project three separate
debugging sessions (`session_08B_notes.md` Addenda 1 and 2, and this one) while
presenting each time as `assertion "ret == 1" failed`. When one error message
has burned three sessions, replacing it is cheaper than diagnosing it a fourth
time.

## A Cold Boot Is Not the Same Reset You Have Been Testing (Session 12)

### The symptom

After a full power cycle, flashing and running the firmware left the LCD dark —
gradually washing out, ghosting, then fading to nothing — while UART output was
completely normal. Clicking Run again *without* unplugging the board produced a
perfect display. Pausing in the debugger for ~20 s before resuming also worked.

Three observations that all say the same thing: **the first run after power is
applied behaves differently from every subsequent run.**

### Root cause

The STM32N6's higher GPIO banks are fed by separately supplied I/O domains
(VDDIO2..VDDIO5), and a bank whose domain has not been declared valid does not
drive its pins. Mapping the domains from the ST BSP's own call sites:

| Domain | Bank | Enabled by |
|---|---|---|
| VDDIO2 | GPIOO | `BSP_LED_Init()` |
| VDDIO3 | GPION | SD card-detect init |
| VDDIO4 | GPIOH | I2C1 MspInit (camera) |
| VDDIO5 | GPIOC, GPIOE | SDMMC2 MspInit |

LTDC's MspInit configures **PE11** (LCD_VSYNC), **PE1** (touch NRST) and
**PH3/PH4/PH6** (colour bits B4/R4/B5) — pins in the VDDIO5 and VDDIO4 domains —
and enables neither. Nothing enabled VDDIO5 until the SD card was initialised,
long after the panel had been configured and the LTDC had begun scanning out.

**Why it hid for nine sessions:** the PWR `SVMCR*` "supply valid" bits live in
the always-on power domain and **are not cleared by a system reset** — only by
actually removing VDD. Once any run had enabled them, every subsequent
flash-and-run inherited valid domains and the display came up correctly. The
normal development loop is flash-and-run on a board that never loses power, so
the bug was invisible until someone did a genuine cold boot.

This is the same trap as the Session 06 VDDIO5/SDMMC finding above, one layer
further out: that time the missing domain produced an obvious hang; this time it
produced a display that half-worked and a bug that only appeared when the
development habit changed.

### The fix

Enable **every** I/O domain the board uses once, in `main()`, immediately after
the supply and clock configuration and before any peripheral or GPIO init:

```c
HAL_PWREx_EnableVddIO2();
HAL_PWREx_EnableVddIO3();
HAL_PWREx_EnableVddIO4();
HAL_PWREx_EnableVddIO5();
```

The bits are idempotent, every one of these rails is populated on the
STM32N6570-DK, and the BSP's own later calls become harmless no-ops. It removes
the ordering dependency entirely rather than relying on the right peripheral
being initialised in the right order.

### Hard rules

1. **Enable all I/O supply domains up front, not on demand.** On-demand
   enabling makes correctness depend on peripheral init order, which changes
   whenever a task is added or reordered.
2. **Test from a cold boot, not just a reset.** Anything in an always-on domain
   — PWR supply-valid bits, backup registers, RTC configuration, some RCC
   state — survives a system reset and will mask a missing initialisation for
   as long as the board stays powered. A whole class of bug is invisible to the
   flash-and-run loop.
3. **"It works on the second try" is a diagnosis, not a workaround.** It means
   run N is leaving state that run N+1 depends on. Find out what.

## A Reset Line Configured by One Driver, Released by Another (Session 12)

`PE1` on the STM32N6570-DK is the GT911 touch controller's NRST. LTDC's MspInit
configures it as a push-pull output as part of bringing up the display — and
GPIO ODR resets to zero, so **initialising the display holds the touch
controller in reset**. Nothing releases it until `BSP_TS_Init()` runs, much
later.

`BSP_TS_Init()` then drives NRST high and probes the part over I2C
**immediately**, with no delay. The GT911 needs tens of milliseconds to boot
before it answers. Whether the probe succeeds therefore depends on how fast the
code happens to run between two adjacent lines — which is why touch
initialisation worked in the Debug build (`-O0`) and failed in Release (`-Os`)
with `BSP_ERROR_NO_INIT` (-1).

**Fix:** do the reset explicitly in this project's own `touch_driver_init()`
before calling into the BSP — assert NRST, hold 20 ms, release, wait 120 ms —
and retry the whole sequence once if the probe still fails.

**The rule:** when a shared line is configured by one driver and used by
another, own the sequencing yourself at the application layer. And never let a
device's power-on timing be satisfied by "however long the compiler decided the
intervening code should take."

## Putting a CPU to Sleep Is a System-Wide Change, Not a Power Tweak (Session 12)

### What happened

Session 12 added a `WFI` to the kernel idle path to satisfy the competition's
power-saving criterion, measured 89.6% idle, and recorded it as a win. It was
also, silently, the worst bug of the project.

`WFI` on the STM32N6 enters CSleep, which stops the CPU **and stops the clock
of every peripheral, bus and memory whose `LPEN` bit is clear**. The
framebuffer lives in AXISRAM3-6. So on every idle tick the LTDC's DMA lost
either its own clock, the AXI bus matrix clock, or the RAM it was reading. It
kept scanning and kept driving sync, and fetched nothing. From a cold boot the
home screen drew, glitched, and greyed out — while UART, touch, the SD card,
the NPU and all five tasks reported perfect health.

It took six rounds to find. Five diagnoses were wrong first.

### Why every measurement lied

Because the CPU takes the measurements, and the CPU is only running when it is
**not** asleep. A framebuffer checksum read by the CPU is always correct. An
LTDC register read by the CPU always says "enabled, scanning, right address".
The one agent that could see the problem — the LTDC's DMA — has no way to
report anything.

The same asymmetry produced the false clue that misled everything else: the
fault vanished under the debugger, and on warm re-runs driven from the
debugger. That reads exactly like a hardware settling problem. It actually
meant "a halted core never executes WFI".

### Hard rules

1. **Adding a sleep instruction changes the contract for every DMA master in
   the system.** Before enabling any sleep mode, enumerate what moves data
   without the CPU — display, camera, SD, accelerators, the bus matrix itself,
   and the RAM each of them touches — and explicitly keep those clocked. On
   STM32 that is the `*LPENR` family; every platform has an equivalent.
2. **A subsystem that cannot report is a subsystem you cannot debug by
   reading registers.** When the CPU's view of a peripheral is structurally
   unable to observe the failure, stop dumping registers and start perturbing
   the system. One build that removed the WFI and changed nothing else
   answered a question that four builds of instrumentation could not.
3. **"It works under the debugger" is evidence about what the CPU is
   executing, not about timing.** It was read as "the hardware needs settling
   time" for three rounds. It meant "the core is halted, so it is not sleeping".
4. **Change one variable per hardware run.** The runs that produced knowledge
   were the ones that changed exactly one thing. The runs that produced more
   theories changed several.
5. **The most recently added subsystem is the first suspect, especially when
   it is the one you are proud of.** The WFI was skipped for five rounds
   precisely because it was the session's headline feature and had been
   carefully reviewed for its *own* correctness. It was correct. Its effect on
   everything else was not.

### The part worth sitting with

Four of the six theories were about hardware — power domains, reset state,
settling time, panel timing. The cause was a deliberate, reviewed, documented
change made in that same session. The prior should have been the other way
round from the first round, and the question should have been "what did I add,
and what does it change for everything else" long before it was.
