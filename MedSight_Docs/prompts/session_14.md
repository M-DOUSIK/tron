# Session 14 — Physical Dispensing Hardware: Stepper Turntable + IR Pill Counter

## Context — the scope change this session represents

**Physical dispensing is back in scope.** Read that carefully, because every
document in this repository written between Sessions 10 and 12 says the
opposite, in strong terms.

The history, so nobody has to reconstruct it:

- The **original Program Plan** (submitted March 2026, `scratch/Program Plan
  54916.pdf`) described a **camera-only verification device**. No dispensing at
  all. MedSight looked at a pill you were holding and told you whether it was
  the right one.
- An intermediate revision of `MASTER_PROJECT_PLAN.md` (v1–v4) added a full
  **multi-hopper physical dispenser**, 6–8 independently driven hoppers.
- **v8 cut that entirely**, on the grounds that it could not be built solo
  before the deadline. Sessions 10–12 were built as a software-only simulation:
  an on-screen animation plus an "I Took It" button. Every doc gained a firm
  "this is a cut, not a 'not yet' — reject it as scope creep" banner.
- **That constraint has now changed**: a teammate capable of designing and
  building the hardware has joined. A single-hopper turntable driven by a
  **28BYJ-48 stepper**, with a **hand-built IR break-beam** counting pills as
  they drop, is buildable within the remaining time.

So this is not scope creep, and not a reversal on the merits — it is the
resource assumption behind the v8 cut no longer being true.
`MASTER_PROJECT_PLAN.md` v11 records this properly. **The "no motors, ever"
rule in the older docs is superseded by this session; the "no networking" rule
is not, and never will be.**

---

## How to Start This Session

1. **READ ALL DOCUMENTATION** in `MedSight_Docs/` — especially
   `MASTER_PROJECT_PLAN.md` (v11 changelog first), `HARDWARE_ARCHITECTURE.md`,
   `MECHANICAL_DESIGN.md`, `SOFTWARE_ARCHITECTURE.md` and
   `ENGINEERING_LESSONS.md`.
2. **READ `MECHANICAL_DESIGN.md` IN FULL.** It already specifies exactly this
   mechanism — a 28BYJ-48 turntable duplicated per hopper — and explains, in
   its section on why a sliding gate was rejected, precisely why the IR counter
   matters: *a gate cannot know how many pills went through it.* That reasoning
   is the whole basis of this session's design, and the document needs no
   correction: build what it already describes.
3. **READ PAST SESSION PROMPTS** `session_01.md` through `session_13.md`.
4. **READ `milestones/session_12_notes.md`**, particularly the task-priority
   derivation in Part A3 and the frame-buffer ownership analysis in Part A1 —
   you are adding a task and it has to fit that scheme.
5. **READ `ENGINEERING_LESSONS.md`'s "New Findings (Session 06)"** before
   touching a single GPIO. The `VddIO` power-domain rule is the one that will
   cost you an afternoon if you skip it: *GPIO banks on this part are powered
   off by default and a peripheral on an unpowered pin hangs silently.*
6. **READ THE WORKING CODE** in `sessions/session_13/FSBL/` (or `session_12/`
   if 13 has not run yet): `Src/ui/state_machine.c`'s `STATE_DISPENSING`,
   `Src/main.c`'s task table, `Src/ms_osal.c`'s OSAL.

---

## Project Rules

