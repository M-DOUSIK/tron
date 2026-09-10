# MEMORY_MAP.md — where every byte on this board actually lives

**Written:** Session 15, Part B4.
**Sources, in order of authority:** the Cube.AI-generated memory-pool
declarations in `FSBL/Src/ai/fd.c` and `FSBL/Src/ai/faceid.c` (these are
generated *from* the tool's own view of the part, and they are the only
in-tree statement of what the NPU can reach); the linker script; the `.map`
and `arm-none-eabi-size -A` output of a real build; the ST HAL headers.
**Not** derived from any earlier document in this repository, including the
session prompts — several of those carried numbers that turned out to be
stale, and the point of this exercise was to stop guessing.

## Why this document exists

Knowing that ~2.7 MB of SRAM is idle is worth very little on its own. The
deliverable Session 15 was asked for is memory that is **claimed,
addressable, proven on hardware, and documented well enough that a future
session can drop a model into it without repeating this work.** §5 is that
runbook.

---

## 1. The region table

### 1a. On-chip AXI SRAM

The part has ~3.75 MB of AXI SRAM in six banks. The N6 data brief's "4.2 MB
contiguous" figure includes the NPU cache RAM and FLEXRAM, which no part of
this application uses.

| Bank | Address range | Size | Clocked/powered by | NPU can reach it? |
|---|---|---|---|---|
| AXISRAM1 | `0x34000000`–`0x340FFFFF` | 1024 KB | always on | **No** |
| AXISRAM2 | `0x34100000`–`0x341FFFFF` | 1024 KB | always on | **No** |
| AXISRAM3 | `0x34200000`–`0x3426FFFF` | 448 KB | `stm32n6xx_hal_msp.c` (DCMIPP MspInit) | Yes — `npuRAM3` |
| AXISRAM4 | `0x34270000`–`0x342DFFFF` | 448 KB | `stm32n6xx_hal_msp.c` (DCMIPP MspInit) | Yes — `npuRAM4` |
| AXISRAM5 | `0x342E0000`–`0x3434FFFF` | 448 KB | **`npu_init.c`'s `SystemInit_POST()` only** | Yes — `npuRAM5` |
| AXISRAM6 | `0x34350000`–`0x343BFFF7` | 448 KB (less 8 bytes) | **`npu_init.c`'s `SystemInit_POST()` only** | Yes — `npuRAM6` |

The bank boundaries and sizes are taken verbatim from the generated pool
declarations, e.g. `fd.c` line 68:

```
/* index=1 file postfix=AXISRAM5 name=npuRAM5 offset=0x342e0000
   absolute_mode size=458752 READ_WRITE ... */
```

and the four-bank virtual pool at line 76:

```
/* index=8 file postfix=AXISRAM3_AXISRAM4_AXISRAM5_AXISRAM6
   name=npuRAM3_npuRAM4_npuRAM5_npuRAM6 offset=0x34200000
   absolute_mode size=1835000 vpool READ_WRITE ... */
```

**The "clocked/powered by" column is the trap in this whole table**, and it is
the reason the arena self-test is where it is. See §3.

### 1b. External memory

| Region | Address | Size | Contents | Wired up by |
|---|---|---|---|---|
| OSPI2 NOR (`xspi2`, "octoFlash") | `0x71000000` | 64 MB mapped, ~290 KB used | NPU epoch-controller blobs, linked `(NOLOAD)` and flashed by hand | `BSP_XSPI_NOR_Init` in `aiPreInitialize()` |
| OSPI2 NOR — `fd` weights | `0x70380000` | 28 MB window declared, a fraction used | CenterFace constant pools | flashed separately (see `AI_LESSONS.md`) |
| OSPI2 NOR — `faceid` weights | `0x72000000` | 32 MB window declared | MobileFaceNet constant pools | flashed separately |
| HyperRAM / PSRAM (`xspi1`) | `0x90000000` | 16 MB | MobileFaceNet activation scratch | `BSP_XSPI_RAM_Init` + memory-mapped mode |

The three external addresses come from `session_08B_notes.md` Addenda 2 and 4
and are confirmed against the generated sources' `offset=0x…` comments.

### 1c. What the linker claims, after Session 15

