# Running MedSight on your own STM32N6570-DK

A complete, tested path from a fresh clone to a board that dispenses. Written
for someone who has never seen this repository before.

There are **two ways to run it**, and you should do them in this order:

| | Mode | Needs a PC? | Use it for |
|---|---|---|---|
| **A** | **Development boot** | yes, USB attached | First run. You get the UART log, which tells you everything. |
| **B** | **Standalone boot** | no — power only | Showing the device as a device. Runs from a power bank. |

**Mode A is enough to evaluate every software feature.** Mode B exists because
a medication dispenser that needs a laptop is not a medication dispenser.

**No dispensing hardware? That is a supported configuration** — set one macro
and the whole application runs with the dispense simulated. See §7.

---

## 1. What you need

### Hardware

| Item | Required? | Notes |
|---|---|---|
| STM32N6570-DK | **yes** | with its camera module and LCD |
| USB-C cable | **yes** | to `CN6` |
| microSD card | **yes** | patient records and the audit log live here |
| 28BYJ-48 stepper + ULN2003A board | optional | the usual pairing, sold together |
| 3-pin IR break-beam module | optional | digital `OUT`, not analog |
| Active piezo buzzer | optional | **active**, not passive |
| USB power bank | Mode B only | any 5 V; a 10 000 mAh / 22.5 W unit was used |
| Female-female jumpers | optional | 14 of them |

Wiring for the optional peripherals: **`documents/HARDWARE_WIRING.md`**.
Fourteen wires, no resistors, no transistors, no external supply.

### Software

- **STM32CubeIDE 2.1.1** or newer. This project has **no `.ioc`** — all
  peripheral setup is hand-written HAL — so nothing needs regenerating and
  no CubeMX step exists.
- `STM32_Programmer_CLI` and `STM32_SigningTool_CLI`, which ship inside
  CubeIDE. On a default Windows install:

  ```
  C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_<version>\tools\bin\
  ```

  The external loader you will need is in `ExternalLoader\` beside them:
  `MX66UW1G45G_STM32N6570-DK.stldr`.

Every command below is written for **PowerShell**. Set these once per shell and
the rest can be pasted as-is:

```powershell
$BIN    = "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin"
$CLI    = "$BIN\STM32_Programmer_CLI.exe"
$SIGN   = "$BIN\STM32_SigningTool_CLI.exe"
$LOADER = "$BIN\ExternalLoader\MX66UW1G45G_STM32N6570-DK.stldr"
```

> Your plugin folder's version suffix will differ. `dir "$BIN\.."` to find it.

---

## 2. The one rule that will save you an hour

> ### After **any** external-loader operation, unplug the USB cable.

Not a reset. Not the IDE's stop button. **The cable, out, for a few seconds.**

Back-to-back operations through the external loader leave the flash chip's
live bus state confused. The symptom is a boot that hangs where XSPI is first
touched — the log stops right after `ai_vision_init: DEBUG build (-O0).` and
the LCD holds a stale frame. The flash *contents* are fine; only the
peripheral's bus state needs clearing.

**This applies to reads as well as writes.** It was originally documented for
writes only and caught us a second time after two verification reads — which
is exactly when you are most tempted to leave the board running.

---

## 3. Which build to use

```
sessions/session_17/          <-- the latest, and the one to build
```

The repository keeps one folder per development session. **`session_17` is the
current build**; earlier folders are history and are kept so the project's
progression is auditable. There is no `session_14` — it is a retired number.

The CubeIDE project is at:

```
sessions/session_17/STM32CubeIDE/FSBL/
```

and is named **`MedSight_Session17_FSBL`**.

---

## 4. Step by step — Mode A, development boot

### 4.1 Set the boot switches

| Switch | Position | Meaning |
|---|---|---|
| **SW1 (BOOT1)** | **HIGH** | development boot — wait for the debugger |
| **SW2 (BOOT0)** | **LOW** | |

This is the board's factory position. If you have never moved them, they are
already right.

### 4.2 Build

1. **File → Import → Existing Projects into Workspace.**
2. Point it at `sessions/session_17/STM32CubeIDE/` and import
   `MedSight_Session17_FSBL`.
3. Build the **Debug** configuration.

Expected result: **0 errors, 0 warnings.**

Headless equivalent, if you prefer:

```powershell
& "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\stm32cubeidec.exe" --launcher.suppressErrors -nosplash `
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild `
  -data <a scratch workspace dir> `
  -import "sessions\session_17\STM32CubeIDE\FSBL" `
  -cleanBuild "MedSight_Session17_FSBL/Debug"