| Rule | Detail |
|---|---|
| No `.ioc` files | Manual HAL only, per `ENGINEERING_LESSONS.md` rule 1. This project has no `.ioc`; you configure clocks, pins and peripherals in code. |
| **No networking** | Unchanged and permanent. |
| OSAL boundary | The new dispenser code calls `ms_osal.h` only. **Zero `tk_*` calls outside `ms_osal.c`.** If you need a new primitive, add it to the OSAL — Session 12 did exactly that for event flags. |
| No embeddings over UART | Unchanged. |
| No `printf` on a hot path | Nothing in an ISR — and this session adds a real ISR (the IR sensor's EXTI). See `session_11_notes.md` Addendum 8 for what that costs. |
| Trace the schematic first | `ENGINEERING_LESSONS.md` hard rule 4. Confirm from the STM32N6570-DK schematic which pins are genuinely free on the Arduino/STMod+ headers before assigning any. |
| New session = new folder | Copy `sessions/session_13` → `sessions/session_14`. |

---

## Part 0 — The hardware, and what the firmware has to do about it

### The actuator: 28BYJ-48 unipolar stepper + ULN2003

Exactly what `MECHANICAL_DESIGN.md` already specifies, so that document is
correct as written. What it means for firmware:

- **Four GPIO outputs**, one per coil, driven in a direct coil sequence — not
  STEP/DIR. There is no motor-driver IC doing the sequencing for you; the
  ULN2003 is just a Darlington array switching the coils you tell it to.
- **Half-step (8-phase) sequencing** is the sensible default: smoother and
  roughly double the resolution of full-step, for the cost of a longer table.
  The 28BYJ-48's internal 1:64 gearbox gives ~4096 half-steps per output
  revolution — plenty of resolution for singulating pills.
- **Step timing is yours to generate.** Too fast and the motor stalls silently
  (it is open-loop, so it will not tell you). Start conservative — around
  2 ms/step — and only speed up once the mechanism is proven. This is a good
  fit for a dedicated task with `osal_delay_ms()`, or a µT-Kernel cyclic
  handler if you want tighter step timing.
- **De-energise the coils when idle.** Leaving one energised holds torque and
  cooks both the coil and the ULN2003 for no benefit — the gearbox holds
  position mechanically. Drive all four low on every exit path, including the
  error ones.
- **Open-loop is exactly why the IR counter exists.** Steps commanded is not
  pills dispensed. Never infer a count from step count.

### The sensor: a hand-built IR break beam

Your teammate is soldering this from discrete parts rather than using a
packaged module: an **IR emitter LED** and an **IR receiver** (photodiode or
phototransistor) facing each other across the chute, each with its series /
pull-up resistor, so a pill falling between them momentarily breaks the beam.

Firmware consequences of a discrete build, all of which matter:

- **Confirm the idle polarity on a scope or a meter before writing the ISR.**
  With a pull-up on the receiver, a phototransistor typically pulls the line
  **LOW while the beam is unbroken** and lets it rise **HIGH when a pill
  interrupts it** — but that inverts depending on how the receiver is wired.
  Measure it; do not assume. Getting this backwards means counting the gaps
  between pills instead of the pills.
- **Configure the pin as a plain input** — the pull-up is external, so do not
  also enable the internal one unless your teammate asks for it.
- **A hand-built beam is noisier than a packaged module.** Expect a dirtier
  edge than a datasheet would suggest, and see Part A item 1 on debouncing.
- **Ambient IR is a real failure mode.** Sunlight and some indoor lighting emit
  IR strongly enough to hold the receiver saturated so the beam never reads as
  broken. Test under the lighting the demo will actually use. If it is a
  problem, the fixes are mechanical (shroud the beam path) before they are
  electrical.
- **Check the VddIO domain of whichever pin you pick** —
  `ENGINEERING_LESSONS.md`'s Session 06 finding. A GPIO in an unpowered domain
  reads a constant value and looks exactly like a sensor that never triggers.

### Power

`HARDWARE_ARCHITECTURE.md` already says it and it is not optional: the stepper
draws current spikes the DK board's regulator should not supply. Use a separate
5 V supply for the ULN2003's motor rail with grounds tied to the board's
ground. Confirm this with your teammate before powering anything.

### Deliverable for Part 0, before any driver code

Write down in the session notes: the measured idle/broken polarity of the beam,
the pins chosen for the four coil lines and the sensor (with their VddIO domain
confirmed), the step rate that runs without stalling, and how many half-steps
of turntable rotation reliably releases exactly one pill. That last number is
the one the whole dispense loop is built on.

---

## Part A — The dispenser driver

Create `FSBL/Src/dispenser.c` and `FSBL/Inc/dispenser.h`. This is the module
`SOFTWARE_ARCHITECTURE.md` described in an early revision and then deleted when
the hardware was cut; it now exists for real.

**API, kept deliberately close to what the old plan specified** so the
architecture docs and the state machine agree:

```c
typedef enum {
    DISPENSE_OK = 0,        /* requested count counted out of the chute      */
    DISPENSE_SHORT,         /* fewer pills than requested before timeout     */
    DISPENSE_JAM,           /* actuator ran, no pill seen at all             */
    DISPENSE_NOT_READY      /* driver not initialised                        */
} dispense_result_t;

void              dispenser_init(void);
/* Dispense `count` pills. Blocking, bounded by a timeout. Returns how many
 * were actually counted through the IR beam in *out_dispensed. */
dispense_result_t dispenser_dispense(uint8_t count, uint8_t *out_dispensed);
uint32_t          dispenser_get_total_dispensed(void);   /* since boot */
```

**The IR sensor is the whole point.** `MECHANICAL_DESIGN.md` and
`RAGNAR_CAD_PROMPT.md` both explain why: an actuator that runs for a fixed time
has no idea how many pills it released — pills bridge, clump, or fail to drop.
The break-beam counts each pill physically falling. **The motor stops when the
count is reached, not when a timer expires.** That closed loop is the single
most demonstrable engineering idea in this session; build it that way.

Requirements:

1. **IR sensor on a GPIO EXTI interrupt**, not polled. Each beam break
   increments a `volatile` counter. **Debounce it, and expect to need it** — a
   pill tumbling through the beam produces multiple edges, and a hand-built
   beam is noisier than a packaged module, so a naive counter will over-count.
   Debounce in the ISR with a timestamp comparison (`HAL_GetTick()` is safe to
   read from an ISR); **do not `printf`, do not call any `tk_*`/`osal_*`
   blocking call there** — `session_11_notes.md` Addendum 8 is the record of
   what a slow interrupt path costs on this hardware. Make the debounce window
   a tunable constant with the reasoning written beside it; you will adjust it
   on the bench.
2. **Bounded everywhere.** Every wait has a timeout —
   `ENGINEERING_LESSONS.md`'s Session 06 rule about polling loops applies
   directly, and Session 12's Addendum 2 is a case study in what an unbounded
   or always-failing hardware check costs.
3. **A jam is a normal outcome, not a fault.** Return `DISPENSE_JAM` /
   `DISPENSE_SHORT` and let the UI handle it. **Never call `Error_Handler()`** —
   the same rule Session 12 applied to SD failures.
4. **Leave the stepper de-energised** on every exit path, including the error
   ones — all four coil GPIOs low. The 28BYJ-48's gearbox holds position
   mechanically, so there is nothing to gain from an energised coil and a
   motor and driver IC to cook if you leave one on.
5. **Never infer the count from steps taken.** The stepper is open-loop and
   stalls silently. The IR count is the only thing that knows what actually
   came out; if the two disagree, the sensor is right.

---

## Part B — Wire it into the dispense flow

`STATE_DISPENSING` currently draws a ~2 s progress bar and moves on. It becomes
the real thing.

1. **A dispenser task, or not — decide with evidence.** Session 12's Part A
   moved the NPU into its own task because it was blocking the UI for seconds.
   Dispensing has the same shape: seconds of blocking hardware work. Follow
   that precedent unless you can show it does not fit, and place it in the
   priority scheme derived in `session_12_notes.md` Part A3 — with the
   reasoning written down, as that section did. Note that `OSAL_MAX_TASKS` is
   8 and five are used.
2. **Use the OSAL, and extend it if you must.** The AI's request/response
   handshake over an event flag (`ai_vision.h`, Session 12) is the pattern to
   copy for a dispenser task. Do not invent a second mechanism.
3. **The progress bar becomes real.** It currently animates on a fixed timer;
   it should now track pills actually counted (`2 of 3 dispensed`).
4. **New outcomes need new screens**, built in Session 13's design system:
   - **Jam / short dispense** → "I could not release your pills. Please tell
     your carer." Log it. `STATE_ALERT` already exists for exactly this shape.
   - This is also where a genuine **refill** signal finally becomes possible:
     Session 12 deliberately removed the software stock counter because the
     firmware had no way to know the hopper level. A `DISPENSE_SHORT` after a
     full actuator cycle *is* that signal — it means the hopper is empty.
     Implement refill detection here, where it is measured rather than assumed.
5. **Log everything.** `DISPENSE: <name> <requested> requested, <n> counted`
   and the failure outcomes, to `events.log`. Session 12's Part C5 review
   established that the audit trail records what actually happened, not what
   was intended.

---

## Part C — Keep the software simulation working

**Do not delete the simulated dispense path.** Guard the physical driver behind
a build-time switch (`#define MEDSIGHT_PHYSICAL_DISPENSER 1`) so the firmware
still builds and runs correctly on a board with no hardware attached.

Two concrete reasons, both practical rather than theoretical:

- Sessions 01–13 are the rollback trail. If the hardware misbehaves the night
  before a deadline, you need a build that demonstrates the full flow without
  it.
- The demo video and any remote presentation may need to run on a bare board.

---

## Part D — Documentation

Several documents currently assert, firmly, that this hardware will never
exist. Every one of them must be corrected — not quietly, but with the reason:

1. **`HARDWARE_ARCHITECTURE.md`** — §2 is titled "Dispensing Hardware: NOT
   Built — Design Intent Only" and §3 lists actuators under "Explicitly Not
   Used". Rewrite both. Add the real pin assignments to
   `SOFTWARE_ARCHITECTURE.md` §8's pin map.
2. **`MECHANICAL_DESIGN.md`** — remove the "NOT BUILT" banner for the single
   hopper that now is built; keep it for the 6–8 hopper scaling, which remains
   design intent. `MECHANICAL_DESIGN.md` already specifies the 28BYJ-48 +
   ULN2003 correctly, so no correction is needed there — only the removal of
   its "NOT BUILT" banner for the one hopper that now is.
3. **`SOFTWARE_ARCHITECTURE.md`** — §1's physical-hardware-cut paragraph, §7's
   state machine (the jam edge case it explicitly deleted comes back), the
   module list, and §9's task table.
4. **`MASTER_PROJECT_PLAN.md`** — §2's scope guardrails and §6's
   prototype-vs-final table both state the cut as firm. Update, and add a
   changelog entry.
5. **`THIRD_PARTY_SOFTWARE.md`** — if you add any driver library, it goes in
   the inventory. Contest rule 1.3 is not optional.
6. **`milestones/session_14_notes.md`** — the usual: what was built, every
   deviation, the measured numbers (pills/second, count accuracy over N
   dispenses, jam rate), and what is still untested.

---

## Definition of Done

- [ ] `sessions/session_14/` created per the folder rules; clean baseline build
      reproduced before any edits.
- [ ] **Part 0**: beam polarity measured; pins chosen with VddIO domains
      confirmed; step rate and half-steps-per-pill established and written down;
      separate motor supply wired with common ground.
- [ ] **Part A**: `dispenser.c/.h` — actuator driven, IR sensor counting on
      EXTI with debounce, every wait bounded, no `Error_Handler()` on any
      dispense path.
- [ ] **Part B**: `STATE_DISPENSING` drives real hardware; progress reflects
      pills actually counted; jam/short outcomes have screens and log entries;
      refill detection based on a real short dispense.
- [ ] **Part C**: `MEDSIGHT_PHYSICAL_DISPENSER 0` still builds and runs the
      full simulated flow.
- [ ] **Part D**: all five documents corrected; pin map updated.
- [ ] Build 100% clean, **both** configurations.
- [ ] **On hardware**: 10 consecutive dispenses of 3 pills each. Record how
      many were counted correctly. **A number, not an impression** — this is
      the headline measurement of the session, the way ~89.6% idle was
      Session 12's.
- [ ] Face-match → dispense → IR-counted → confirm works end to end.
- [ ] Zero `tk_*` outside `ms_osal.c`; no embeddings in a real UART capture.

---

## What This Session Does NOT Do

- **No networking.** Permanent.
- **No multi-hopper build.** One hopper. `MECHANICAL_DESIGN.md`'s 6–8 hopper
  architecture stays design intent — but keep `dispenser_dispense()`'s
  signature clean enough that a `hopper_id` parameter would be an additive
  change rather than a rewrite.
- **No new AI models.** Action recognition is a Session 15 question.
- **No UI redesign** — that is Session 13's. New screens here follow the design
  system Session 13 established.
- **No `.ioc` files.**
