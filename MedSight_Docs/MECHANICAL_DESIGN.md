# MECHANICAL_DESIGN.md — MedSight Dispensing Mechanism

> **STATUS: DESIGN INTENT ONLY — NOT BUILT, NOT WIRED TO FIRMWARE.** Everything below
> describes the dispensing mechanism a real, future MedSight product would use. It was
> originally scoped to be physically built (2–3 hoppers) for the contest demo; that
> physical-build plan was **cut entirely** — see `MASTER_PROJECT_PLAN.md`'s Changelog
> and `prompts/session_10.md`'s "Hardware decision (FINAL): No physical motors/
> servos/IR sensors are interfaced." The contest-submitted prototype dispenses via an
> on-screen simulation only (Session 10). This document is retained as submission
> material — illustrated via `RAGNAR_CAD_PROMPT.md`'s 3D renders and referenced from
> Session 13's `DESIGN_PROTOTYPE.md` deliverable — explaining the design intent, not
> as instructions for anything actually built in this project. Read it as "here is
> what we'd build next," not "here is what Session 10 does."

## 1. Scope Change: Multi-Hopper, Multi-Patient Device

MedSight is a **shared device serving multiple people, each on multiple
medications** — not a single-medicine device. Per current polypharmacy data, the
average nursing-home resident takes about seven medications and the average elderly
patient overall is on five or more; a single-hopper carousel can't represent that.

**Design target: up to 6–8 independently addressable medication hoppers.** That
range covers one resident at typical-to-moderate polypharmacy, or two residents on
lighter regimens, without jumping to hospital-cabinet scale (commercial automated
dispensing cabinets hold 400+ SKUs, but those are refrigerator-sized units with
robotic pick-to-light drawers — a different product category, not a reasonable
benchmark for this project).

**Build target for the Sept 30 contest deadline: 2–3 physically populated hoppers**,
with the architecture designed to scale to the full 6–8 without firmware changes.
Don't try to physically build all 6–8 before the deadline — the mechanical/BOM cost
of each additional hopper is real, and a working 2–3-hopper demo with a documented,
genuinely scalable design is a stronger submission than an overextended, unreliable
6–8-hopper build.

## 2. Chosen Design: Stacked Independent Hopper Modules

Each medication gets its own small, self-contained dispensing module: a hopper body,
a simple single-pill-type release mechanism, its own small motor, and its own local
IR break-beam sensor. Modules stack vertically (or arrange side-by-side if vertical
stacking doesn't suit your enclosure), each dropping into a **shared collection
chute/tray** at the bottom.

**Note (not current firmware):** an earlier revision of this plan expected a
`dispenser.c` module exposing `dispense_dose(hopper_id, count)`, so that adding a
physical hopper module later would be a hardware task plus a config entry, not a
firmware rewrite. That module was never built — Session 10's actual dispense flow is
software-simulated (see `SOFTWARE_ARCHITECTURE.md` §7) and has no hopper concept at
all. If this design is ever actually built, `dispense_dose(hopper_id, count)` (or
equivalent) would still be the right abstraction point — it just doesn't exist in
this project's real firmware today.

### Why stacked independent modules over one bigger carousel

| Approach | Reliability | Buildability under deadline | Scalability | Pill-type isolation |
|---|---|---|---|---|
| **Stacked independent hopper modules** | High — a jam in one module doesn't affect others | High — build 2-3 now, add more later without redesign | High — add a module = print + wire one more unit | Perfect — each hopper is one medicine type only |
| One large multi-position carousel (all meds on one disc) | Medium — one mechanism, one point of failure for everything | Lower — must build the full disc capacity up front to be useful at all | Poor — capacity fixed at print time | Requires careful compartment sizing per pill type on one disc |
| Auger-per-hopper bank (industrial pattern) | High | Lower — augers are more complex to print/tune than a simple gate | High | Good |

The stacked-module approach wins specifically because it lets you build
incrementally against a hard deadline: 2 working hoppers today, 6 later, without
touching firmware.

## 3. Per-Hopper Mechanism

Rather than designing a new release mechanism, **each hopper is a physical duplicate
of a proven open-source design**: a small turntable with passive guides that
singulates loose, unsorted pills into a single-file row, pushes that row past a
break-beam IR sensor, and drops counted pills down a chute — the same approach used
in Mr Innovative's open-source tablet counter/dispenser (Hackster.io) and the similar
Makers UPV design. You don't need precise per-pill compartment alignment (unlike a
notch-wheel carousel) — pills are just dropped in loose, and the guides do the
singulation work mechanically.

