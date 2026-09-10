# Session 17 — Physical Dispensing Hardware: Stepper Turntable + IR Pill Counter

> **RENUMBERED FROM SESSION 14.** This prompt was written as Session 14 and
> never run. The project owner decided after Session 15 to do the hardware
> interfacing **last**, so action recognition took the next slot (Session 16)
> and this became Session 17. **14 is a retired number** — nothing was
> dropped, and `MASTER_PROJECT_PLAN.md`'s v13 changelog entry records the
> move.
>
> Everything below is unchanged except the session number and the base-folder
> instruction.
>
> **One correction to Part E, from Session 15.** Part E (the buzzer) was
> already in this prompt when it was Session 14, and it is unchanged in
> scope. What Session 15 settled is *what the buzzer fires on and who it is
> for*: it sounds on the **missed-dose edge**, and it is aimed at a **carer**,
> not at the patient. A missed window is by definition the case where the
> patient did not respond to the on-screen reminder, so beeping harder at them
> is nagging; the buzzer fetches a carer, who opens carer mode → DOSE HISTORY
> and sees who missed and when. That screen exists and is tested.
>
> The firmware hook also already exists: `schedule_service()`'s
> `SCHED_FLAG_CLOSE` branch in `state_machine.c` fires exactly once per missed
> window and is where the `MISSED:` line is written. Part E adds one call
> there.

## Context — the scope change this session represents

**Physical dispensing is back in scope.** Read that carefully, because every
document in this repository written between Sessions 10 and 12 says the
opposite, in strong terms.

The history, so nobody has to reconstruct it:

