# Engineering Lesson: Overcoming STM32N6 NPU and Eclipse Debugger Limitations

In Session 08A, we embarked on porting our working STM32N6 Image Classification model to use the high-performance NPU (Neural Processing Unit). Along the way, we hit two significant roadblocks that taught us critical lessons about the STM32N6 memory architecture and the limitations of the STM32CubeIDE debugger.

## 1. The NPU Hardware Fault (BUSIF0 ERR: 0x1f)
When we first initialized the ST Edge AI network and ran inference, the NPU immediately crashed the system, throwing a hard fault (`BUSIF0 ERR: 0x1f`).

**The Root Cause:**
The ST Edge AI library allocated the model's weights (`network_data.hex`) as `const` arrays in the `.rodata` section. Our linker script placed `.rodata` inside `AXISRAM1` (the main system RAM where our application code lives). 
However, according to the STM32N6 Reference Manual's Bus Matrix diagram, the NPU's Data Masters (MST0 and MST1) **physically lack a connection** to `AXISRAM1`. The NPU simply cannot read from that memory region, resulting in a bus fault when it tried to fetch the weights.

**The Fix:**
We modified our C code to tag the weight arrays with a specific memory section attribute (`__attribute__((section(".xspi2")))`) and updated our linker script to map `.xspi2` to the external OCTOSPI2 NOR flash memory (`0x71000000`). This is a memory region the NPU's AXI interface can directly access at high speeds.

## 2. The Eclipse Debugger Lockup ("Target is not responding")
After fixing the memory mapping, clicking "Debug" in STM32CubeIDE caused the IDE to hang and throw a "Target is not responding" error, which also crashed the ST-Link USB connection.

**The Root Cause:**
Our new `.elf` file contained both internal RAM sections (`0x34000000`) and the large 2.6 MB external flash section (`0x71000000`). To flash the external memory, STM32CubeIDE uses an External Loader (`MX66UW1G45G_STM32N6570-DK.stldr`).
However, Eclipse's default `connect_under_reset` debug configuration holds the MCU in hardware reset while the external loader tries to communicate with the flash chip. This prevents the external loader from correctly initializing the XSPI peripheral. Furthermore, if the flash chip was left in Octal (OPI) mode from a previous run, the 1-bit SPI initialization commands from the loader would be completely ignored, causing the flash process to permanently hang.

**The Fix:**
We decoupled the external flash programming from the IDE's debug cycle:
1. We used `arm-none-eabi-objcopy` to extract the `.xspi2` section into a standalone `weights.hex` file.
2. We flashed `weights.hex` to the external NOR flash once using `STM32_Programmer_CLI` in `HOTPLUG` mode (which doesn't hold the board in reset).
3. We updated our linker script to mark the `OSPI_NOR` section as `(NOLOAD)`. This tells the IDE debugger that the section exists for address resolution, but it shouldn't attempt to erase or program it.

As a result, STM32CubeIDE now only flashes the lightweight internal RAM application, which takes milliseconds and never hangs, while the NPU flawlessly fetches its weights from the pre-programmed external flash!

## Result
Our FreeRTOS application is now successfully running concurrent Camera ISP processing, UI rendering, SD Card logging, and hardware-accelerated NPU Image Classification with an incredible latency of **~3-4 ms** per frame!

## Flashing Reference (command line, no STM32CubeIDE GUI needed)

Session 08B needed this repeatedly while debugging the NPU pipeline
hardware-in-the-loop. `STM32_Programmer_CLI.exe` and its external loaders
ship inside the STM32CubeIDE install — exact path depends on which
`cubeprogrammer.win32_*` version is installed; check
`C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\` for the folder name on this
machine. The examples below use the version found during Session 08B.

### 1. Flash + run the application (internal RAM — the normal, fast, every-edit iteration loop)

Equivalent to STM32CubeIDE's own Debug/Run, but from a terminal. Use after
any plain code change — this never touches external flash.

```powershell
$cli = "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"
$elf = "C:\Users\Dousik\Workspace\TRON\sessions\<session_folder>\STM32CubeIDE\FSBL\Debug\<ProjectName>.elf"

& $cli -c port=SWD -w $elf -rst
```
`-rst` (software reset) is what actually worked reliably across Session
08B's many iterations — an explicit `-g <entry_address>` ("go") was tried
once as an alternative and, empirically, did not reliably result in a
running/UART-visible application on this board. Stick with `-rst` unless
you have a specific reason not to.

### 2. Flash external OSPI NOR weight/data content (one-time — only when `.xspi2`-tagged model files change)

The linker script marks the OSPI NOR output section `(NOLOAD)` on purpose
(see the main lesson above) — the command in step 1 never touches it. Any
new/changed model weight data needs a **separate, explicit write to its own
physical address**, using the external loader:

```powershell
$cli = "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"
$loader = "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\ExternalLoader\MX66UW1G45G_STM32N6570-DK.stldr"

& $cli -c port=SWD mode=HOTPLUG -el $loader -w "<path-to-binary>" <address>
```
`mode=HOTPLUG` avoids holding the MCU in reset while the external loader
talks to the flash chip (the exact failure mode this repo's older LCD/touch
lesson already warns about for a different peripheral). To read back and
sanity-check what's actually in flash at an address (e.g. to confirm a
write landed, or to check a known magic-number header):
```powershell
& $cli -c port=SWD mode=HOTPLUG -el $loader -r32 <address> <word_count>
```
**The flashed image is tied to the build that produced it.** Session 12 found
that the `Debug` and `Release` configurations emitted the `.xspi2` blobs in
*different order* (`-O0` ascending, `-Os` reversed), so a Debug-flashed image
made Release fail with `Error: Epoch Controller binary is invalid` and no
amount of re-flashing could satisfy both at once. Fixed by adding
`-fno-toplevel-reorder` to the Release compiler settings, which restores source
order and makes one image serve both — see `ENGINEERING_LESSONS.md`. Before
blaming code for an epoch-controller error, compare the layouts:
```bash
arm-none-eabi-nm -n <build>/<Project>.elf | grep '^71' | head
```
A different first symbol than the build the image came from means the flash is
wrong for this binary.

**Every distinct model/weight file needs its own address** — don't assume
one `.xspi2`-tagged blob is the whole story. Session 08B's face pipeline
needed flashing to *three* separate addresses across two rounds of
debugging before it worked: the epoch-controller microcode blobs (linked
via the normal build, address determined by the linker script), and two
more, completely separate constant-data pools whose addresses only show up
in generated-source comments like `file postfix=xSPI2 name=octoFlash
offset=0x70380000` — grep the model's `.c` file for `offset=0x` if
detection/inference runs but produces garbage/NaN output; it's very likely
a missing weight region at one of these addresses, not a code bug. Full
story in `documents/milestones/session_08B_notes.md`.

### 3. If the board hangs at boot after flashing external memory

A board that boots fine, then hangs (often inside a `HAL_XSPI_GET_FLAG`
polling loop) right after flashing OSPI content — even though the flash
write itself reported success — is very likely the flash chip's live bus
state getting confused by back-to-back external-loader operations without
a reset in between. **Do a full power cycle** — unplug the USB cable
entirely (not a software/debugger reset), wait a few seconds, plug it back
in. The flash *content* survives (non-volatile); only the peripheral's live
bus state needed clearing. Seen and fixed exactly once, in Session 08B.