```
ROM      (xrw)  ORIGIN = 0x34100000   LENGTH = 1024K   -- all of AXISRAM2
RAM      (xrw)  ORIGIN = 0x34000400   LENGTH = 1023K   -- AXISRAM1 less 1 KB
AI_ARENA (xrw)  ORIGIN = 0x34388000   LENGTH =  220K   -- top of AXISRAM6
OSPI_NOR (rx)   ORIGIN = 0x71000000   LENGTH =   64M   -- (NOLOAD)
```

### 1d. What is at a fixed address in code rather than in the linker script

These are the ones that do not appear in the linker script at all and will not
show up in a `.map` file. Every one of them is a hard-wired absolute address.

| What | Address | Size | Declared in | Hard-wired or a choice? |
|---|---|---|---|---|
| Camera / LCD framebuffer (`BUFFER_ADDRESS`) | `0x34200000` | 768,000 B | `FSBL/Inc/main.h` | A **choice** — but one that deliberately aliases the NPU's pool, see §2 |
| `GUI_BUFFER_ADDRESS` | `0x342BB800` | 768,000 B | `FSBL/Inc/main.h` | A choice, and **currently unused** — Session 13 proved it unusable, see §2 |
| `fd` + `faceid` activations | `0x34200000`–`0x34387FFF` | 1,605,632 B | generated `fd.c` / `faceid.c` | **Hard-wired** by the ST Edge AI codegen. Changing it means re-running the tool. |

---

## 2. The NPU's real footprint, and the framebuffer it overlaps

The two networks' activation addresses were extracted mechanically from every
address literal in `fd.c` and `faceid.c` — 642 references across
`LL_ATON_Cache_MCU_Clean_Range`, `LL_ATON_Cache_MCU_Invalidate_Range`,
`start_offset` and `addr_base` — and merged:

```
0x34200000 - 0x34387FFF     1,605,632 bytes, ONE contiguous block
```

Three things follow, and each of them settles a question this project has
been carrying:

**a) The framebuffer sits inside the NPU's activation space, on purpose.**
`BUFFER_ADDRESS` is `0x34200000`, which is exactly where the pools start.
That is why the UI must not draw between `ai_vision_capture_request()` and
`ai_vision_capture_wait()`, and why `state_machine.c` blanks the LTDC layer
for the capture window instead. It was always described as "the activation
scratch overlaps the framebuffer"; the extent is now known.

**b) Session 13's second framebuffer could never have worked.**
`GUI_BUFFER_ADDRESS` is `0x342BB800` and runs to `0x34376FFF` — entirely
inside the block above. Session 13's canary reported both ends clobbered on
the first capture and the feature was reverted; this is the arithmetic that
explains it, and it means the revert was correct rather than cautious. Note
also that Session 13's `ms_configure_sleep_clocks()` comment said the NPU
scratch "only ever overlaps `BUFFER_ADDRESS`" — **that was wrong**, and it is
corrected in `main.c` by this session.

**c) Everything above `0x34388000` is genuinely free.** That is the arena.

---

## 3. `AI_ARENA` — address, size, symbols, and the two preconditions

```
_ai_arena_start = 0x34388000
_ai_arena_end   = 0x343BF000
size            = 0x37000 = 225,280 bytes (220 KB)
guard band      = 0x343BF000 .. 0x343BFFF8 (4,088 bytes, deliberately unclaimed)
section         = .ai_arena, (NOLOAD)
```

The guard band exists because `npuRAM6`'s own declaration stops 8 bytes short
of the bank boundary (`size=458744`, not `458752`) and there is no in-tree
explanation of why. Rather than have the arena's last page depend on an
unexplained 8 bytes, it ends a page early. If a future session needs those
4 KB, the thing to do is find out what the 8 bytes are for first.

### Precondition 1 — AXISRAM5 and AXISRAM6 are NOT powered at reset

This is the single most important fact in this document.

`FSBL/Src/stm32n6xx_hal_msp.c` enables **AXISRAM3 and AXISRAM4 only**
(`LL_MEM_EnableClock` plus `HAL_RAMCFG_EnableAXISRAM`). AXISRAM5 and AXISRAM6
are brought up by `SystemInit_POST()` inside `FSBL/Src/ai/npu_init.c`, which
runs from `aiPreInitialize()` — on the **AI task**, after the scheduler has
started.

So between reset and the AI task finishing its init, the arena is memory that
the CPU can address and that does not exist. It does not fault; it returns
whatever an unclocked bank returns. **That is exactly the shape of result
that gets written into a document as a fact.**

