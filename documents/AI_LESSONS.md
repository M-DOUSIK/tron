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

### 2b. The four addresses this project actually uses

Every one of these was found the hard way, and none of them is discoverable
from the linker script alone. Flash all four on a fresh board.

| Address | Bytes | What lives there | Source of the blob |
|---|---|---|---|
| `0x70380000` | 2,030,881 | CenterFace (face detector) constant pools | generated with the face networks, Session 08B |
| `0x71000000` | 296,832 | NPU epoch-controller blobs, **both** face networks | linked `(NOLOAD)`; extract from the `.elf` |
| `0x72000000` | 1,092,129 | MobileFaceNet (embedder) constant pools | generated with the face networks, Session 08B |
| `0x73000000` | 3,221,233 | **hand landmark weights (Session 16 final)** | `tools/action_recogntion/build/generated_hand/hand_atonbuf.xSPI2.bin` |
| `0x73400000` | 3,049,169 | **pill detector weights (Session 16 final)** | `tools/action_recogntion/build/generated_pill2/pill_atonbuf.xSPI2.bin` |

> **The pill detector moved from `0x73000000` to `0x73400000`, and its
> activations moved out of `AI_ARENA` into PSRAM.** Both were forced by the
> hand model, which needs the whole 220 KB arena and 3.2 MB at `0x73000000`.
> Rather than choose between the two networks, the pill detector was
> regenerated against a memory pool that excludes AXISRAM6 entirely and places
> its 208 KB of activations at `0x90A00000` in PSRAM. Nothing is shared:
>
> | | activations | weights |
> |---|---|---|
> | hand landmarks | 220 KB `AI_ARENA` + ~978 KB PSRAM `0x90500000` | `0x73000000` |
> | pill detector | 208 KB, all PSRAM `0x90A00000` | `0x73400000` |
>
> The pill detector pays for coexistence by running out of PSRAM, which is
> slower — and it is the stage that can afford it, because it only
> corroborates and runs on one frame in four.
>
> **One generation flag had to be dropped for it**: `--Ocache-opt` makes the
> Neural Art compiler abort with an internal assertion
> (`check_npu_caching_of_output_live_buffers`) when every activation lives in
> a cacheable external pool. The profile `medsight-pill2` omits it. That is a
> compiler limitation, not a configuration error, and it costs only an
> optimisation pass.

The first three are Session 08B's and are documented above. The fourth is
Session 16's and has its own runbook, below, because regenerating it is a
three-step process with two traps in it.

### 2c. Regenerating and flashing the pill detector (Session 16)

The weights at `0x73000000` come from an ONNX model that has been through
`export -> cut head -> INT8 quantise`. `tools/action_recogntion/summary.md`
and `build_pill_detector.py` cover the model side; this is the device side.

**Step 1 — generate the NPU sources.** Note the `--st-neural-art` argument:
it is a *profile reference*, not a bare flag.

```bash
cd tools/action_recogntion
"C:/ST/STEdgeAI/4.0/Utilities/windows/stedgeai.exe" generate \
    --model models/pill_detector/pill_cut_int8_160_hn.onnx \
    --target stm32n6 --name pill --no-report \
    --st-neural-art "medsight-arena@medsight_neuralart.json" \
    --workspace build/ws_hn --output build/generated_hn
```

> **TRAP 1 — the memory pool must come through the profile.** Plain
> `--st-neural-art`, with no profile, uses ST's default pool, which places
> activations at `0x342E0000` — *inside the face networks' scratch*. Nothing
> warns you. The symptom is face recognition degrading after an intake watch.
> `--memory-pool` is **not** the right flag either; it is silently ignored for
> this target. Only the `profile@config.json` form works.

Verify placement before believing the output — do not skip this:

```bash
python -c "import json;d=json.load(open('build/generated_hn/pill_c_info.json'));\
print(d['memory_footprint'])"
```

Expect `activations` at or under **225,280** (the `AI_ARENA` size) and the
first memory pool's `address` to read **876118016** — that is `0x34388000` in
decimal, AXISRAM6, which is what `medsight_arena.mpool` asks for.

**Step 2 — install the generated sources.** Four files, and the firmware must
be rebuilt even when only the weights changed, because the INT8 quantisation
scales are compiled into `stai_pill.h`:

```bash
G=tools/action_recogntion/build/generated_hn
F=sessions/session_16/FSBL
cp $G/pill.h $F/Inc/pill.h
cp $G/stai_pill.h $F/Inc/stai_pill.h
cp $G/pill.c $F/Src/ai/pill.c
cp $G/stai_pill.c $F/Src/ai/stai_pill.c
```

`FSBL/Src/ai` is a **type-2 (folder) link** in `.project`, so new `.c` files
there are picked up automatically. `FSBL/Src/ui` is not — a file added there
needs its own `<link>` entry, which is Session 09's Bug 1.

**Step 3 — flash the weight blob.**

```bash
cp $G/pill_atonbuf.xSPI2.raw $G/pill_atonbuf.xSPI2.bin
```

> **TRAP 2 — the `.raw` extension is rejected outright.**
> `STM32_Programmer_CLI` answers *"the download command ... has a wrong
> extension, please note that the supported extension are .bin, .hex,
> .srec"*. ST Edge AI emits `.raw`; copy it to `.bin`. This is also the
> convention the two face networks already follow
> (`fd_data.xSPI2.bin`, `faceid_data.xSPI2.bin`).

```powershell
$cli = "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"
$loader = "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\ExternalLoader\MX66UW1G45G_STM32N6570-DK.stldr"
& $cli -c port=SWD mode=HOTPLUG -el $loader -w "<...>\pill_atonbuf.xSPI2.bin" 0x73000000
```

**Step 4 — verify the write, then power-cycle.** Reading back costs seconds
and distinguishes "the flash is wrong" from "the code is wrong" later:

```powershell
& $cli -c port=SWD mode=HOTPLUG -el $loader -r32 0x73000000 8
```

Compare against the file's own first words:

```bash
python -c "import struct;print('0x%08X 0x%08X' % struct.unpack('<II', \
    open('$G/pill_atonbuf.xSPI2.bin','rb').read(8)))"
```

Then **unplug the USB cable** — a full power cycle, not a debugger reset —
before running the application. Back-to-back external-loader operations leave
the flash chip's live bus state confused; see section 3 immediately below.

Finally, rebuild the firmware in the IDE. A stale build against new weights
produces plausible-looking but wrong detections, because the quantisation
scales no longer match the tensors they are decoding.

### 3. If the board hangs at boot after ANY external-memory operation

> **Session 16 correction: this applies to READS as well as writes.** The
> heading used to say "after flashing". It happened again after two `-r32`
> verification reads on a live board — the log stopped right after
> `ai_vision_init: DEBUG build (-O0)`, before `HAL_CACHEAXI_Enable`, which is
> where XSPI is first touched, and the LCD held a stale frame. Verifying a
> write is exactly when one is most tempted to leave the board running.
> Power-cycle after **any** external-loader operation.

A board that boots fine, then hangs (often inside a `HAL_XSPI_GET_FLAG`
polling loop) right after flashing OSPI content — even though the flash
write itself reported success — is very likely the flash chip's live bus
state getting confused by back-to-back external-loader operations without
a reset in between. **Do a full power cycle** — unplug the USB cable
entirely (not a software/debugger reset), wait a few seconds, plug it back
in. The flash *content* survives (non-volatile); only the peripheral's live
bus state needed clearing. Seen and fixed exactly once, in Session 08B.
