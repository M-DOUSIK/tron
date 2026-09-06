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

## 2. Dispensing Hardware: NOT Built — Design Intent Only

**This section previously specified real BOM items (stepper motors, ULN2003 driver
boards, IR break-beam sensors, a shared motor power rail) to be physically wired to
the DK board.** That physical-build plan was cut entirely — see
`MASTER_PROJECT_PLAN.md`'s Changelog and `prompts/session_10.md`'s "Hardware decision
(FINAL): No physical motors/servos/IR sensors are interfaced." The contest-submitted
prototype dispenses via an on-screen simulation only. **None of the hardware in the
table below is purchased, wired, or driven by firmware in this project — it documents
the mechanism a real future product would use**, kept here (and in
`MECHANICAL_DESIGN.md`, illustrated via `RAGNAR_CAD_PROMPT.md`'s 3D renders) purely as
submission material explaining the design intent.

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
- Any dispensing actuator hardware (motors, servos, IR sensors) — see §2 above; this
  is a firm decision, not a "not yet," and no future session in this project's plan
  (Sessions 01-13, the full current plan) adds it back

## 4. Debug Interface

On-board STLINK-V3EC handles both flashing and a USB virtual COM port for the UART
debug log introduced in Session 02 — no external debug probe required.