```

### 4.3 Flash the NPU weights — **do not skip this**

The firmware **does not contain its neural network weights.** The linker marks
the external NOR regions `(NOLOAD)` deliberately, so building and flashing the
application never writes them.

A fresh clone therefore compiles a perfectly good `.elf`, flashes it, and
produces a device whose face recognition and action recognition both output
garbage — **with nothing in any log to say why.**

```powershell
& $CLI -c port=SWD mode=UR -el $LOADER -w "weights\fd_data.xSPI2.bin"      0x70380000
& $CLI -c port=SWD mode=UR -el $LOADER -w "weights\ec_blobs.xSPI2.bin"     0x71000000
& $CLI -c port=SWD mode=UR -el $LOADER -w "weights\faceid_data.xSPI2.bin"  0x72000000
& $CLI -c port=SWD mode=UR -el $LOADER -w "weights\hand_atonbuf.xSPI2.bin" 0x73000000
& $CLI -c port=SWD mode=UR -el $LOADER -w "weights\pill_atonbuf.xSPI2.bin" 0x73400000
```

**Then unplug the cable** (§2).

Full details, SHA-256 fingerprints and provenance for each blob:
**`weights/README.md`**.

> **`mode=UR` vs `mode=HOTPLUG`.** `weights/README.md` prescribes `HOTPLUG`,
> which was correct when it was written — the only writes happened before any
> application existed. Once MedSight is on the board, `HOTPLUG` needs a live,
> responsive core, and with MedSight *running* the erase fails outright,
> because the application has XSPI2 memory-mapped for the weights while the
> loader wants the same bus in indirect mode. **Use `mode=UR` for writes to
> external flash.** That README is due a correction.

### 4.4 Insert the microSD card

An empty FAT32 card is fine — the firmware creates what it needs. Patient
records, face embeddings and the audit log all live here.

### 4.5 Run

Press **Run** (or **Debug**) in CubeIDE. The `.launch` configuration is
committed and needs no editing.

### 4.6 Watch the log

Open the ST-LINK virtual COM port at **115200 8N1**.

A correct boot says, in this order:

```
ai_vision_init: face detector + embedder ready.
ARENA SELFTEST PASS: 225280 bytes writable and readable
intake: hand landmark model ready (MediaPipe 224x224 INT8, 21 keypoints, ...)
intake: pill detector ready (YOLOv8n 160x160 INT8, head cut, decode on CPU).
dispenser: ...
buzzer: active piezo on PA3 (Arduino D10), keyboard click off
task_camera_isp: started.
```

> **If detection runs but the numbers are meaningless, suspect a weight region
> before you suspect the code.** A missing or wrong weight image does not
> announce itself as a flash error — the network initialises cleanly and
> returns nonsense.

### 4.7 Two LEDs that tell you the RTOS is alive

Worth knowing before you wonder about them:

- The **green** LED toggles once per iteration of the camera/ISP task, which
  runs every 1 ms. At ~500 Hz your eye integrates it into a **steady dim
  glow** — dimly lit means the ISP is running every frame.
- The **red** LED toggles every 500 ms in the idle task — a **1 Hz blink**.

Both ends of the priority range visibly alive. Mixed together they read as
red-to-yellowish-green cycling. That is correct behaviour, not a fault.

---

## 5. Step by step — Mode B, standalone boot

The board boots MedSight from external flash with nothing attached but power.

### 5.1 Understand the shape of it first

The STM32N6 has **no internal flash**. Its boot ROM copies a first-stage
bootloader (FSBL) from external flash into AXI SRAM2 — and that copy is
**capped at 512 KB**. MedSight is ~880 KB, so it can never be an FSBL.

So this uses ST's two-stage **"load and run application"** mode:

| Stage | What | Address | Size |
|---|---|---|---|
| 1 | ST's prebuilt FSBL (`ai_fsbl.hex`) | `0x70000000` | ~30 KB |
| 2 | MedSight, signed `-t ssbl` | `0x70100000` | ~899 KB |
| — | NPU weights, untouched by the above | `0x70380000`+ | |

`0x70100000 + 899 KB = 0x701DB6E0`, comfortably short of the weights at
`0x70380000`. Nothing collides.

### 5.2 Get ST's FSBL

`ai_fsbl.hex` is **not in this repository** — it is ST's binary, not ours.
Download it from ST:

> **X-CUBE-N6-AI-POWER-MEASUREMENT**, v1.4.0 or newer → `FSBL/ai_fsbl.hex`

The copy this project was verified against:

```
sha256  8fa77dcdcb9aeed6e167c1ce5ffe9e16071fdeced8d13e22edd31a37abed1993
size    176,541 bytes
```

Any ST-provided N6 FSBL built on STM32Cube N6 FW ≥ 1.2.0 that supports boot
header v2.3 should work; that is the one that was tested.

### 5.3 Sign the application

From `sessions/session_17/STM32CubeIDE/FSBL`:

```powershell
Remove-Item -Force "Debug\MedSight_ssbl.bin" -ErrorAction SilentlyContinue
& $SIGN -bin "Debug\MedSight_Session17_FSBL.bin" -nk -t ssbl -hv 2.3 -la 0x34000000 -align -o "Debug\MedSight_ssbl.bin"
```

Every flag matters:

| Flag | Why |
|---|---|
| `-t ssbl` | **second**-stage. `-t fsbl` produces an image the ROM can never load, because of the 512 KB cap. This was the single most costly mistake made during bring-up. |
| `-hv 2.3` | boot header version the ROM and ST's FSBL expect |
| `-la 0x34000000` | load address — must match the linker script's ROM origin region |
| `-nk` | no signing key; the board is not in a secured state |
| `-align` | pad to the alignment the loader requires |

The tool prints an **entry point value**. Note it — it is how you tell two
builds apart later, and reading a shared magic number is *not* a substitute.

### 5.4 Flash both stages

```powershell
& $CLI -c port=SWD mode=UR -el $LOADER -w "<path>\ai_fsbl.hex"
& $CLI -c port=SWD mode=UR -el $LOADER -w "Debug\MedSight_ssbl.bin" 0x70100000 -v
```

`ai_fsbl.hex` carries its own addresses, so it takes no address argument.

**Use `-v`.** Verify the whole file. Do not check a few bytes of a magic number
and call it confirmed — during bring-up that produced a "verified" write that
had not happened, because both candidate images shared those bytes.

**Then unplug the cable** (§2).

### 5.5 Move the boot switch

| Switch | Position | Meaning |
|---|---|---|
| **SW1 (BOOT1)** | **LOW** | boot from external flash |
| **SW2 (BOOT0)** | **LOW** | |

### 5.6 Power it

Plug the USB-C into a power bank or any 5 V USB supply. The board boots
MedSight on its own. Register a patient, request a dose, and it dispenses —
this has been done on battery with no laptop in the room.

**To get back to Mode A**, set SW1 HIGH again. Nothing is erased and nothing
needs re-flashing.

---

## 6. Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Boot log stops after `ai_vision_init: DEBUG build (-O0).`, LCD frozen | flash bus state after an external-loader operation | **unplug the cable** (§2) |
| Networks initialise, outputs are nonsense | weights missing or at the wrong address | §4.3, then `weights/README.md` |
| `HOTPLUG` connection fails, or erase fails | MedSight is running and owns the XSPI2 bus | use `mode=UR` |
| Nothing boots in Mode B | image signed `-t fsbl`, or at the wrong address | re-sign `-t ssbl`, write to `0x70100000` |
| `loaded 0 patient(s)` | SD card not seated, or a record lost | reseat the card; re-register |
| Stepper does nothing, driver LEDs dark | ground not shared, or wrong pins | `HARDWARE_WIRING.md` §3 and §5 |
| `DISPENSE_JAM` immediately, motor never moved | coil pins not driving | `HARDWARE_WIRING.md` §5 |

### A known intermittent, stated rather than hidden

The SD card occasionally fails `HAL_SD_Init` at boot and recovers on a retry.
It predates this session's work and was not investigated. If a boot comes up
with no patients, reseat the card and reboot.

---

## 7. Running with **no** dispensing hardware

Fully supported, and it is how a reviewer without a stepper motor should build
this.

In `sessions/session_17/FSBL/Inc/dispenser.h`:

```c
#define MEDSIGHT_PHYSICAL_DISPENSER 0
```

Rebuild. The entire application runs — face recognition, scheduling, the UI,
the audit log — with the dispense simulated exactly as it was before physical
hardware existed. **Both values build with 0 errors and 0 warnings**, and both
are verified configurations rather than one real path and one stub.

---

## 8. Other knobs worth knowing

All are plain macros in the sources. No `.ioc`, no regeneration.

| Macro | File | Default | Effect |
|---|---|---|---|
| `MEDSIGHT_PHYSICAL_DISPENSER` | `Inc/dispenser.h` | `1` | `0` = no hardware needed (§7) |
| `MS_COIL_PINSET` | `Src/dispenser.c` | `1` | `1` = `D3/D5/D6/D9`. `0` = the CN7 analog set, which **does not work** on this board and is kept only as a record |
| `MS_DISPENSE_DIRECTION` | `Src/dispenser.c` | `1` | `-1` reverses the turntable, for a mirrored hopper |
| `MS_NO_PILL_TIMEOUT_MS` | `Src/dispenser.c` | `25000` | how long with no counted pill before JAM/SHORT |
| `MEDSIGHT_BUZZER_KEYBOARD_CLICK` | `Inc/buzzer.h` | `0` | `1` ticks on every keypress |
| `MS_XSPI_TRACE_ON` | `Drivers/BSP/.../_xspi.c` | `0` | `1` traces external-flash bring-up |
| `MS_BOOT_LED_CHECKPOINTS` | `Src/main.c` | `0` | `1` blinks progress through boot |

---

## 9. What a full demo looks like

1. Boot. Wait for the gallery to load.
2. **Register a patient** — the camera captures a face embedding to the SD card.
3. **Set a dose schedule.**
4. **Face match** → the patient is recognised.
5. **Request the dose** → the turntable turns, the IR beam counts each pill,
   and the on-screen progress bar steps **once per counted pill** — it is a
   readout of the sensor, not a timer.
6. The buzzer sounds two short beeps on success.
7. The audit log records one line, after the fact:
   `DISPENSE: <patient> <n> requested, <m> counted (OK)`

`documents/DEMO_SCRIPT.md` has the presentation version of this.

### If a dispense goes wrong

Both outcomes are normal, neither is an error, and neither calls
`Error_Handler()`:

- **`DISPENSE_JAM`** — 25 s with nothing counted at all. Nothing was released.
- **`DISPENSE_SHORT`** — some pills, then 25 s of silence. **The hopper needs
  refilling.**

The audit log always records the count that *actually came out*, not the count
that was requested.

---

## 10. Where to read next

| Document | What it is |
|---|---|
| `documents/HARDWARE_WIRING.md` | the fourteen wires, with a diagram |
| `weights/README.md` | the NPU blobs, addresses and fingerprints |
| `documents/SOFTWARE_ARCHITECTURE.md` | tasks, states, module boundaries |
| `documents/HARDWARE_ARCHITECTURE.md` | pin map and peripheral budget |
| `documents/milestones/session_17_notes.md` | how standalone boot was solved, and what is still untested |
| `documents/DEMO_SCRIPT.md` | the guided walkthrough |

---

## Honest notes for an evaluator

- **Reliability is under-measured.** Four dispenses were run (5/5, 3/3, 3/3,
  2/2 — 13 of 13 pills counted correctly). Four trials cannot distinguish a
  100% device from a 95% one, and the notes say so.
- **No jam was ever deliberately induced.** The 25 s bound is reasoned from
  measured break durations, not exercised against a physically blocked
  mechanism.
- **The buzzer was confirmed by ear only.** No capture exists.
- **One hopper.** The 6–8 hopper architecture in `MECHANICAL_DESIGN.md`
  remains design intent.
- **No networking, by permanent project rule.** Nothing here phones home, and
  no biometric data leaves the SD card.
