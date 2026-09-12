# Session 17 Notes — Physical Dispensing Hardware, and Standalone Boot

## Base folder, and why

**Built on `sessions/session_16/`, copied to `sessions/session_17/`.**

`prompts/session_17.md` says to take the highest-numbered `sessions/session_NN`
that has a matching `session_NN_notes.md` recording a completed,
hardware-verified run. At the time this session started:

- `ls sessions/` ended at `session_16`. There is no `session_14` and there
  never will be — it is a **retired number** (`MASTER_PROJECT_PLAN.md` v13).
- `ls documents/milestones/` ended at `session_16_notes.md`.
- `session_16_notes.md` records three hardware rounds (Addenda 3, 4 and 6),
  the action-recognition pipeline running end to end on the board, and both
  build configurations verified clean.

So Session 16 was both the highest-numbered folder and a completed,
hardware-verified one. Project identity was renamed to
`MedSight_Session17_FSBL` across `.project` (×3), `.cproject` and `.launch`,
per `ENGINEERING_LESSONS.md` and `session_16_notes.md` Addendum 5.

---

## Summary

Two things happened this session, and only one of them was on the plan.

**The planned one:** MedSight now physically dispenses. A 28BYJ-48 stepper
turns a turntable through a ULN2003A, an IR break-beam counts pills as they
pass, and a piezo buzzer gives the device a voice. `STATE_DISPENSING` drives
real hardware and reports the count that actually came out.

**The unplanned one, and the harder one:** the board boots MedSight from
external flash with no debugger attached, and runs a complete dispense from a
power bank. That took five distinct fixes and ten eliminated hypotheses, and
it is the longer half of this document.

| Part | Outcome | On hardware |
|---|---|---|
| **0** | Pins chosen against the live pin budget; beam polarity and break durations **measured**, not assumed | **VERIFIED** — idle HIGH confirmed 7×, breaks 17–46 ms |
| **A** | `dispenser.c/.h` — half-step table as atomic `BSRR` writes, EXTI0 counting with a measured chatter floor, one bounded wait, no `Error_Handler()` anywhere | **VERIFIED** |
| **B** | `STATE_DISPENSING` drives the hardware; the progress bar steps once per **counted** pill; JAM/SHORT have screens and audit entries | **VERIFIED** |
| **C** | `MEDSIGHT_PHYSICAL_DISPENSER 0` still builds and runs the Session 10 simulation untouched | **VERIFIED** (build); simulated flow not re-run on hardware this session |
| **D** | Five documents corrected — see the Part D section | n/a |
| **E** | Buzzer built as the Alert Task at priority 1, five patterns, rank-based pre-emption | **VERIFIED** by ear; **no UART capture exists** |
| **—** | **Standalone boot from external flash** | **VERIFIED** — dispense on power bank alone |

**The headline measurement the prompt asked for was not collected.** The
prompt specified 10 consecutive dispenses of 3 pills, recorded as a number.
Four dispenses were run: **5/5, 3/3, 3/3, 2/2 — 13 of 13 pills counted
correctly, 0 miscounts, 0 spurious jams.** That is 4 trials, not 10, and it is
stated as 4 rather than rounded up into a claim about reliability. See
*What is not done*.

---

## The deviation that shaped the session

The prompt laid out three phases: a standalone bench rig first, then
integration one peripheral at a time, then alerts and documentation.

**That was overridden by the project owner: "Screw the phases. Directly
integrate with the session 16 code."**

This is recorded because it was the right call and the reasoning is reusable.
The phased plan's value is isolating a new peripheral from a large working
system. But the peripherals had *already* been isolated — both the stepper and
the IR module had been independently proven on an ESP32 before this session
started. The uncertainty was never "does this motor turn", it was "does this
motor turn **from this board's pins**". A bench rig would have answered a
question that was already answered, and the real question — which turned out
to be a genuine hardware-level surprise (see §CN7 below) — only surfaces when
you drive the peripheral from the STM32.

The cost of skipping phases was real and should be recorded honestly: the CN7
failure was debugged inside the full application rather than in a 40-line test
program. It was still found in about an hour.