Consequence, and it is enforced in the code: `ms_memtest_arena()` is called
from `state_machine_init()` immediately after `ai_vision_wait_init()` returns,
and from nowhere else.

### Precondition 2 — `LPEN`, and why nothing new was needed

`session_12_notes.md` Addendum 9: `WFI` on this part enters CSleep, which
stops the clock of every memory bank whose `RCC_MEMLPENR` bit is clear. A
buffer in a gated bank, touched by anything other than the CPU, produces a
silent intermittent fault that six rounds of register-dumping cannot see.

Checked for the arena, both halves:

- `ms_configure_sleep_clocks()` in `main.c` already sets `AXISRAM3LPEN`
  through `AXISRAM6LPEN`. The arena is in AXISRAM6, so it is covered.
- `Set_CLK_Sleep_Mode()` in `npu_init.c` independently sets the sleep-clock
  bits for **all six** AXISRAMs plus FLEXRAM and the cache RAM, later in boot.

**No new `LPEN` bit was required by Session 15**, and the reason is recorded
rather than assumed. The rule still stands for whoever comes next:

> Anything that puts a buffer outside AXISRAM3–6, or adds a DMA master that
> writes into one, must add its `LPEN` bit to `ms_configure_sleep_clocks()`
> **in the same change**, and must be tested from a **cold boot** — the only
> condition under which the original fault ever appeared.

Note that AXISRAM1 and AXISRAM2 — where all the code, `.rodata` and `.bss`
live — are **not** in `ms_configure_sleep_clocks()`'s set, and are only
covered later by `npu_init.c`. That is currently harmless because no DMA
master reads from them: the SD path uses polling `HAL_SD_*` calls, so the CPU
is awake throughout. **If anything ever DMAs to or from a `.bss` buffer, this
becomes a live bug.**

### The proof

`FSBL/Src/ms_memtest.c` fills the whole arena and reads it back three times,
from a cold boot, and prints the byte count:

1. `0xA5A5A5A5` — every bit set one way
2. `0x5A5A5A5A` — and the other
3. **address-in-address** — the only pattern that catches an aliased or
   short-decoded bank, where a write lands somewhere else and both constant
   patterns still "pass"

Each pass does a `SCB_CleanInvalidateDCache_by_Addr()` between the write and
the read, because the arena is ordinary cacheable memory and a write-back
cache would otherwise satisfy every read from L1 and let an unpowered bank
pass. That is the same asymmetry Addendum 9 is about: a measurement taken by
the CPU can be structurally unable to see the thing being measured.

Gated by `MEDSIGHT_ARENA_SELFTEST` (default 1). The result appears over UART
as `ARENA SELFTEST PASS: 225280 bytes writable and readable` or a FAIL line
naming the first bad word and its address.

> **Hardware status: PASSED, from a cold boot.**
>
> ```
> ARENA SELFTEST: testing 0x34388000-0x343BEFFF (225280 bytes)...
> ARENA SELFTEST PASS: 225280 bytes writable and readable (3 patterns, 28ms).
> ```
>
> All three patterns, including address-in-address, over the full region, on
> a genuinely cold boot with AXISRAM6 powered up by `aiPreInitialize()` on
> the AI task earlier in the same boot. 28 ms for 3 x 225,280 bytes of
> write-plus-cache-maintenance-plus-read is about 48 MB/s of round trips,
> which is the right order for this bus and is itself a sanity check that
> the accesses reached memory rather than a cache.
>
> **The arena is a resource, not a hypothesis.** Cite it.

---

## 4. Why `ROM` and `RAM` are the sizes they are

### Before

```
ROM  0x34180400  511K   -- the top half of AXISRAM2, less 1 KB
RAM  0x34000400 1023K   -- AXISRAM1, less the 1 KB the boot ROM owns
```

Both numbers came from the ST `DCMIPP_ContinuousMode` example this repository
was founded on in Session 03. Neither was ever derived from what MedSight
needs, and between them sat **513,024 bytes of AXISRAM2 that nothing has ever
claimed** — a hole the project has been carrying for twelve sessions.

Measured occupancy of the old regions, immediately before the change:

| | used | of | | free |
|---|---|---|---|---|
| Debug ROM | 474,080 | 523,264 | **90.6%** | 48 KB |
| Debug RAM | 802,304 | 1,047,552 | 76.6% | 239 KB |
| Release ROM | 342,720 | 523,264 | 65.5% | 176 KB |
| Release RAM | 786,944 | 1,047,552 | 75.1% | 254 KB |

