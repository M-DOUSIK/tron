# HARDWARE_ARCHITECTURE.md — MedSight

## 1. Core Board

**STM32N6570-DK** (confirmed target, per official ST data brief DB5351 Rev 1).
- MCU: STM32N657X0H3Q, Arm Cortex-M55 core, up to 800MHz.
- ST Neural-ART Accelerator NPU, up to 1GHz, ~600 GOPS INT8.
- 4.2MB contiguous on-chip SRAM.
- On-board: 5" LCD **with capacitive touch panel already integrated** (driven via I2C
  — see Session 05 in the prompts, `touch_driver.c`), camera module connector
  (MB1854/B-CAMS-IMX bundle), microSD slot, Octo-SPI flash (1Gbit), Hexadeca-SPI PSRAM
  (256Mbit), USB-C (DRP) and USB-A host, Gigabit Ethernet (**unused — no networking in
  this project**), SAI audio codec + MEMS mic, 2 user LEDs, user/tamper/reset buttons,
  on-board ST-LINK-V3EC debugger, Arduino Uno R3 + STMod+ expansion headers.

This board alone covers camera, display, **touch input**, storage, and debug — no
separate dev board or added touch hardware needed for the electronics core. The touch
panel is a driver/software task (Session 05), not a wiring task.

## 2. Dispensing Hardware: One Hopper, Built and Working (Session 17)

**Status changed in v11 of `MASTER_PROJECT_PLAN.md` — read the history, because
this section said the opposite for three sessions.** The physical build was cut
in v8 on the grounds that it was not achievable solo before the deadline;
Sessions 10-13 were therefore built as a software-only simulation, and every
document gained a firm "no motors, ever" banner. A teammate able to design and
build the hardware has since joined, so **Session 17 built a real
single-hopper turntable dispenser, and it works**: an actuator that singulates
pills onto a chute, and an IR break-beam sensor that **counts each pill as it
physically passes**, so the actuator stops on a real count rather than a timer.
Four dispenses have been run on hardware — 13 of 13 pills counted correctly —
including one from a power bank with no laptop attached.

What exists and what does not:

- **Built and verified on hardware (Session 17):** one hopper, one actuator,
  one IR counter, plus a carer-facing piezo buzzer.
- **Still design intent:** the 6-8 independently addressable hopper
  architecture in `MECHANICAL_DESIGN.md`, illustrated by
  `RAGNAR_CAD_PROMPT.md`'s renders. The firmware keeps the dispense API clean
  enough that adding a `hopper_id` would be additive, not a rewrite.

**The actuator is a 28BYJ-48 unipolar stepper driven through a ULN2003**, which
is exactly what the table below and `MECHANICAL_DESIGN.md` already specify — so
no correction was needed there, only the removal of the "not built" framing.
Four GPIO lines drive the coils directly in a half-step sequence; there is no
STEP/DIR driver IC. The motor is open-loop and stalls silently, which is
precisely why the count comes from the sensor and never from the step count.

**The IR beam is a packaged 3-pin module** (VCC / GND / OUT) — a small PCB
carrying the emitter, the detector, an LM393 comparator and a threshold trim
pot. This replaces the hand-built discrete emitter/receiver pair originally
specified; the project owner changed it after Session 13. The comparator and
its pull-ups live on the module, so the firmware sees a clean digital line and
most of the electrical risk goes away.

The **slot type** (U-shaped gap, sold as a speed sensor or photo-interrupter)
is preferred over the **reflective type** (FC-51 and lookalikes): a pill is
small, fast, and may be white, translucent or dark, and a reflective sensor
asked to detect one in mid-fall is doing the hardest version of its job.

### What was predicted, and what the hardware actually said

Session 17 built it. Three of the predictions above were wrong, and they are
corrected here rather than quietly edited out, because each one cost bench
time and the correction is the useful part.

**Polarity — predicted active LOW, measured idle HIGH.** The prompt warned
that most modules are active LOW and that polarity must be checked rather than
assumed. The warning was right; the guess was not. This module **idles HIGH
and pulls LOW while the beam is broken**, confirmed across seven boots. Worth
noting that boot-time polarity *learning* was tried and abandoned — on one
boot the line read LOW for two full seconds before settling, and a single
startup sample got it backwards, turning the first dispense into an instant
false jam. Polarity is now pinned to the measured value.