---

## Part 0 — the hardware, measured

### Pins

Full wiring is in **`documents/HARDWARE_WIRING.md`**, which is the document to
hand someone who is holding jumper wires. Summary:

| Function | Silkscreen | MCU | Domain |
|---|---|---|---|
| ULN2003 `IN1`–`IN4` | `D3` `D5` `D6` `D9` | PE9 / PE10 / PE13 / PE14 | VDDIO5 |
| IR `OUT` | `D2` | PD0 (EXTI0) | main VDD |
| Piezo `+` | `D10` | PA3 | main VDD |

All VddIO domains are already enabled unconditionally in `main()` (the Session
12 fix), so GPIOE needed no new `HAL_PWREx_EnableVddIOn()` call — but that was
*checked* rather than assumed, because a GPIO on an unpowered domain reads
back correctly and drives nothing, which is precisely the failure mode that
cost time on CN7.

`D14`/`D15` are the camera's I2C1 and were confirmed off-limits before
anything was assigned.

### The IR module, measured rather than assumed

Three numbers were taken off the bench with a histogram tool before any
threshold was chosen:

- **Idle level: HIGH.** Sampled across seven separate boots.
- **Real pill breaks: 17–46 ms.** This confirms the prompt's prediction that
  pills *slide down a ramp* rather than free-fall — a free-fall through a beam
  would have been a few milliseconds. The prompt was right and the code is
  sized for the slower number.
- **Contact chatter: 0–1 ms.** Comfortably separated from 17 ms.

`MS_IR_MIN_BREAK_MS` is therefore **8 ms** — above every chatter event
observed and less than half the shortest real break. It is a measured floor,
not a guessed one.

Boot-time polarity *learning* was tried first and abandoned: on one boot the
line read LOW for two full seconds before settling, and a single startup
sample got the polarity backwards, which turned the first dispense into an
instant JAM. Polarity is now pinned by `MS_IR_IDLE_FORCE 1` because it has
been measured. The auto-learning path survives behind
`MS_IR_IDLE_FORCE < 0` for a module that idles the other way.

### The step rate

`MS_STEP_PERIOD_MS` is **2 ms** per half-step. The 28BYJ-48 runs smoothly and
silently there under turntable load, and it leaves the dispense loop plenty of
slack to drain the break-duration ring between steps.

---

## Part A — the driver

`FSBL/Src/dispenser.c`, `FSBL/Inc/dispenser.h`.

Three decisions worth keeping:

**1. One atomic store per step.** The half-step table holds fully-formed
`BSRR` words rather than per-pin booleans, so advancing the motor is a single
32-bit write — no read-modify-write, so no way for an interrupt landing
mid-step to leave two coils energised in a combination the table never
contains. `COILS_OFF_BSRR` is written on *every* exit path, including the
error paths, so a stepper is never left drawing holding current after a failed
dispense.

**2. The ISR does timestamps, the task does decisions.** Both edges are
timestamped with `HAL_GetTick()` in `dispenser_exti_rising/falling`. Breaks
shorter than the measured chatter floor are discarded there; surviving
durations go into a 16-entry ring drained from task context. Nothing prints
from the ISR (`session_11_notes.md` Addendum 8), nothing allocates, and the
counting decision — which needs `requested` and the progress callback — lives
where it can block safely.

**3. One bound, and it measures the right thing.** This replaced two, and the
reason is the most useful thing in this section.

The original design had a *stall bound* (how long the beam may stay broken)
set at 1500 ms, plus an absolute cap on the whole dispense. The very first
standalone run produced legitimate single-pill breaks of **852 ms and
1210 ms**. That is 290 ms of margin on a medication device, where tripping the
bound means **refusing a dose that was being delivered correctly**. A slightly
slower pill would have been reported as a jam.

The absolute cap was measuring the wrong quantity too: it punished a *slow*
dose rather than a *stopped* one, so a large dose needed a large timeout and a
genuinely stuck mechanism still had to wait the whole thing out.