**Debug ROM at 90.6% is the finding.** Forty-eight kilobytes is roughly one
feature away from a link failure, and a link failure at 3 a.m. before a
deadline is not a good way to discover a linker script nobody re-derived.

### After

```
ROM  0x34100000 1024K   -- ALL of AXISRAM2, ending exactly at the framebuffer
RAM  0x34000400 1023K   -- unchanged; AXISRAM1 has nothing more to give
```

`ROM` absorbs the whole 501 KB hole. `RAM` is untouched — AXISRAM1 is
physically 1 MB and the first 1 KB belongs to the boot ROM, so 1023 KB is
already all of it, and at 77% full it is not the constraint anyway.

Measured occupancy after, on the same build:

| | used | of | | **headroom** |
|---|---|---|---|---|
| Debug ROM | 496,000 | 1,048,576 | 47.3% | **539 KB** |
| Debug RAM | 812,680 | 1,047,552 | 77.6% | **229 KB** |
| Release ROM | 356,736 | 1,048,576 | 34.0% | **675 KB** |
| Release RAM | 797,312 | 1,047,552 | 76.1% | **244 KB** |

(The used figures include everything Session 15 itself added: carer mode, the
RTC time source, the schedule engine and the arena self-test — about 22 KB of
ROM and 10 KB of RAM in Debug.)

### What did NOT change, and was checked

`.rodata` stays in `RAM` (AXISRAM1), where the ST template put it. It is
tempting to move it into the now-roomy `ROM` region, and it would work,
because the CPU reaches both. It was left alone because **the NPU cannot
reach either bank**, so moving it buys nothing that matters and the one
lesson this project has about `.rodata` placement
(`AI_LESSONS.md`: `.rodata` in AXISRAM1 hard-faulted the NPU, fixed by moving
the *weights* to external flash) is a reason to leave a working arrangement
working.

The `.xspi2` external-flash layout was verified byte-for-byte unchanged after
the linker edit, in **both** configurations, against the Session 13 Debug
ELF — `ENGINEERING_LESSONS.md` requires this whenever anything about the
model files or their compilation changes, and moving the ROM origin is close
enough to count:

```
$ arm-none-eabi-nm -n <elf> | grep '^71' | head -4
71000000 b _ec_blob_faceid_1
71000700 b _ec_blob_faceid_6
71000c40 b _ec_blob_faceid_10
710016c0 b _ec_blob_faceid_14      <- identical in s13 Debug, s15 Debug, s15 Release
```

The already-flashed weight image therefore still serves this build. Nothing
needs re-flashing to external memory for Session 15.

---

## 5. Runbook — how to add a third model

Written so that the next person does not repeat Session 15's archaeology.

**1. Weights go to external OSPI NOR, never to internal SRAM.**
Tag them `__attribute__((section(".xspi2")))`; the linker script maps that to
`0x71000000` and marks it `(NOLOAD)`. Flash them separately with
`STM32_Programmer_CLI` in `HOTPLUG` mode and the `MX66UW1G45G` external
loader — the exact commands are in `AI_LESSONS.md`. Then **verify the layout**
against the flashed image with `arm-none-eabi-nm -n … | grep '^71' | head`; a
different first symbol means the flash is wrong for this binary, full stop.
Also check the generated `.c` for `offset=0x…` comments: Session 08B needed
weights at **three** separate addresses and the extra two only appeared in
those comments.

**2. Activations go in `AI_ARENA`, and you must tell the tool that.**
`0x34388000`, 220 KB. Give ST Edge AI an explicit memory-pool description
naming that address and size rather than letting it default — the default
puts activations at `0x34200000`, which is the framebuffer, which is why the
existing two networks alias it.

**3. Check reachability before anything else.** The NPU's data masters do
**not** connect to AXISRAM1 or AXISRAM2 (`AI_LESSONS.md`: a `BUSIF0 ERR:
0x1f` hard fault). Only AXISRAM3–6 and the external XSPI regions are usable
by a model. Memory the CPU can address is not automatically memory a model
can run from.

**4. Power and clocks.** If the arena is where it is, nothing new is needed —
`SystemInit_POST()` powers AXISRAM5/6 and `ms_configure_sleep_clocks()` plus
`Set_CLK_Sleep_Mode()` cover the `LPEN` bits. If you move it, redo §3's two
preconditions from scratch and **test from a cold boot**.