- The **original Program Plan** (submitted March 2026, `tools/Program Plan
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
  **28BYJ-48 stepper**, with a **3-pin IR sensor module** counting pills as
  they drop, is buildable within the remaining time.

So this is not scope creep, and not a reversal on the merits — it is the
resource assumption behind the v8 cut no longer being true.
`MASTER_PROJECT_PLAN.md` v11 records this properly. **The "no motors, ever"
rule in the older docs is superseded by this session; the "no networking" rule
is not, and never will be.**

---

## Session ordering — read this before anything else

**Sessions 14 and 15 are independent and may be run in either order.** Do not
assume this session follows 13, and do not assume 15 has not already happened.

Before reading anything else:

1. `ls sessions/` and `ls documents/milestones/`. The correct base for this
   session is **the highest-numbered `sessions/session_NN` that has a
   corresponding `session_NN_notes.md` recording a completed, hardware-verified
   run** — not simply this session's number minus one.
2. Copy that folder to `sessions/session_17/` and say in your first message
   which base you chose and why. It will be `session_16` if action
   recognition ran, `session_15` if it was cut.
3. Read the milestone notes for **every** completed session, in order. If
   `session_15_notes.md` exists, Session 15 has already run: it will have
   touched the linker script, the memory map, `patients.dat`'s format (v3), the
   state machine (carer mode, gated registration) and possibly
   `ms_configure_sleep_clocks()`. All of that is now your baseline and none of
   it may be regressed.
4. Record the base you built on at the top of `session_17_notes.md`. A future
   session needs to be able to reconstruct the chain.

If the two sessions turn out to conflict — most likely in `state_machine.c`,
the task table in `main.c`, or GPIO pin assignments — the later session
reconciles, and says so in its notes. Do not silently revert the other
session's work.

---

## How to Start This Session

1. **READ ALL DOCUMENTATION** in `documents/` — especially
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
| New session = new folder | Copy the highest-numbered completed session folder → `sessions/session_17`. |

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

### The sensor: a 3-pin IR module (VCC / GND / OUT)

**Changed after Session 13, by the project owner:** this was specified as a
hand-built emitter/receiver pair soldered from discrete parts. It is now an
off-the-shelf IR sensor module - a small PCB with three pins, an IR emitter
and detector, an LM393 comparator and a trim pot, of the kind sold for a
few rupees as an obstacle / speed / photo-interrupter sensor.

That is a straightforward win. The comparator, the pull-ups, the resistor
network and the threshold all live on the module, so the firmware sees a
clean digital line and the electrical risk mostly disappears.

**Which module, though - this part matters more than the price.** Two shapes
are sold under similar names:

- **Slot type** (U-shaped gap, often sold as a "speed sensor" or
  "photo-interrupter" for encoder wheels). The emitter and detector face each
  other across a fixed few-millimetre gap. A pill falling through the slot
  breaks the beam cleanly, every time, at a known geometry.
- **Reflective type** (FC-51 and lookalikes, emitter and detector side by side
  on the front edge, aimed outward). It detects light bounced back off an
  object in front of it. Range depends on the object's size, colour and
  reflectivity, and is set by a trim pot.

**Prefer the slot type for counting pills.** A pill is small, fast, and may be
white, translucent or dark; a reflective sensor asked to detect it in mid-fall
is being asked to do the hardest version of its job, and a translucent capsule
may simply not return enough light. If only a reflective module is available,
say so in the notes and design the chute so the pill passes within a few
millimetres of the face against a matte dark backdrop.

Firmware consequences:

- **Confirm the output polarity before writing the ISR.** Most of these
  modules are **active LOW** - OUT sits HIGH and is pulled LOW when the beam
  is broken or an object is detected - and many have an onboard LED that
  lights on detection, which makes this a ten-second check with a pill and
  your eyes rather than a scope. Check it anyway; do not assume. Getting it
  backwards counts the gaps between pills instead of the pills.
- **Power it at 3.3 V, not 5 V.** Most of these modules run happily from 3.3 V.
  If you power it at 5 V its OUT swings to 5 V, and unless the pin you chose
  is 5 V tolerant that is a way to damage the MCU. If it must run at 5 V, level
  shift it - a divider is enough for a digital output.
- **Configure the pin as a plain input.** The module drives the line actively
  (or open-collector with its own pull-up); do not add the internal pull-up
  unless the module needs it.
- **Still debounce.** The LM393 gives you hysteresis and a much cleaner edge
  than a discrete build, but a pill tumbling past can still produce a short
  double-break. Keep Part A item 1's minimum-pulse-width filter; the module
  makes it easier, not unnecessary.
- **The trim pot is a physical calibration step**, and it is now part of the
  build procedure rather than a firmware constant. Set it with a real pill,
  and write down where it ended up - "we turned it until it worked" is not
  reproducible.
- **Ambient IR is still a real failure mode.** Sunlight and some indoor
  lighting can hold the detector saturated so the beam never reads as broken.
  Test under the lighting the demo will actually use. The fixes are mechanical
  (shroud the beam path) before they are electrical.
- **Check the VddIO domain of whichever pin you pick** -
  `ENGINEERING_LESSONS.md`'s Session 06 finding. A GPIO in an unpowered domain
  reads a constant value and looks exactly like a sensor that never triggers.

### Power

`HARDWARE_ARCHITECTURE.md` already says it and it is not optional: the stepper
draws current spikes the DK board's regulator should not supply. Use a separate
5 V supply for the ULN2003's motor rail with grounds tied to the board's
ground. Confirm this with your teammate before powering anything.

### Deliverable for Part 0, before any driver code

Write down in the session notes: the exact sensor module used (slot or
reflective, and its markings), its measured idle/broken polarity, its supply
voltage and whether any level shifting was needed, the trim-pot setting and
how it was arrived at, the pins chosen for the four coil lines and the sensor
(with their VddIO domain confirmed), the step rate that runs without
stalling, and how many half-steps of turntable rotation reliably releases
exactly one pill. That last number is the one the whole dispense loop is
built on.

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
   pill tumbling through the beam can produce multiple edges even with the
   module's comparator hysteresis, so a naive counter will over-count.
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
6. **`milestones/session_17_notes.md`** — the usual: what was built, every
   deviation, the measured numbers (pills/second, count accuracy over N
   dispenses, jam rate), and what is still untested.

---

## Part E — The buzzer (the Program Plan's Alert Task)

**Scope: a buzzer and the existing LEDs. No speaker, no amplifier, no codec, no
WAV playback.** That is not a simplification — it is what the contest Program
Plan actually committed to, and matching it is the point.

The Program Plan (`tools/Program Plan 54916.pdf`) specifies, in two places:

- §5 Development Scope — *"Alert / Feedback Module: Buzzer, LED, optional small
  display output"*.
- §6 Feature Description — *"The Alert Task drives a buzzer and LED (green =
  correct, red = incorrect/missed)"*, shown as the fourth box in the task
  diagram: Camera → Inference → Validation → **Alert**.

The device currently has no audible feedback at all, so this is an
unimplemented commitment from our own submitted plan, not a new idea. Session
15 Part A reconciles our documentation against that plan; building this closes
the gap rather than explaining it away.

**This part comes strictly after Parts A–C work.** The stepper and the IR
counter are what this session exists for. If the buzzer is not done by the time
those are solid and documented, stop, write down where you got to, and leave it
— a buzzer that has stolen a pin from the dispenser is a worse outcome than no
buzzer.

### E1. Pick the part and the pin

Two kinds of buzzer, and the choice changes the driver:

- **Active buzzer** — contains its own oscillator. Drive the pin high, it
  sounds; low, it stops. One GPIO, no timer. You get one fixed pitch, which is
  enough for "ready / done / wrong".
- **Passive buzzer** — a transducer with no oscillator. Needs a square wave,
  so a timer PWM channel. Gives you *different* pitches, which is what lets a
  confirmation chirp sound different from a missed-dose alert. Slightly more
  work, meaningfully better result.

**Recommendation: passive, on a timer PWM channel.** Distinguishable tones are
most of the value here, and the cost is one timer.

Either way: most buzzers draw more than a GPIO should source directly. Use a
small NPN transistor or MOSFET with a base/gate resistor, and a flyback diode
if the part is magnetic rather than piezo. Confirm with your hardware teammate.

`ENGINEERING_LESSONS.md` hard rule 4 applies: **trace the schematic first.**
Establish which pin is genuinely free after the dispenser has taken its five,
which timer channel it maps to, and which **VddIO domain** it sits in — the
Session 06 rule that a GPIO bank on this part is unpowered by default, and a
peripheral on an unpowered pin fails silently, has cost this project an
afternoon before.

### E2. Build it as the Alert Task

The Program Plan describes an Alert *Task*, and that is the right shape here
too: a tone must be able to outlive the call that triggered it without blocking
the UI.

- A small task, or a reuse of an existing one, that owns the buzzer and takes
  requests over an OSAL primitive. Session 12 added event flags to the OSAL for
  exactly this kind of thing; add what you need there rather than reaching for
  `tk_*`. **Zero `tk_*` outside `ms_osal.c`.**
- Fit it into the priority scheme derived in `session_12_notes.md` Part A3 —
  and justify where you put it. A buzzer has no hard deadline; it almost
  certainly belongs low.
- **Never block `state_machine_update()` on a tone finishing.** The UI task's
  10 ms poll is what makes touch feel immediate.
- **No `printf` in any timer callback or ISR.** `session_11_notes.md`
  Addendum 8 is the evidence for what that costs on this board.
- A buzzer needs no DMA, so `ms_configure_sleep_clocks()` should not need
  touching. **If your design ends up using DMA anyway, its `LPEN` bit must go
  in there in the same change** — read `session_12_notes.md` Addendum 9 first;
  that bug cost six rounds of debugging and it would present here as a tone
  that stutters or cuts out when the system is idle.

### E3. What it should sound like

Keep it to a handful of short, clearly different patterns, and pair each with
the LED colour the Program Plan already specifies:

| Event | Suggested | LED |
|---|---|---|
| Dose ready / attention | two short rising chirps | — |
| Pills dispensed, correct | one short confirming chirp | green |
| Wrong patient / no match | two low buzzes | red |
| Jam or short dispense | a longer, lower tone | red |

Nothing long, nothing repeating indefinitely. This device may sit in a bedroom.

**Quiet hours and mute.** A device that chirps at 08:00 needs a way to be
silenced. If Session 15's carer mode exists by the time you build this, the
setting belongs there — say so and wire it. If not, note it as a dependency in
the notes so Session 15 picks it up.

### E4. Deliverable

Whether or not the buzzer ships, write the answer down in
`session_17_notes.md`: buzzer type and part number, the pin and timer channel
with the VddIO domain confirmed, the drive circuit, where the Alert Task sits
in the priority scheme, and the tone patterns. If it was not built, say exactly
what remains.

**Documentation rule, and it is not optional:** a previous pass deliberately
removed an unbuilt buzzer claim from the project documents. If the buzzer runs
on hardware, the docs describe what was built and demonstrated. If it does not,
they stay silent. Nothing goes into `MASTER_PROJECT_PLAN.md` or the submission
material that has not run on hardware.

---

## Definition of Done

- [ ] `sessions/session_17/` created per the folder rules; clean baseline build
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
- [ ] **Part E (buzzer)**: an answer written down either way — buzzer type,
      pin and timer channel with VddIO domain confirmed, drive circuit, where
      the Alert Task sits in the priority scheme, and the tone patterns. If it
      was not built, exactly what remains. **Docs mention it only if it ran on
      hardware.**
- [ ] **Part D**: all five documents corrected; pin map updated.
- [ ] Build 100% clean, **both** configurations.
- [ ] **On hardware**: 10 consecutive dispenses of 3 pills each. Record how
      many were counted correctly. **A number, not an impression** — this is
      the headline measurement of the session, the way ~89.6% idle was
      Session 12's.
- [ ] Face-match → dispense → IR-counted → confirm works end to end.
- [ ] Zero `tk_*` outside `ms_osal.c`; no embeddings in a real UART capture.
- [ ] `session_17_notes.md` states at the top **which session folder this was
      based on**, so the chain is reconstructable whatever order 14 and 15 ran
      in.

---

## What This Session Does NOT Do

- **No networking.** Permanent.
- **No multi-hopper build.** One hopper. `MECHANICAL_DESIGN.md`'s 6–8 hopper
  architecture stays design intent — but keep `dispenser_dispense()`'s
  signature clean enough that a `hopper_id` parameter would be an additive
  change rather than a rewrite.
- **No new AI models.** Action recognition is a Session 15 question.
- **No speaker, amplifier, codec or voice output.** Part E is a buzzer and the
  existing LEDs, because that is what the Program Plan committed to ("Alert /
  Feedback Module: Buzzer, LED"). Spoken prompts would be a genuine
  accessibility gain, but they need an I2S amplifier, audio assets, a DMA path
  and its `LPEN` bit — a session's work on their own, and not this session's.
- **No UI redesign** — that is Session 13's. New screens here follow the design
  system Session 13 established.
- **No `.ioc` files.**