Both are gone. `MS_NO_PILL_TIMEOUT_MS` (**25 s**) measures **time since the
last counted pill**, and every pill restarts it. A working mechanism runs as
long as it needs to; a stopped one is caught within one window — including the
stalled-in-the-beam case, which simply stops producing counts and is caught
like anything else. One bound, on the quantity that actually distinguishes
working from stopped.

The 25 s figure came from the project owner: *"when no pill detected alone for
like 20-30 seconds, then say jam or pill needs to be refilled."*

**`Error_Handler()` is not called anywhere in `dispenser.c`.** A jam is a
normal outcome of a mechanical device, not a system fault.

---

## Part B — the dispense flow

`STATE_DISPENSING` in `state_machine.c` is now `#if MEDSIGHT_PHYSICAL_DISPENSER`
around real hardware, `#else` around the Session 10 simulation, which is
preserved byte-for-byte.

The progress bar is driven by `dispensing_progress_cb()`, called from the
driver **only on a real change in the count** — one pill, one bar step. The
animation is therefore a readout of the IR interrupt, not a timer that happens
to look plausible. That was an explicit requirement from the project owner and
it matters: an animation that runs on a timer while the mechanism is jammed is
a device that lies to a patient.

Outcomes:

| Result | Meaning | Trigger |
|---|---|---|
| `DISPENSE_OK` | requested count reached | — |
| `DISPENSE_SHORT` | some pills, then 25 s of nothing | **hopper needs refilling** |
| `DISPENSE_JAM` | zero pills in 25 s | nothing was released at all |

**The audit line was moved to after the dispense**, and this is
`ENGINEERING_LESSONS.md` Addendum 27's rule in practice: the audit log must
never contradict itself. Logging "dispensing 3" before the fact and "counted
1" afterwards leaves a record that reads as two claims about one event. It now
logs once, after the outcome is known:

```
DISPENSE: <patient> <n> requested, <m> counted (<result>)
```

---

## Part C — no hardware still works

`MEDSIGHT_PHYSICAL_DISPENSER 0` builds clean and runs the full application
with nothing attached. This is not a debug path; it is the configuration a
reviewer without a stepper motor will build, and it is why the `#else` branch
was preserved rather than deleted.

Verified at the end of the session: **0 errors, 0 warnings in both
configurations.** The simulated flow itself was last exercised on hardware in
Session 10 and was not re-run this session — it is a build-verified claim, not
a hardware-verified one.

---

## Part E — the buzzer

`FSBL/Src/buzzer.c`, `FSBL/Inc/buzzer.h`. Active piezo on **PA3 / `D10`**,
main VDD I/O domain.

**The part is an active piezo, so there is no timer channel and no PWM.** It
contains its own oscillator; the firmware switches DC. This is a deviation
from the prompt's E1, which anticipated `TIM16_CH1`. The pin is still
`TIM16_CH1`-capable AF1, so a passive element could be substituted later
without moving the wire. No drive transistor and no resistor — the element
draws what a GPIO sources.

It runs as the **Alert Task at priority 1**, the lowest in the system. A
buzzer that delays the camera ISP task to finish a beep is a defect; a beep
that arrives 5 ms late is not perceptible.

| Pattern | Sound | For |
|---|---|---|
| `BUZZ_TICK` | one 10 ms blip | UI tap |
| `BUZZ_DISPENSE_OK` | 2 × 80 ms | dose came out |
| `BUZZ_DISPENSE_FAIL` | 4 × 60 ms, rapid | jam or short |
| `BUZZ_DOSE_REMINDER` | 3 × (short-short) | **patient**, window opening |
| `BUZZ_DOSE_MISSED` | 4 × 500 ms, twice | **carer**, window closed unmet |

`BUZZ_DOSE_MISSED` is the only pattern built from long tones, deliberately.
The patient-facing reminder is polite because it fires on schedule when
nothing is wrong; the carer-facing alert is insistent because by then
something is. A rank array prevents a keyboard tick from cutting off a
missed-dose alert — the low-rank pattern is dropped, not queued.