**5. Run the arena self-test first, on the actual board**, before writing a
line of model integration. `MEDSIGHT_ARENA_SELFTEST=1`, look for the PASS
line and the byte count. An arena nobody has written to is a hypothesis.

**6. Budget for the toolchain, not for the memory.** Sessions 08A, 08B and 12
between them lost most of three sessions to: an NPU bus fault from a bad
`.rodata` placement, an IDE lock-up caused by the external loader under
`connect_under_reset`, weights needed at three addresses, and Debug and
Release emitting the same blobs in *opposite order* (fixed by
`-fno-toplevel-reorder` on Release — keep that flag). None of these are
memory problems and all of them will recur.

**7. Where it must go in the task set.** A third network runs on the existing
`ai` task at priority 3, below the UI, using the same event-flag handshake
(`ai_vision.h`). Do not create a second AI task; the frame-buffer ownership
argument in `SOFTWARE_ARCHITECTURE.md` §9 depends on there being exactly one
owner of `BUFFER_ADDRESS` at a time.

---

## 6b. Does a third model fit? — the verdict, corrected

**Written with §1–§6 in hand, and corrected after the first hardware round.**

### The correction

An earlier revision of this project's answer — in
`PROGRAM_PLAN_RECONCILIATION.md`, in the README, and in the first draft of
these notes — said a temporal model could not fit because an 8-frame ring at
128×128×3 is 393 KB against a 220 KB arena.

**The arithmetic is right and the premise was wrong.** The frame ring never
had to be in SRAM. §1b of this document lists 16 MB of **NPU-reachable
external PSRAM at `0x90000000`**, declared `READ_WRITE` in *both* generated
networks' own memory-pool tables (`hyperRAM`, `size=16777208`), and
MobileFaceNet already uses it as activation scratch. It was in the table and
it was not carried into the budget.

### So what is actually available

| | |
|---|---|
| ROM for generated forward-pass code | 536 KB free (Debug), 674 KB (Release) |
| Weights | external NOR at `0x71000000`, 64 MB mapped, ~290 KB used |
| Fast activation working set | `AI_ARENA`, 220 KB, **proven** (§3) |
| Frame ring / bulk buffers | PSRAM at `0x90000000`, 16 MB, NPU-reachable |

On memory, **both** a small pill classifier and a small action-recognition
model fit. Note the PSRAM's own declaration though: `THROUGHPUT=MID`,
`LATENCY=HIGH`, against `THROUGHPUT=HIGH LATENCY=LOW` for the SRAM banks. Put
the frame ring there and the activation working set in `AI_ARENA`, not the
other way round, and budget for the inference being slower than the 209 ms
the face pipeline manages out of SRAM.

### The real constraint is the camera

The DCMIPP DMAs into `BUFFER_ADDRESS`, which is the display framebuffer, and
the camera is **stopped** for the entire dispense flow after the face capture
— Session 09 found that resuming it lets the DMA continuously overwrite
whatever the UI has drawn, which is why every screen from the capture onward
is static UI rather than a preview.

Action recognition needs the camera running *while the UI keeps drawing*. That
means:

1. Pointing the DCMIPP at a destination that is not the framebuffer — PSRAM
   is the obvious one, and it is where the ring wants to be anyway.
2. Re-deriving §3's `LPEN` question for that destination **from scratch**. A
   DMA master writing into a bank whose clock stops on `WFI` is precisely the
   fault that cost six rounds of debugging on the display, and PSRAM's path
   (XSPI1) is not in `ms_configure_sleep_clocks()`'s set at all today.
3. Deciding what happens to the ISP task, which currently assumes one pipe
   into one buffer.

**That is the open engineering question, and it is a firmware one rather than
a model one.** Solve it before integrating anything.

### The remaining honest caveat

This project has lost most of three sessions to NPU toolchain problems that
had nothing to do with memory — a bus fault from a bad `.rodata` placement, an
IDE lock-up under the external loader, weights needed at three separate flash
addresses, and Debug and Release emitting the same blobs in opposite order.
§5's runbook exists so that history is not repeated, but it does not make the
work free.

## 8. What Session 16 actually put in the arena

`AI_ARENA` was claimed and pattern-tested in Session 15 with no consumer. It
has one now, and every number below is from `stedgeai analyze`, not arithmetic.

