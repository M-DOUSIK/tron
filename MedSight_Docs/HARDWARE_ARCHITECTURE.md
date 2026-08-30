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

## 2. Added Peripherals (not on the DK board)

MedSight is a multi-hopper, shared device (see `MECHANICAL_DESIGN.md` §1) — build 2–3
hopper modules for the contest demo, with the electronics pattern below repeated
identically per hopper so scaling to 6–8 later is a wiring/BOM task, not a redesign.

| Component | Purpose | Interface | Notes |
|---|---|---|---|
| 28BYJ-48 unipolar stepper + ULN2003 driver board, one pair per hopper (2–3 for the demo build) | Drives that hopper's turntable, singulating and counting loose pills — direct duplicate of the Mr Innovative/UPV reference design (see `MECHANICAL_DESIGN.md` §3) | 4 GPIO lines per hopper to the ULN2003 board (direct coil-sequence drive, not STEP/DIR) | Cheap, well-documented, easy to duplicate identically across hoppers; bench-test each hopper's actual pill/guide fit under load before Session 10's full integration |
| IR break-beam sensor (emitter + receiver pair), one per hopper — **not shared across hoppers** | Confirms and counts pills dropping from that specific hopper | GPIO EXTI (interrupt on beam break) per hopper | Needs debounce logic (Session 10) — mechanical pill edges can cause multiple rapid triggers; break-beam specifically (not reflective IR) to avoid pill-color false negatives |
| Shared 5V/6V motor power rail | Motors draw current spikes the board's own regulator shouldn't supply | Separate buck supply or battery pack, **grounds tied to the DK board ground**, sized for however many motors may actuate concurrently | Never power motors from the DK's own 3.3V/5V logic rail directly — do this on the bench before Session 10, this is a "you" task, not Antigravity's |
| microSD card | Local-only event/log/face-data storage (see `COMPLIANCE_PRIVACY_POSTURE.md`) | On-board SDMMC | Already on the DK board — just needs a card inserted |

**GPIO/pin budget note:** each hopper needs 4 GPIO lines to its ULN2003 driver board
(direct coil-sequence drive for the 28BYJ-48) + 1 IR EXTI input — 5 pins/hopper. For a
2–3 hopper demo (10–15 pins) this fits comfortably on the Arduino/STMod+ headers; if
you scale toward 6–8 hoppers later (30–40 pins), you'll need a GPIO expander (e.g. an
I2C port expander) rather than running out of native pins — note this now so it isn't
a surprise when the hopper count grows, but it's not a Session 10 concern for the
2–3-hopper demo build.

**Power note:** 28BYJ-48 steppers draw modestly (well under 1A each at 5V), so the
shared motor rail sizing is less demanding than it would be for larger motors — but
still size it for the worst case of 2–3 hoppers stepping concurrently, and still keep
grounds tied to the DK board per the wiring notes below; don't assume the on-board
5V rail can carry this without checking its actual current budget first.

## 3. Explicitly Not Used

- Ethernet (on-board but never initialized — zero-network requirement)
- USB Host/Device data functions beyond ST-LINK debug/flash
- Any wireless module — none is added to this BOM

## 4. Wiring / Power Safety Notes (for your bench work — Antigravity cannot verify these)

- Common ground between DK board and the shared motor supply is mandatory before any
  ULN2003 driver board is connected — floating grounds will produce erratic stepper
  behavior or damage the GPIO.
- Confirm each IR sensor's logic-level output matches STM32 GPIO input voltage (most
  break-beam modules are 3.3V/5V tolerant, but check the specific part before wiring).
- Route each hopper's motor-driver control lines and IR EXTI input to documented,
  free pins on the Arduino/STMod+ headers — pins will be finalized and recorded in
  `SOFTWARE_ARCHITECTURE.md`'s pin map once Session 10 assigns them per hopper (don't
  guess ahead of that session).
- Size the shared motor power rail for the worst case of multiple hoppers actuating
  in the same dose event, not just one hopper in isolation.

## 5. Debug Interface

On-board STLINK-V3EC handles both flashing and a USB virtual COM port for the UART
debug log introduced in Session 02 — no external debug probe required.