`MEDSIGHT_BUZZER_KEYBOARD_CLICK` defaults to **0**. A device that beeps on
every keypress is tiring to use; the capability is there and off.

**Honest limit:** the buzzer was confirmed working by the project owner
("buzzer works alright man"), but **no UART capture of it exists** and no
pattern was verified against a scope or a recording. Per the prompt's Part E
rule, it is documented here; the claim in the other documents is kept to what
was observed.

---

## Standalone boot from external flash

This was not in the prompt. It became necessary because the project owner
bought a power bank, and a device that needs a laptop attached to boot is not
a device.

> *"A — I want standalone boot. Else, buying the power bank would have been
> pointless."*

It works. The board boots MedSight from external flash with **BOOT1 (SW1)
LOW**, no debugger, and has run a complete registered-patient dispense on
battery alone.

Five separate faults stood between here and there. Four were mine.

### 1. The boot ROM's 512 KB limit

The STM32N6 has no internal flash. The boot ROM copies an FSBL into AXI SRAM2
(header at `0x34180200`) and the copy is capped at **512 KB**. MedSight is
878 KB. Signing the whole application `-t fsbl` — which is what I did first —
produces an image the ROM can never load, no matter how correct everything
else is.

The fix is ST's two-stage **"load and run application"** mode: ST's own small
FSBL at `0x70000000`, MedSight signed `-t ssbl` at `0x70100000`.

```
STM32_SigningTool_CLI -nk -t ssbl -hv 2.3 -la 0x34000000 -align
```

### 2. The loaded image overlapped the loader

`STM32N657X0HXQ_AXISRAM2_fsbl.ld` had ROM and RAM the wrong way round for this
boot mode. **Swapped:**

```
ROM (xrw) : ORIGIN = 0x34000400, LENGTH = 1023K   /* code + .rodata */
RAM (xrw) : ORIGIN = 0x34100000, LENGTH = 1024K   /* .data + .bss  */
```

### 3. `CNF_SYSTEMAREA_END` did not move with the RAM region

This one is worth the space because it looks like nothing.

`.bss` moved up with the RAM region, so `_end` landed at `0x341A3890` — above
`CNF_SYSTEMAREA_END`, still at `0x34100000`. `knl_lowmem_top` then exceeded
`knl_lowmem_limit`, **the kernel heap had negative size**, every `tk_cre_*` in
`usermain()` failed, and the scheduler started with no tasks.

The board printed its entire pre-kernel boot log and then stopped between
`sleep clocks:` and `task_camera_isp: started.` No fault. No
`Error_Handler()`. Nothing on UART. A silent hang that looked like a boot
failure and was actually a configuration constant left behind.

`CNF_SYSTEMAREA_END` is now `0x34200000` — still exactly where the camera
framebuffer begins, which is the physical boundary it has always tracked. Both
`config.h` and the linker script now carry a note saying they are **one
decision and must always move together.**

### 4. XSPI kernel clock selection — *not the fix, kept anyway*

`PeriphCommonClock_Config()` now force-resets XSPIM/XSPI1/XSPI2 and pins both
XSPI kernel clocks to HCLK. **This did not fix anything.**

It is retained, with a caveat recorded here rather than buried: it writes
`RCC_CCIPR6/7/8/13`, and **STM32N6 erratum 2.2.23** states that modifying
those registers can cause boot ROM execution failure after reset. The same
applies to the narrowed `Set_RISAF_Default(RISAF12_S)` call added to
`npu_init.c`.

Both were kept because they are part of the exact configuration that was then
verified across a dozen power cycles and a battery dispense, and the project
owner had finished testing — removing verified-working code with no way to
re-verify it is the worse trade. **If standalone boot ever regresses, these
two are the first things to revisit.**

### 5. The actual fault

ST's external loader leaves the MX66UW1G45G in **Octal-DTR** mode, and that is
a property of the **chip**, not of the controller. It survives any reset on
our side — a controller reset, a core reset, a power cycle of the MCU.