| | Bytes | Where |
|---|---|---|
| Pill detector activations, INT8 @160 | **208,000** | `AI_ARENA`, `0x34388000` — **17,280 spare** |
| Pill detector weights, INT8 | **3,049,169** | external OSPI NOR, `0x73000000` |
| Camera frame for action recognition | 768,000 | PSRAM, `0x90400000` |

### Why 160 and not the collaborator's 320

Measured, at three input sizes:

| Input | Activations |
|---|---|
| 320×320 FP32 (as delivered) | 4,505,600 |
| 192×192 INT8, head cut | 267,264 — **42 KB over the arena** |
| 160×160 INT8, head cut (shipped) | **208,000 — fits** |

The arena cannot be grown to take 192: AXISRAM6 ends at `0x343BFFF7`, capping
a region from `0x34388000` at 229,368 bytes. PSRAM would have fitted it and was
rejected on **latency**, not capacity — `THROUGHPUT=MID LATENCY=HIGH` against
the arena's `HIGH/LOW`, and the intake state machine's thresholds are frame
counts calibrated to ~30 fps. 160 costs nothing measurable: 84/90 detections on
held-out images, matching FP32 at 192, because this device feeds a tight
mouth-centred ROI rather than a whole scene.

### §5's runbook was right, with one correction

Step 2 said to give ST Edge AI an explicit memory-pool description "rather than
letting it default", and it was right — left to default the tool placed the
activations at **`0x342E0000`**, inside the block the two face networks occupy.

**The mechanism has moved, though.** With `--st-neural-art` the pool comes from
a *profile* in a `user_neuralart.json`, and the CLI's own `--memory-pool` flag
applies to a different path and is silently ignored. The working form is:

```
stedgeai generate --model <int8.onnx> --target stm32n6 --name pill   --st-neural-art "medsight-arena@medsight_neuralart.json"
```

where the profile names an `.mpool` with every pool zeroed except `npuRAM6`,
redefined as `0x34388000` / 220 KB, and `octoFlash` moved to `0x73000000`.
Both files are generated by `tools/action_recogntion/build_pill_detector.py`.
**Verify by reading the generated `pill.c`'s own pool comment** — it states the
offset it was built against, and that is the only thing that settles it.

### External NOR is now four regions, not three

| Address | Size | Contents |
|---|---|---|
| `0x70380000` | 2,030,881 | CenterFace constant pools |
| `0x71000000` | 296,832 | epoch-controller blobs (both face networks) |
| `0x72000000` | 1,092,129 | MobileFaceNet constant pools |
| **`0x73000000`** | **3,221,233** | **hand landmark weights (Session 16 final)** |
| **`0x73400000`** | **3,049,169** | **pill detector weights, relocated (Session 16 final)** |