- **Motor: 28BYJ-48 unipolar stepper + ULN2003 driver board, one pair per hopper.**
  This is the same class of motor used in the UPV reference design — cheap,
  well-documented, low current draw, and easy to duplicate identically across
  hoppers. It needs 4 GPIO lines per hopper to the ULN2003 board (direct
  coil-sequence drive, not a STEP/DIR interface) — see the GPIO budget note in
  `HARDWARE_ARCHITECTURE.md` §2.
- The turntable spins via stepper steps, pushing pills through the guide path; the
  hopper's local break-beam sensor counts pills dropping past it, and the motor stops
  once the prescribed count for that hopper is reached — reusing the same
  sensor-counted approach Mr Innovative's reference implementation already proved out.
- Break-beam sensing specifically (not reflective IR) — a documented failure mode in
  the reference design is reflective IR missing certain tablet colors; break-beam
  avoids that regardless of pill color.
- Turntable/guide geometry is sized per hopper to that hopper's specific pill — this
  is the one part of the design that isn't a pure copy-paste between hoppers, since
  different medications have different pill sizes.

## 4. Shared Collection Path

All hoppers drop into one shared chute/tray rather than each having its own separate
output — this keeps the "the user picks up their dose from one place" experience
simple, and keeps the enclosure design manageable. The state machine (Session 11)
is responsible for sequencing which hoppers fire for a given scheduled dose event
across however many medications that specific dose requires.

## 5. Reference Designs

- **Mr Innovative's tablet counter/dispenser (Hackster.io) — primary reference,
  duplicated per hopper.** Single stepper motor, passive guides for singulation,
  break-beam sensor counts pills until the target is reached. This is the design
  each hopper module physically replicates; adapt guide dimensions to each hopper's
  specific pill, but the mechanism and control logic carry over directly.
- Makers UPV's smart pill dispenser (Hackster.io) — the other open-source reference
  using the same 28BYJ-48 stepper class, useful as a second data point on motor
  choice and mounting.
- Mellow_labs' stackable pill dispenser (Hackster.io) — useful for the physical
  stacking/enclosure pattern (identical electronics per module, printed extension
  pieces) even though its own release mechanism differs from the turntable approach
  chosen here.

## 6. Key Design Parameters (finalize during Session 10 bench work)

- Hoppers physically built for the contest demo: 2–3, each a direct duplicate of the
  Mr Innovative turntable design. Architecture designed for: 6–8.
- Turntable/guide geometry: sized to that hopper's specific pill/capsule with margin
  for irregular shapes — this is the one thing tuned per hopper, not copy-pasted.
- Motor per hopper: 28BYJ-48 unipolar stepper + ULN2003 driver board (4 GPIO lines
  per hopper for direct coil-sequence drive) — same part class as the open-source
  references above, chosen for low cost and ease of exact duplication across hoppers.
- Break-beam IR sensor: one per hopper, local to that hopper's own drop point —
  not one sensor shared across all hoppers, since simultaneous or near-simultaneous
  drops from different hoppers would be uncountable with a single shared sensor.

## 7. Jam Handling (document in Session 10, implement then)

- If a given hopper's beam doesn't break within a timeout window after that hopper's
  dispense command, log a "possible jam / empty hopper" event scoped to that specific
  hopper — don't halt the whole dispense cycle for the other hoppers in the same dose
  event.
- Module isolation means a jam in one hopper doesn't cascade to others — this is the
  core reliability argument for this architecture, same principle as compartment
  isolation in a carousel, just applied at the module level instead.

## 8. Manufacturability

Standard FDM printing per module. PETG or PLA+ recommended over standard PLA for the
release-mechanism parts specifically (repeated motor-driven wear). Keep each
module's footprint and mounting pattern identical across hoppers even though the
internal pill-sized geometry differs — this is what makes "add one more hopper"
a repeatable print-and-wire task rather than a fresh design exercise each time.