`BSP_XSPI_NOR_Init()` called `XSPI_NOR_ResetMemory()`, which issues its reset
sequence *before* establishing what mode the chip is actually in. So it spoke
single-SPI to a chip listening in Octal-DTR, got nothing back, and returned
`-5`.

The fix, at the top of `XSPI_NOR_ResetMemory()` in
`Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.c`: **ask the chip in
OPI/DTR first, and if it answers, adopt that mode instead of resetting it.**

```c
if (MX66UW1G45G_ReadStatusRegister(&hxspi_nor[Instance], BSP_XSPI_NOR_OPI_MODE,
                                   BSP_XSPI_NOR_DTR_TRANSFER, reg) == MX66UW1G45G_OK)
{
    XSPI_Nor_Ctx[Instance].IsInitialized = XSPI_ACCESS_INDIRECT;
    XSPI_Nor_Ctx[Instance].InterfaceMode = BSP_XSPI_NOR_OPI_MODE;
    XSPI_Nor_Ctx[Instance].TransferRate  = BSP_XSPI_NOR_DTR_TRANSFER;
    return BSP_ERROR_NONE;
}
```

Also fixed alongside it: the "reset all modes" branch had **no** post-reset
delay at all; it now waits `MX66UW1G45G_RESET_MAX_TIME`.

### The ten hypotheses that were wrong

Recorded because negatives are the expensive part and they do not survive in
code.

1. The signed image is malformed → verified by full-file `-v`, it is not
2. The image is at the wrong flash address → checked against the loader's map
3. The boot ROM needs a specific header version → tried 2.3 and alternatives
4. BOOT0/BOOT1 are in the wrong combination → swept all four
5. The external loader is the wrong one → replaced, same behaviour
6. The camera bring-up is hanging (a darkened frame did appear on the LCD
   before the halt, which made this look certain) → it was not; it was
   fault 3 above, and the frame was the last thing that ran before the
   kernel came up with no tasks
7. RISAF isolation is blocking the flash → full `RISAF_Config()` actually
   *hangs* (it walks RISAFs whose IPs are not clocked); narrowed to
   `RISAF11_S`/`RISAF12_S`, still not it
8. The XSPI controller is in a different state on the two boot paths
9. XSPIM is muxed differently on the two boot paths
10. The XSPI kernel clock is unselected on the flash-boot path

**Hypotheses 8, 9 and 10 died together in one measurement.** A register dump
comparing the dev-boot and flash-boot paths showed XSPI2, XSPI1 and XSPIM
**bit-for-bit identical** in both. That negative result is what finally moved
the search from the controller to the *chip*, which is where the fault was.

That dump is kept behind `MS_AIPRE_TRACE` (now `0`), and the BSP tracing
behind `MS_XSPI_TRACE_ON` (now `0`) — the same treatment
`MS_BOOT_LED_CHECKPOINTS` and `MS_DISPLAY_WATCH` get. Gated off, not deleted.
External-flash bring-up will be debugged again someday.

### Two process notes on my own work, recorded

**I verified a flash write weakly and called it confirmed.** I read four bytes
of a magic number that *both* candidate images happened to share, and reported
the write as verified. It was not. Corrected to full-file `-v` verification
plus entry-point fingerprinting. A verification that cannot distinguish
between the two outcomes you care about is not a verification.

**I introduced a regression by "cheaply" probing `XSPI2->CR`** without
enabling XSPI2's clock, which hung dev boot. Reading a register of an
unclocked peripheral is not a free check. The code now carries a note saying
so.

---

## Why `A0`–`A3` do not work, and how that was found

The first wiring used `A0`–`A3` on CN7 (PA5/PA9/PA10/PA12). The motor did not
move and the ULN2003's channel LEDs never lit — but **all four pins read back
HIGH through `GPIOx->IDR`**, so every software-side check said the MCU was
driving them.

What settled it was the cheapest possible test: jumper the board's `3V3` pin
directly to a ULN input. The channel LED lit immediately. That one observation
eliminated the driver, the grounds and the wiring in a single step and left
the CN7 pins as the only remaining suspect. Moving to `D3/D5/D6/D9` made the
motor turn on the first attempt.

