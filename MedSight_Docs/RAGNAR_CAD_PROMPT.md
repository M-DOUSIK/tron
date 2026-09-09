# RAGNAR_CAD_PROMPT.md — MedSight Enclosure, Text-to-3D Prompt

## Before You Use This

Every dimension below is a **parametric placeholder**, not a measured fact. Verify
against your physical STM32N6570-DK (calipers) or the board's mechanical drawing in
ST's UM3300 user manual before finalizing anything for print — I couldn't pull exact
PCB/LCD panel dimensions from available sources, so don't trust the numbers below as
gospel; they're a reasonable, editable starting point for Ragnar.AI's parametric
model, sized to be adjusted once you have real measurements.

## Why This Differs From a Sliding-Gate Cartridge Design

An earlier draft (from another AI) proposed vertical cartridges with a sliding gate
plate at the bottom — the gate opens, pills fall through by gravity, done. The flaw
your own review caught is real: a gate has no way to know *how many* pills passed
through. Slide it open too long, or have pills bridge/clump above it, and you get an
uncounted, unverified dose — silent overdispensing with no error signal.

**MedSight's actual mechanism avoids this by design** (see `MECHANICAL_DESIGN.md`):
each hopper is a small turntable with passive guides that singulates loose pills one
at a time and pushes them past a break-beam IR sensor that *counts* every pill as it
drops — the motor stops the instant the prescribed count is reached, verified, not
assumed. This prompt is built around that mechanism, not a gate.

## The Prompt (copy-paste into Ragnar.AI)

```
Create a compact, hospital-grade smart medication dispenser enclosure for an
STM32N6570-DK development board with an attached LCD panel and a repositionable
camera module, plus two identical, independently-driven pill dispensing hopper
modules. Apply 0.2mm mechanical clearances throughout for FDM 3D printing, and
2mm fillets on all external edges for a smooth, professional medical-device
appearance. Use a light, clean color scheme (white/light grey) in the render.

1. MAIN CHASSIS (parametric placeholder: 160mm wide x 110mm deep x 90mm tall —
   adjust to actual STM32N6570-DK PCB footprint once measured):
   A rectangular housing with a front face angled at 12-15 degrees from vertical,
   containing a rectangular cutout sized for a 5-inch LCD panel (placeholder
   active area 108mm x 65mm — adjust to actual panel dimensions) with a narrow
   (3-4mm) bezel border. Interior includes four PCB standoff bosses sized for
   M2.5 self-tapping screws, positioned to match the STM32N6570-DK's actual
   mounting hole pattern (verify pattern before finalizing standoff positions —
   do not assume a generic Arduino-shield hole pattern, this board's layout is
   its own). Include a rear-panel cutout for the USB-C power/debug connector and
   a side cutout for the microSD card slot, both positioned to align with the
   board's actual connector locations once confirmed. Add ventilation slots
   (2mm wide, spaced 8mm apart) on the rear panel above the PCB compartment.

2. ARTICULATING CAMERA MOUNT:
   On the top-front edge of the chassis, include a hinged camera boom: a two-
   segment articulating arm (each segment ~35mm long, connected by a friction-
   fit or printed living-hinge joint with enough resistance to hold a set angle)
   that folds flush into a recessed channel in the chassis top when stowed, and
   extends/rotates outward and upward when in use, positioning the camera module
   at face height for the face-recognition step of the dispense flow. Include an
   internal channel or clip points along the boom sized to route a flexible flat
   cable (FFC) from the camera module back into the chassis without pinching at
   the hinge points. The camera module mounting point itself (placeholder 25mm x
   25mm x 8mm) should sit at the boom's far end, angled slightly downward-
   adjustable by the hinge.

3. DUAL HOPPER MODULES (the two medicine slots, side-by-side on the chassis top,
   each module identical and self-contained):
   Each hopper module consists of:
   - A funnel-shaped pill reservoir (placeholder top opening 45mm diameter,
     tapering to a 30mm diameter turntable chamber), open-top with a
     snap-fit or friction-fit refill lid.
   - Inside the chamber, a flat turntable disc (28mm diameter, 4mm thick) with
     2-3 raised passive guide walls (2mm thick, radiating from center) that
     singulate loose pills into a single-file path as the disc rotates,
     matching the Mr Innovative-style turntable dispensing mechanism.
   - A central mounting boss on the underside of the turntable sized for a
     28BYJ-48 stepper motor's output shaft (5mm D-shaft coupling), with the
     motor body (28mm diameter x 19mm height, standard 28BYJ-48 form factor)
     housed in a cavity beneath the turntable, accessible from the chassis
     underside for wiring to an external ULN2003 driver board.
   - A single exit guide channel (10mm wide) leading from the turntable's edge
     to a small drop point, positioned so a **slot-type IR sensor module**
     straddles the falling pill's path — the pill must pass through the
     module's U-shaped gap. Provide a flat mounting shelf with two M3 (or
     3mm) holes on the standard ~28mm pitch of these boards, plus a cable
     exit for its 3-pin header. Do NOT model two aligned holes for a discrete
     emitter/receiver pair; the sensor is a PCB now, not two loose LEDs.
   - Each hopper's drop point funnels into ITS OWN short local chute that then
     joins a shared, printed Y-funnel beneath both hoppers, merging into a
     single collection tray opening at the chassis front, accessible to the
     user without opening the enclosure.

4. MODULARITY / FUTURE EXPANSION:
   Design the two hopper modules as a repeatable unit on a fixed-width "slot"
   footprint (placeholder 50mm wide per slot) along the chassis top, such that
   a future revision could extend the chassis width in 50mm increments to add
   additional identical hopper modules (up to 6-8 total) without redesigning
   the turntable/motor/sensor mechanism itself — only the chassis width and the
   shared Y-funnel's merge geometry change. Model the current two-hopper version,
   but keep the hopper module itself as a clean, independently-repeatable
   sub-component in the design.

5. ASSEMBLY / SERVICE ACCESS:
   Include a removable rear or bottom access panel (secured by 4 corner screw
   bosses, M2.5) over the main PCB compartment, separate from the top-loading
   hopper refill lids, so the electronics can be serviced without disturbing
   loaded medication.
```

## What to Verify Before Printing

- STM32N6570-DK actual PCB length/width/mounting-hole pattern and connector
  locations (USB-C, microSD, camera FFC connector, LCD FFC connector) — measure
  the physical board or pull the mechanical drawing from ST's UM3300 user manual.
- LCD panel active area and bezel dimensions for the physical 5" panel that ships
  with/is used alongside the kit.
- Camera module (B-CAMS-IMX or whichever specific module you're using) physical
  footprint and FFC cable length, to confirm the articulating boom's reach and
  hinge routing actually work for your specific cable.
- 28BYJ-48 stepper + ULN2003 driver board footprint against your specific
  sourced parts — these are a common, standardized size, but confirm before
  finalizing the motor cavity dimensions.

## After Generating

Export as STEP and sanity-check the turntable/guide clearances specifically
against your actual pill sizes for both medicines before committing to a full
print — per `MECHANICAL_DESIGN.md`, this is the one part of the mechanism that's
genuinely per-medicine-specific, not a copy-paste between hoppers.