> The figure above is the **hard-negative retrain** shipped late in Session 16
> (the first build's blob was 3,049,505 bytes). The graph is structurally
> identical — same six cut outputs, same tensor names, same 76,800-byte input
> — so only the weights and the INT8 quantisation scales changed. Those scales
> live in `Inc/stai_pill.h`, which is why a weight change still requires a
> firmware rebuild. Regeneration and flashing runbook: `AI_LESSONS.md` §2c.

`0x73000000` was chosen as the next clear 16 MB boundary above the highest byte
any existing region touches (`0x72106A61`). The pill detector was generated
**without** the epoch controller, so it adds no fifth blob at `0x71000000` and
the existing flashed image there is untouched.

### PSRAM is not empty, and here is its extent

Established the same way §2 established the AXISRAM one — by extracting every
address literal from the generated sources rather than trusting the pool
declaration, which claims all 16 MB:

```
0x90000000 .. 0x90310000    3,211,264 bytes   MobileFaceNet activation scratch
0x90310000 .. 0x91000000    12.94 MB          free
0x90400000 .. 0x904BB800      768,000 bytes   Session 16 camera frame
```

`0x90400000` is the next 4 MB boundary above the embedder, leaving ~0.9 MB of
unclaimed slack rather than butting against it.

**And PSRAM needed `LPEN` bits, which nothing in it ever had before**:
`XSPI1LPEN` and `XSPIMLPEN`, both added to `ms_configure_sleep_clocks()` in the
same change. §3's precondition 2 said this would be required for anything
outside AXISRAM3–6, and it was.

## 8b. Two NPU models in the intake path, and how they stopped colliding

Session 16 ended with **three** networks live, not two: CenterFace and
MobileFaceNet for the face, and — during an intake watch — a MediaPipe hand
landmark model with the pill detector alongside it.

The hand model does not fit in `AI_ARENA`. Its activations are 1,197,952
bytes and the arena is 225,280; the Neural Art compiler refuses outright,
`total bytes left unallocated=3515456`, with single buffers of 451,584 bytes.
It fits only because §1b's 16 MB of NPU-reachable PSRAM is declared
`READ_WRITE` in the pool — about 220 KB stays in the arena and about 978 KB
spills to `0x90500000`.

That left nothing for the pill detector, which had been using the same arena
and the same `0x73000000`. Rather than drop it, it was regenerated against a
pool that excludes AXISRAM6 completely:

| network | activations | weights | notes |
|---|---|---|---|
| CenterFace | overlaps `BUFFER_ADDRESS` (ST codegen) | `0x70380000` | why the UI may not draw during a capture |
| MobileFaceNet | PSRAM `0x90000000`–`0x90310000` | `0x72000000` | |
| epoch-controller blobs | — | `0x71000000` | both face networks |
| **hand landmarks** | 220 KB `AI_ARENA` + ~978 KB PSRAM `0x90500000` | `0x73000000` | decides the verdict |
| **pill detector** | 208 KB, all PSRAM `0x90A00000` | `0x73400000` | corroboration only |

PSRAM occupancy, in address order, so the next person adding a buffer can see
the gaps: MobileFaceNet `0x90000000`–`0x90310000`; the intake camera frame
`0x90400000` + 768,000 B; hand activations from `0x90500000`; pill activations
from `0x90A00000`. The bank runs to `0x91000000`.

**The cost is speed, and it is charged where it can be afforded.** A hand
inference measures 285–302 ms on hardware against ST's published 20.75 ms,
which is an all-internal figure — the difference is PSRAM at `THROUGHPUT=MID
LATENCY=HIGH`. Idle falls to 14–50% during a watch. The pill detector runs on
one frame in four for the same reason.

## 9. The linker's `.rodata` decision, reversed in Session 16

§4 recorded that `.rodata` was deliberately left in `RAM` because "the NPU
cannot reach either bank, so moving it buys nothing that matters". That was
correct when it was written and the premise changed.

Session 16's third network and its generated tables took the Debug **RAM**
region to **95.9% full, with 42 KB free** — which is Session 15's own
description of Debug ROM at 90.6%: roughly one feature away from a link
failure. `.rodata` moved to `ROM`, which had 517 KB spare.

| Debug | Before Session 16 | After, before the move | **After the move** |
|---|---|---|---|
| ROM | 47.3% | 50.7% | **75.5%** (257 KB free) |
| RAM | 77.6% | **95.9%** (42 KB free) | **63.8%** (379 KB free) |

Both regions are now healthier than Session 15 left them, with a third network
added. Two changes did it: the `.rodata` move, and deleting a 76,800-byte
staging image buffer by having the camera ROI written straight into the
network's own input.

**The move does not re-run `AI_LESSONS.md`'s bus fault.** That fault was the
NPU being unable to read *weights* placed in AXISRAM1. The NPU can reach
neither AXISRAM1 nor AXISRAM2, so moving ordinary CPU-read constant data
between them changes nothing the NPU sees, and every model weight in this
project lives in external OSPI NOR regardless.

## 7. One-page summary

| | Bytes | Notes |
|---|---|---|
| AXI SRAM on the part | ~3,932,160 | six banks |
| Claimed by the linker (`ROM`+`RAM`) | 2,096,128 | was 1,570,816 before Session 15 |
| Framebuffer | 768,000 | inside the NPU pools, deliberately |
| NPU activations, both networks | 1,605,632 | `0x34200000`–`0x34387FFF`, contiguous |
| **`AI_ARENA`, free and NPU-reachable** | **225,280** | `0x34388000`, proven by self-test |
| ROM headroom, Debug / Release | 552,576 / 691,840 | 47.3% / 34.0% used |
| RAM headroom, Debug / Release | 234,872 / 250,240 | 77.6% / 76.1% used |
| External NOR available | ~64 MB mapped | ~290 KB used by the two networks |
| External PSRAM available | 16 MB | used as `faceid` scratch |