The analog set is preserved behind `MS_COIL_PINSET 0` so the finding is not
lost. It should be treated as known-bad on this board.

**The generalisable part:** `IDR` reading back the value you wrote proves the
GPIO latch took it. It proves nothing about whether the pad is driving
anything external. A external-side test — a meter, or in this case a bare
jumper and an LED already on the board — is a different measurement, and it
was worth more than an hour of the software-side kind.

### The resistors that were not needed

An early draft called for pull-down resistors on the ULN inputs. The project
owner pushed back: *"I don't wanna add like resistors and make hardware a hell
for me. If you worry about floating, you ground using software."*

Checking TI SLRS027T §7.2–7.3: the ULN2003A has an **internal input network**
— 2.7 kΩ series plus 7.2 kΩ/3 kΩ base-emitter pulldowns. The inputs do not
float. **The requirement was wrong and was dropped.** No external components
of any kind are used.

---

## Measurements

| Quantity | Value | How |
|---|---|---|
| IR idle level | HIGH | measured across 7 boots |
| Real pill break | **17–46 ms** | ESP32 histogram tool, bench |
| Contact chatter | 0–1 ms | same |
| Chatter floor chosen | 8 ms | above all chatter, < ½ shortest real break |
| Longest *legitimate* break observed | **1210 ms** | standalone run — the number that killed the 1500 ms stall bound |
| Half-step period | 2 ms | smooth and silent under turntable load |
| Dispenses run | **4** (5/5, 3/3, 3/3, 2/2) | **13/13 pills correct, 0 miscounts, 0 spurious jams** |
| Battery dispense | 1, complete | power bank alone, no laptop |
| Build | 0 errors, 0 warnings | both configurations |

### On the two LEDs during the battery run

The project owner asked whether the LED behaviour on battery was normal. It
is, and it is a useful liveness readout worth writing down:

- The **green** LED toggles once per iteration of `task_camera_isp_fn`, which
  runs every 1 ms. At ~500 Hz the eye integrates it into a **steady dim
  glow** — so "dimly lit" means the ISP task is running every frame.
- The **red** LED toggles every 500 ms in the idle task — a **1 Hz blink**,
  and mixed with the green it reads as red-to-yellowish-green cycling.

Both ends of the priority range are therefore visibly alive, on battery, with
no debugger. (The LD3/LD4 silkscreen mapping was not separately confirmed;
the firmware behaviour above is what was verified.)

---

## Part D — documentation

Five documents asserted, firmly, that this hardware would never exist. All are
corrected, with the reason rather than quietly:

1. **`HARDWARE_ARCHITECTURE.md`** — §2's "NOT Built — Design Intent Only" and
   §3's "Explicitly Not Used" rewritten; real pin assignments added.
2. **`MECHANICAL_DESIGN.md`** — "NOT BUILT" banner removed for the one hopper
   that now is built. **Kept** for the 6–8 hopper scaling, which remains
   design intent.
3. **`SOFTWARE_ARCHITECTURE.md`** — §1's hardware-cut paragraph, §7's jam edge
   case (which it had explicitly deleted, and which is now real), the module
   list, §8's pin map, and §9's task table including the new `alert` task.
4. **`MASTER_PROJECT_PLAN.md`** — §2's scope guardrails, §6's
   prototype-vs-final table, plus a changelog entry.
5. **`THIRD_PARTY_SOFTWARE.md`** — new §8. **No third-party library was
   added**; `dispenser.c` and `buzzer.c` are original work over ST HAL GPIO,
   which is already inventoried. Stated explicitly because an empty addition to
   a rule-1.3 inventory is indistinguishable from a forgotten one. §8 also
   records the two ST BSP functions this session modified, the fact that ST's
   `ai_fsbl.hex` is used for standalone boot but **not redistributed** here, and
   the other-contest-entrant FSBL that was briefly flashed during debugging and
   removed.

Two documents were added that the prompt did not ask for, and both earned it:

- **`documents/HARDWARE_WIRING.md`** — the wiring table, an SVG schematic, the
  power path, the bring-up order and the firmware knobs. It exists because the
  project owner asked for the pin-to-pin connections four separate times across
  the session, which is a reliable signal that the information had no home.
