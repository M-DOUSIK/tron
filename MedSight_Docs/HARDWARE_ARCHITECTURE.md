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

## 2. Dispensing Hardware: One Hopper Being Built (Session 14)

**Status changed in v11 of `MASTER_PROJECT_PLAN.md` — read the history, because
this section said the opposite for three sessions.** The physical build was cut
in v8 on the grounds that it was not achievable solo before the deadline;
Sessions 10-13 were therefore built as a software-only simulation, and every
document gained a firm "no motors, ever" banner. A teammate able to design and
build the hardware has since joined, so **Session 14 builds a real
single-hopper turntable dispenser**: an actuator that singulates pills onto a
chute, and an IR break-beam sensor that **counts each pill as it physically
drops**, so the actuator stops on a real count rather than a timer.

What is being built and what is not:

- **Built (Session 14):** one hopper, one actuator, one IR counter.
- **Still design intent:** the 6-8 independently addressable hopper
  architecture in `MECHANICAL_DESIGN.md`, illustrated by
  `RAGNAR_CAD_PROMPT.md`'s renders. The firmware keeps the dispense API clean
  enough that adding a `hopper_id` would be additive, not a rewrite.

**The actuator is a 28BYJ-48 unipolar stepper driven through a ULN2003**, which
is exactly what the table below and `MECHANICAL_DESIGN.md` already specify — so
no correction is needed there, only the removal of the "not built" framing.
Four GPIO lines drive the coils directly in a half-step sequence; there is no
STEP/DIR driver IC. The motor is open-loop and stalls silently, which is
precisely why the count comes from the sensor and never from the step count.

**The IR break beam is hand-built from discrete parts** rather than a packaged
module: an IR emitter LED and an IR receiver facing each other across the pill
chute, each with its series/pull-up resistor. `prompts/session_14.md` Part 0
covers what that means for firmware — chiefly that the idle polarity must be
measured rather than assumed, that the edge will be noisier than a datasheet
would suggest, and that ambient IR can saturate the receiver.

The motor rail needs its own 5 V supply with grounds tied to the board's.

| Component (not built) | Would be used for | Would interface via |
|---|---|---|
| 28BYJ-48 unipolar stepper + ULN2003 driver board, one pair per hopper | Drives that hopper's turntable, singulating and counting loose pills — direct duplicate of the Mr Innovative/UPV reference design (see `MECHANICAL_DESIGN.md` §3) | 4 GPIO lines per hopper to the ULN2003 board (direct coil-sequence drive, not STEP/DIR) |
| IR break-beam sensor (emitter + receiver pair), one per hopper | Confirms and counts pills dropping from that specific hopper | GPIO EXTI (interrupt on beam break) per hopper |
| Shared 5V/6V motor power rail | Motors would draw current spikes the board's own regulator shouldn't supply | Separate buck supply or battery pack, grounds tied to the DK board ground |

The only peripheral actually added to the DK board for this project is:

| Component | Purpose | Interface | Notes |
|---|---|---|---|
| microSD card | Local-only event/log/face-data storage (see `COMPLIANCE_PRIVACY_POSTURE.md`) | On-board SDMMC | Already on the DK board — just needs a card inserted |

## 3. Explicitly Not Used

- Ethernet (on-board but never initialized — zero-network requirement)
- USB Host/Device data functions beyond ST-LINK debug/flash
- Any wireless module — none is added to this BOM
- ~~Any dispensing actuator hardware~~ — **no longer true as of v11.** See §2:
  Session 14 interfaces one actuator and one IR sensor. This bullet is left in
  place, struck through, because three sessions' worth of documents and prompts
  cite it as a firm rule and a reader needs to see that it was deliberately
  superseded rather than forgotten.

## 4. Debug Interface

On-board STLINK-V3EC handles both flashing and a USB virtual COM port for the UART
debug log introduced in Session 02 — no external debug probe required.