**Supply — predicted 3.3 V only, measured fine at both.** The module was
tested at 3.3 V and at 5 V and works at either. It runs at 5 V.

**The motor rail did NOT need its own supply.** This section previously said
the motor rail needs a separate 5 V supply with grounds tied. In practice the
ULN2003 runs from the board's own `CN8 5V` under turntable load with no
observable trouble, and that is now the recommended arrangement precisely
because it makes the shared ground structural rather than something a builder
has to remember. A separately-fed driver also works — **but its ground must
still return to `CN8 GND`**, and a driver whose ground floats relative to the
MCU simply does not switch, with no LED, no motion and no error to explain it.

### Debouncing, with a measured number

Debouncing is required, as predicted. The threshold was measured rather than
guessed:

| Quantity | Measured |
|---|---|
| Real pill break | **17–46 ms** |
| Contact chatter | 0–1 ms |
| Chatter floor chosen | **8 ms** |

The 17–46 ms figure confirms something the session prompt predicted: pills
**slide down a ramp** rather than free-falling. A free-fall through the beam
would have been a few milliseconds, and a driver sized for that number would
have thrown away every real pill.

### As built — actual pin assignments

| Component | Signal | Silkscreen | MCU pin | Domain |
|---|---|---|---|---|
| ULN2003A | `IN1` | `D3` | PE9 | VDDIO5 |
| ULN2003A | `IN2` | `D5` | PE10 | VDDIO5 |
| ULN2003A | `IN3` | `D6` | PE13 | VDDIO5 |
| ULN2003A | `IN4` | `D9` | PE14 | VDDIO5 |
| ULN2003A | power | `5V` / `GND` | CN8 | — |
| IR module | `OUT` | `D2` | PD0 (**EXTI0**) | main VDD |
| IR module | power | `5V` / `GND` | CN8 | — |
| Piezo buzzer | `+` | `D10` | PA3 | main VDD |
| Piezo buzzer | `−` | `GND` | CN8 | — |

Four GPIO lines drive the coils directly in a half-step sequence — no STEP/DIR
driver IC. Each half-step is a **single atomic `BSRR` write** of a fully-formed
word, so an interrupt landing mid-step cannot leave two coils energised in a
combination the table does not contain.

**`A0`–`A3` on CN7 do not work for this and must not be used.** They were the
first choice and the motor never moved, while all four pins read back HIGH
through `GPIOx->IDR` — every software-side check said the MCU was driving them.
Jumpering the board's `3V3` pin straight to a ULN input lit that channel
immediately, which eliminated the driver, the grounds and the wiring in one
step. The generalisable lesson: `IDR` reading back what you wrote proves the
GPIO latch took it and proves **nothing** about whether the pad drives anything
external.

**`D14`/`D15` are the camera's I2C1 and are off limits.** The pin budget was
traced against every live consumer before anything was assigned.

Complete wiring, with a diagram and a bring-up order:
**`HARDWARE_WIRING.md`**.

### Still design intent

| Component | For | Interface |
|---|---|---|
| A second through eighth hopper, each with its own stepper + ULN2003 | Independent per-medication dispensing (`MECHANICAL_DESIGN.md` §3) | 4 GPIO lines per hopper |
| One IR module per hopper | Per-hopper counting | one EXTI line each |

The dispense API takes a count and returns what was actually counted, so
adding a `hopper_id` parameter would be additive rather than a rewrite.


The only peripheral actually added to the DK board for this project is:

| Component | Purpose | Interface | Notes |
|---|---|---|---|
| microSD card | Local-only event/log/face-data storage (see `COMPLIANCE_PRIVACY_POSTURE.md`) | On-board SDMMC | Already on the DK board — just needs a card inserted |

## 3. Explicitly Not Used

- Ethernet (on-board but never initialized — zero-network requirement)
- USB Host/Device data functions beyond ST-LINK debug/flash
- Any wireless module — none is added to this BOM
- ~~Any dispensing actuator hardware~~ — **no longer true as of v11.** See §2:
  Session 17 interfaces one actuator and one IR sensor. This bullet is left in
  place, struck through, because three sessions' worth of documents and prompts
  cite it as a firm rule and a reader needs to see that it was deliberately
  superseded rather than forgotten.

## 4. Debug Interface

On-board STLINK-V3EC handles both flashing and a USB virtual COM port for the UART
debug log introduced in Session 02 — no external debug probe required.