- **`documents/RUNNING_ON_HARDWARE.md`** — a complete path from a fresh clone to
  a dispensing device, for someone who has never seen this repository. Standalone
  boot took the longest stretch of this session and it is worth nothing to a
  judge who cannot reproduce it; every command in it is one that was actually
  run.

---

## What is NOT done

Stated plainly, because the gap between "it works" and "it is validated" is
where devices like this get people hurt.

- **The 10 × 3 reliability run was not performed.** Four dispenses were run,
  all correct. Four trials cannot distinguish a 100% device from a 95% one.
  This is the single most valuable missing measurement and it should be the
  first thing done next session.
- **No jam was ever deliberately induced.** `DISPENSE_JAM` has been reached
  only by an early polarity bug, never by physically blocking the mechanism.
  The 25 s bound is reasoned, not exercised.
- **`DISPENSE_SHORT` has never fired in earnest.** The refill path is code
  that has not run against a genuinely empty hopper.
- **The buzzer has no capture.** Confirmed by ear only; no scope, no
  recording, no UART log of the Alert Task.
- **The Part C simulated flow was not re-run on hardware** this session. It
  builds; it was last exercised in Session 10.
- **Erratum 2.2.23 is live.** The `CCIPR6/7/8/13` writes and the `RISAF12`
  call are both retained and both unproven as necessary.
- **The SD card intermittently fails `HAL_SD_Init` at boot** and recovers on
  its own. At one point the gallery showed `loaded 0 patient(s)` after a card
  write was lost during an earlier fault. Pre-existing, not introduced here,
  and not investigated.
- **One hopper.** The 6–8 hopper architecture stays design intent, as the
  prompt specified. `dispenser_dispense()`'s shape does not preclude it.

---

## Build status

```
0 errors, 0 warnings
```

Both configurations, headless build, verified at the end of the session:

- `MEDSIGHT_PHYSICAL_DISPENSER 1` — hardware path
- `MEDSIGHT_PHYSICAL_DISPENSER 0` — Part C fallback

Zero `tk_*` calls outside `ms_osal.c`. No `.ioc`. No networking. No `printf`
in any ISR or handler.

---

## Files

**New:**

```
FSBL/Inc/dispenser.h          FSBL/Src/dispenser.c
FSBL/Inc/buzzer.h             FSBL/Src/buzzer.c
documents/HARDWARE_WIRING.md
documents/images/session17_dispenser_wiring.svg
```

**Modified:**

```
FSBL/Src/main.c                    init + alert task + XSPI clock pinning
FSBL/Src/stm32n6xx_it.c            EXTI0_IRQHandler + rising/falling callbacks
FSBL/Src/ui/state_machine.c        STATE_DISPENSING, progress cb, buzzer hooks
FSBL/Src/ai/npu_init.c             RISAF12, register dump (gated off)
FSBL/mtk3_bsp2/config/config.h     CNF_SYSTEMAREA_END -> 0x34200000
Drivers/BSP/.../stm32n6570_discovery_xspi.c    THE standalone-boot fix
STM32CubeIDE/FSBL/STM32N657X0HXQ_AXISRAM2_fsbl.ld    ROM/RAM swapped
STM32CubeIDE/FSBL/.project         two new source links (64 total)
```

An ESP32 break-duration histogram tool was written to take the IR
measurements. It lives in `tools/`, which is gitignored, so it is not in the
repository — noted here so the provenance of the 17–46 ms figure is traceable.

---

## For the next session

1. **Run the 10 × 3 reliability trial.** It is one evening's work and it is
   the number this session owes.
2. **Induce a real jam** and a real empty hopper. Exercise both failure paths
   physically.
3. **Capture the buzzer**, even just a UART log of the Alert Task firing each
   pattern.
4. If standalone boot regresses, revisit the erratum-2.2.23 clock pinning and
   the `RISAF12` call **first** — they are the two pieces of the working
   configuration that were never shown to be load-bearing.
