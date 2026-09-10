# Session 15 Notes — Carer Mode, Scheduled Dosing, the Memory Map, and the Submission

## Base folder, and why

**Built on `sessions/session_13/`, copied to `sessions/session_15/`.**

`prompts/session_15.md` says to take the highest-numbered `sessions/session_NN`
that has a matching `session_NN_notes.md` recording a completed,
hardware-verified run — not simply this session's number minus one, because
Sessions 14 and 15 are independent and may run in either order.

At the time this session started:

- `ls sessions/` ended at `session_13`. There was no `session_14`.
- `ls documents/milestones/` ended at `session_13_notes.md`. There was no
  `session_14_notes.md`.
- `session_13_notes.md` records two full hardware rounds (Addenda 9 and 11) and
  an Addendum 12 closing every Definition-of-Done item, including the 30-minute
  idle soak.

So **Session 14 had not run**, and Session 13 was both the highest-numbered
folder and a completed hardware-verified one. The prompt's instruction that B4
"must not reclaim memory that Session 14's DMA buffers are using" is therefore
vacuous for this run — there are no Session 14 DMA buffers — and that is
recorded rather than left as an assumption somebody has to reconstruct later.

**If Session 14 runs after this one**, it is the later session and it
reconciles. The things it will meet are listed in "What Session 14 will collide
with" at the end of this file.

---

## Summary

Five deliverables, in the order the prompt lays them out.

- **Part A** — `documents/PROGRAM_PLAN_RECONCILIATION.md`, a clause-by-clause
  comparison of the March Program Plan against what exists, every row checked
  against the source rather than against this project's own later documents.
  Two of them turned out to be wrong.
- **Part B1** — `schedule_time_source.c`, written at last: the RTC behind one
  interface, with a compressed-day demo mode behind the same interface.
- **Part B2/B2a** — carer mode behind a hidden gesture and a passcode, and the
  same passcode in front of REGISTER PATIENT, closing the open-enrolment hole.
- **Part B3** — dose windows that open and close, driven by a µT-Kernel alarm
  handler, writing the `MISSED:` line this device has never been able to write.
- **Part B4** — the linker script re-derived from the reference manual and a
  real build; `ROM` grown from 511 KB to 1 MB; a named, NPU-reachable
  `AI_ARENA` with a cold-boot self-test; `documents/MEMORY_MAP.md`.
- **Parts C and D** — the README rewritten to lead with measured numbers, the
  concrete µT-Kernel relevance, the debugging stories and the honest
  limitations, with the deferred-work roadmap in that same file rather than a
  separate one.

**Build status: both configurations verified via the real headless STM32CubeIDE
build, before and after every meaningful change.**

| Configuration | Result | text | data | bss |
|---|---|---|---|---|
| Debug (baseline, before any edit) | 0 errors, 2 warnings | 905,568 | 4,008 | 658,624 |
| Debug (final) | 0 errors, 2 warnings | 933,328 | 4,036 | 663,876 |
| Release (baseline, before any edit) | 0 errors, 4 warnings | 759,616 | 4,004 | 658,620 |
| Release (final) | 0 errors, 4 warnings | 778,688 | 4,032 | 663,868 |

The warning counts are identical to the baseline and every warning is
pre-existing, in ST-generated code (`ll_aton_profiler.c` `%d`/`int32_t`
mismatches, and an `ATON.h` array-bounds warning in Release). Session 15 added
one warning of its own — a `-Wformat-truncation` on a page-counter buffer — and
fixed it.

Session 15 costs about **22 KB of ROM and 5 KB of RAM** in Debug, for carer
mode, the RTC time source, the schedule engine and the arena self-test.

---

## Part A — Program Plan reconciliation

`documents/PROGRAM_PLAN_RECONCILIATION.md`. The instruction was to verify each
divergence against the actual code and not to trust the prompt's own list. That
was worth doing: the list was substantially right, and two entries were wrong.

**What the prompt's table got wrong:**

1. **"Debug ROM is 85% full with 78 KB free."** Measured at the start of this
   session: **90.6% full, 48 KB free** (474,080 of 523,264 bytes). The figure
   had drifted since Session 12 and nobody had re-measured it. It mattered,
   because 48 KB is close enough to a link failure to change how urgent B4 was.

2. **The whole "a third model does not fit" argument rested on that number**,
   and B4 then removed it: Debug ROM now has 539 KB free. The conclusion did
   not change, but the *reason* did, and a reconciliation document that
   repeated the old reason would have been repeating something no longer true.
   §8 of the reconciliation says so explicitly: pill classification is now
   blocked on **time and toolchain risk**, not on memory.

**What was checked and confirmed by grep rather than by reading a document:**

- No classification or action-recognition network exists anywhere in the tree.
- No SAI/audio/buzzer code exists; `HAL_SAI_MODULE_ENABLED` is still commented
  out.
- No Wi-Fi, BLE, lwIP, Ethernet or socket code exists anywhere. The
  zero-network claim holds by construction.
- No TFLM. The plan listed "STM32Cube.AI / Renesas RA Smart Configurator +
  TFLM" because the board had not been allocated yet; only the ST path was
  ever taken.

**A row the prompt's table did not have**, and which a careful judge might: the
plan's Open Source Commitment promises "model training scripts". There are
none, because no model was trained — both networks are pretrained public ONNX
models converted with ST Edge AI. Saying that precisely is better than letting
the phrase stand unqualified.

---

## Part B1 — `schedule_time_source.c`

`SOFTWARE_ARCHITECTURE.md` §2 has carried this module as "DESIGNED, NOT YET
WRITTEN" since Session 10, and `MASTER_PROJECT_PLAN.md` §6 has described the
design since v1. It is built to that design and nothing else:

> "implemented as a swappable time source (same pattern as the OSAL: one
> clearly isolated point of substitution), not hardcoded to the fast-timer
> path."

**The substitution point is the deliverable, not the timer.** Everything above
this module asks "what minute of the day is it" and "how long until HH:MM" and
never learns which backend answered. `-DMEDSIGHT_FAST_CLOCK=1` compresses a day
into `MEDSIGHT_FAST_DAY_SECONDS` (default 240 s) so a window opening, being met
and a later one being missed all fit in one take — and **not one conditional
appears anywhere in `state_machine.c` or `carer_ui.c`** as a result.

### The scheduling axis is minute-of-day, everywhere

0..1439. It is small enough to store four per patient in a record written to an
SD card in full on every edit, it has no timezone and no DST, and it is exactly
the granularity a prescription is written in. Seconds appear only where a human
reads a clock.

### RTC bring-up, and the three things that look like a dead peripheral

Manual HAL, no `.ioc`, per `ENGINEERING_LESSONS.md` rule 1. The backup domain
has its own rules and each of them fails silently rather than loudly:

1. **The backup domain is write-protected after reset.** Every register in it
   — the RTC, the backup registers, the LSE control bits — ignores writes until
   `HAL_PWR_EnableBkUpAccess()`. No error; the writes simply do not land.
2. **The RTC clock source is selected through the RCC extended peripheral-clock
   API**, and that selection is itself in the backup domain, so it has to
   happen after (1).
3. **LSE is a crystal and can fail to start.** `HAL_RCC_OscConfig()` blocks on
   it and then returns an error, and the natural reaction is to conclude the
   RTC is broken. It is not. This code tries LSE, falls back to LSI, and
   **prints which one it got**, because "which oscillator is behind the clock"
   is exactly the kind of fact nobody should have to re-derive on a bench at
   midnight.

The prescalers differ between the two (`127/255` for LSE's 32768 Hz,
`127/249` for LSI's ~32000 Hz) and getting that wrong does not fail to build or
to init — the clock just runs at the wrong speed, which is an unpleasant bug to
find from a schedule that fires early.

`HAL_RTC_GetTime()` must be called before `HAL_RTC_GetDate()`: reading the time
locks the calendar shadow registers and reading the date unlocks them, so the
reverse order can return a date one day stale across midnight. That is an ST
HAL contract, not a style preference, and there is a comment saying so.

### "Has the clock ever been set?"

A magic word in RTC backup register 0. Backup registers live in the always-on
domain and survive a system reset — **the same property that made the cold-boot
display bug invisible for nine sessions** (`ENGINEERING_LESSONS.md`), used
deliberately here instead of tripped over. Only removing VDD clears it.

`time_source_init()` is called from `main()` rather than from a task: the
backup domain must be unlocked and the oscillator started before anything asks
the time, and the LSE start-up is a blocking wait of up to a second that has no
business inside a task with a deadline. Its failure is **deliberately not
fatal** — a device with no working RTC keeps dispensing on demand, it just
cannot remind anyone, and the home screen says so.

---

## Part B2a — registration is authorised (the important one)

This is the most important change in the session. It closes a real hole.

**The old behaviour, stated plainly:** anyone could walk up, tap REGISTER
PATIENT, enrol their own face and their own dose size, and then use DISPENSE to
have the machine hand them medication — and the audit log would record it as a
legitimate, face-matched dispense to a registered patient, because from the
device's point of view that is exactly what it was. Face recognition was doing
its job perfectly. The gallery it matched against accepted anyone who asked.

Every other control in the device rests on that gallery being trustworthy. With
enrolment open, identification is theatre.

**The fix:** REGISTER PATIENT now leads to `STATE_PASSWORD` and the existing
Session 09 flow runs only after the carer passcode validates.

**Gated at the start, not the end** — and having implemented it, the prompt's
three reasons are right and there is a fourth:

1. It does not waste the user's time before rejecting them.
2. No face and no name are taken from someone who was never authorised, which
   `COMPLIANCE_PRIVACY_POSTURE.md` would otherwise have to apologise for.
3. There is no half-written state to unwind on a mid-flow abort.
4. **The fourth, found while writing it:** the gallery-full check already
   happens before the gate, and that ordering is right — being told the device
   is full is more useful than being asked for a code and only then told. Two
   pre-checks in sequence only stay legible if both are before the flow starts.

**One prompt screen, one validation routine.** `carer_pass_check()` in
`carer_ui.c` is the only function in this firmware that compares a passcode,
and `carer_ui_draw_prompt()` is the only screen that collects one. The two
entry points differ by exactly one variable (`s_pw_next`) and one piece of
wording.

**The wording is part of the design, not decoration.** The person most likely
to see the registration version of the prompt is a patient who pressed the
wrong button, so that version is titled *"A CARER SETS THIS UP"* and says *"Ask
your carer to enter their code"* — not "Access denied".

### The passcode itself, and what it is not

- **Numeric, not typed.** A deliberate choice: the keypad's targets are 128×66
  px, nearly twice the area of registration's QWERTY keys, which matters for
  whoever is actually holding the device. Nothing about a word is more secure
  than digits against this threat model.
- **Stored as a salted 32-bit FNV-1a hash** in `carer.cfg`, with the same
  magic/version header pattern `patients.dat` uses, so a wrong-shaped file says
  so rather than being indistinguishable from no file. Not a password hash in
  the real sense — no work factor, no per-device salt — and the reason it is a
  hash at all is narrow and worth stating: a carer's chosen code is likely a
  code they use elsewhere, and writing it in clear on a removable card would
  leak that reuse for no benefit.
- **Compared in constant time.** Nearly free on two 32-bit words, which was the
  prompt's own test. Written out anyway because the shape is the habit.
- **Rate-limited**: five attempts, then 30 seconds. The counter is in RAM, so a
  power cycle clears it — **deliberate**: persisting it would turn a wrong tap
  into an SD write, and would hand anyone a way to wear the card out or to lock
  a device out permanently by pulling power at the right moment. Recorded in
  `COMPLIANCE_PRIVACY_POSTURE.md` §6 and listed as future work in the README,
  not hidden.
- **Never logged, in any form** — not the digits, not the hash, not the length,
  not on a failure. A *lockout* is logged, because the event is auditable and
  the secret is not.
- **Ships as a build-time default** (`1379`) so a fresh device is usable, and
  the device says on every boot when it is still on it.

### The entry gesture

Five taps on the home screen's title bar within three seconds. The title bar
because it is the one element on that screen unmistakably **not** a button, so
tapping it repeatedly is not something anyone does by accident, and it is
320×68 px — reliably hittable with an unsteady hand. Any tap elsewhere resets
the count, so five accidental taps spread over a session never accumulate.

Five: one or two are plausible accidents, ten is a chore to explain over the
phone. Three seconds: comfortably long for a deliberate five taps by an older
hand, far too short for accidental ones to add up.

---

## Part B2 — what carer mode contains

Seven screens, all drawn from Session 13's design system so they look like the
product rather than like a settings menu bolted on. Full descriptions are in
`UI_SCREEN_INVENTORY.md` screens 13–22.

1. **Set the clock** — five fields, one selected, one big − and one big +. More
   taps than typing a number per field, and the right trade for this device: it
   is **impossible to enter an invalid value** (month lengths and leap years
   clamp the day as you go), there is nothing to validate, and every control is
   thumb-sized. A carer sets the clock once.
2. **Per-patient dose schedule** — four slots, 15-minute steps. Fifteen because
   it is the granularity a prescription is written in ("half eight in the
   morning") and it turns a 24-hour sweep into 96 taps instead of 1440. A new
   slot starts at 08:00, not midnight: the most common first dose, and far from
   the wrap-around where − and + are most confusing.
3. **Set the dose size** — 1–10, shown with **the same pill lentils the
   dispense screen uses**, so "3" here and three gems during a dispense are
   visibly the same fact.
4. **Review the log** — and this is the strongest thing in carer mode. The
   audit trail has existed since Session 06 and reading it has always meant
   taking the card to a laptop. It now reads the **end** of `events.log` (a new
   `SD_Read_File_Tail()`, because the file is append-only and unbounded), keeps
   only the lines that answer the question — CONFIRMED, MISSED, DISPENSE,
   SKIPPED — filters to the selected patient, and shows the last eight. Missed
   doses are the one line that is not grey.
5. **Delete a patient** — the function `COMPLIANCE_PRIVACY_POSTURE.md` §5
   recorded as deliberately never built, because the documents described it
   with no authentication. Behind the passcode it becomes appropriate. It
   `memset`s the **whole record**: a cleared name with a live embedding would
   still match a face, it would just match it to nobody. The confirmation
   **times out to KEEP**, which is the only correct default for a destructive
   action and is the opposite of every other timeout in this UI.
6. **Change the passcode** — two stages, and deliberately **no** "enter the old
   code first": the carer entered it thirty seconds ago to get here, and asking
   twice teaches people that this device asks for its code more often than it
   needs to.

### Schedule storage — `patients.dat` v2 → v3

`PatientRecord` grew `dose_time_count` and `dose_time[4]`, so the record size
changed and every v2 card is stale. **Session 12 built the versioned header for
exactly this**, and it did its job without anyone having to think about it: a
v2 card is reported by name and version and rejected, rather than silently
loading as an empty gallery.

Carers re-register once. The README says so, which is the other half of the
feature.

### Two decisions the prompt asked to be made deliberately

**Should registration still collect the dose size?** The prompt asked to
reconsider once carer mode could set it. **Decision: keep it in registration.**
After B2a the registration flow is *already* carer-operated — the carer entered
their code to start it — so the person setting the dose is now the right
person, which was the original objection. Removing the step would shorten a
flow that works, break a screen that Session 13 redesigned, and leave a
freshly-enrolled patient with a dose of zero until somebody went into carer
mode. The dose is editable in carer mode afterwards, which is what was actually
missing.

**The greyed-out hopper slots (`HOPPER_LIVE`).** The prompt says that if this
session builds any part of carer mode, the pill-count screen's B/C/D slots
"need colours and touch targets turned on, not a redesign". **Decision: leave
them off, and record why.** That instruction's premise is that carer mode makes
those slots mean something — but Session 15's carer mode adds no multi-hopper
support, and `PatientRecord` has no `hopper_id` field (adding one with a single
hopper would be dead weight in a record written to a card in full on every
edit). Lighting up three slots that dispense nothing would claim a capability
the device does not have, which is precisely what the greyed-out design was
chosen to avoid. They light up when there is a second hopper, and the README's
future-work table says what that needs.

---

## Part B3 — the scheduled-dose flow

### Why an alarm handler, and not a task that polls the clock

A dose window is a one-shot deadline at an absolute time of day, three or four
times a day. A task waking every ten seconds to compare the clock against a
schedule would wake **8,640 times a day to act four times** — and it would
spend the one number this project has been measuring since Session 12 (~89% of
wall-clock time asleep) in order to do it. `osal_delay_ms()` until the next dose
blocks a whole task on nothing. An event flag has no notion of time at all.

`tk_cre_alm`/`tk_sta_alm` is an object that costs exactly nothing until it
fires. Rule 1.4's "real-time performance" and its "power saving" are answered
here by the same mechanism — and it is, not incidentally, **the mechanism the
original Program Plan named**: *"The Camera Task wakes either on a scheduled
µT-Kernel alarm (aligned to dose times) or on user button press."* That sentence
had been unimplemented since March.

### One alarm, both edges

A dose window has an opening and a closing edge and both matter:

```
alarm fires (OPEN)  -> handler sets SCHED_FLAG_OPEN, returns
  UI task next tick -> banner on the home screen, SCHEDULE: line to the log,
                       mascot -> MASCOT_ACTIVE
                    -> the SAME alarm re-armed for the window's close
  patient dispenses inside the window
                    -> "CONFIRMED: <name> took the 08:00 dose (on time)"
alarm fires (CLOSE) -> handler sets SCHED_FLAG_CLOSE, returns
  UI task next tick -> if unserved: "MISSED: <name> did not take the 08:00 dose"
                    -> the alarm re-armed for the NEXT dose
```

One object, one pending expiry at a time, and nothing anywhere polls the clock.
The window is 30 *schedule* minutes, so in demo mode it compresses with
everything else and a missed dose is filmable in seconds.

**The `MISSED:` line is the point of the whole feature.** This device could
always record that a dose was given. It could not record that one was not.

### Handler context is what shapes the design

`schedule_alarm_handler()` runs in µT-Kernel's timer context. It may not block,
may not `printf` (`session_11_notes.md` Addendum 8 is the record of what a slow
handler path costs on this hardware) and may not call anything unbounded. It is
**three lines**: set one bit in an event flag and return. Every piece of real
work — reading the gallery, drawing, logging — is done by the UI task. That is
the same division of labour Session 12 established between the UI and the AI
task, applied to time instead of to inference.

### A correctness detail worth writing down

`schedule_find_next()` treats a dose time equal to "now" as a **whole day
away**, not zero. Without that, re-arming immediately after a window fires
would fire it again instantly and spin. The same rule is in
`time_source_ms_until()`.

The consequence is that at the instant the OPEN alarm fires, the dose that just
came due is the *furthest* one by that function's reckoning, not the nearest —
so `schedule_service()` recovers it from the clock instead, matching any
scheduled time within ±1 minute of the current minute. The tolerance exists
because the alarm's expiry and the RTC's minute boundary are two different
clocks with no reason to agree to the millisecond.

### No sixth task

Scheduled dosing looks at first like it wants one. It does not: its work is an
alarm handler (which is not a task) plus a few lines of reaction in the UI
task, which already runs every 10 ms and already owns every screen the reaction
touches. A sixth task would have needed its own stack and its own claim on the
framebuffer for no behaviour that is not already there. `OSAL_MAX_TASKS` is 8
and five are used, so the room exists — it just is not needed. Recorded because
"we did not add a task" is a decision, the same way Session 12's two rejected
idioms were.

---

## Part B4 — the memory, claimed and proved

Full account in **`documents/MEMORY_MAP.md`**. The findings, in the order they
changed something:

### 1. The NPU's activation footprint is one contiguous 1.53 MB block

Extracted mechanically from every address literal in the generated networks —
642 references across `LL_ATON_Cache_MCU_Clean_Range`,
`..._Invalidate_Range`, `start_offset` and `addr_base` — and merged:

```
0x34200000 - 0x34387FFF     1,605,632 bytes
```

Two documents were wrong about this and are corrected:

- **`main.c`'s `ms_configure_sleep_clocks()` comment said the NPU's activation
  scratch "only ever overlaps `BUFFER_ADDRESS`".** It does not. Corrected in
  place, with the arithmetic.
- **Session 13's second framebuffer could never have worked.**
  `GUI_BUFFER_ADDRESS` is `0x342BB800`–`0x34376FFF`, entirely inside that
  block. Session 13's canary found both ends clobbered and reverted the
  feature; this is the arithmetic that explains it, which means the revert was
  correct rather than merely cautious.

### 2. AXISRAM5 and AXISRAM6 are not powered at reset

`stm32n6xx_hal_msp.c` enables **AXISRAM3 and AXISRAM4 only**. The other two are
brought up by `SystemInit_POST()` inside `npu_init.c`, which runs from
`aiPreInitialize()` **on the AI task, after the scheduler starts**.

This is the trap in the whole exercise. Between reset and the AI task
finishing, the upper two banks are memory the CPU can address and that does not
exist. It does not fault; it returns whatever an unclocked bank returns — which
is exactly the shape of result that gets written into a document as a fact.

Enforced in code: `ms_memtest_arena()` is called from `state_machine_init()`
**immediately after `ai_vision_wait_init()` returns**, and from nowhere else.

### 3. `LPEN` — checked, and no new bit was needed

`session_12_notes.md` Addendum 9 is a hard constraint on this session and it
was treated as one. The arena is in AXISRAM6; `ms_configure_sleep_clocks()`
already sets `AXISRAM3LPEN`–`AXISRAM6LPEN`, and `Set_CLK_Sleep_Mode()` in
`npu_init.c` independently sets all six later in boot. **No new bit was
required, and that was checked rather than assumed.**

Found while checking, and recorded because it is a live hazard for whoever
comes next: **AXISRAM1 and AXISRAM2 — where all the code, `.rodata` and `.bss`
live — are not in `ms_configure_sleep_clocks()`'s set at all.** Harmless today
because no DMA master reads from them (the SD path is polling, so the CPU is
awake throughout). It becomes a real bug the moment anything DMAs to or from a
`.bss` buffer.

### 4. The linker script, re-derived twelve sessions late

The 511 KB / 1023 KB split came from the ST `DCMIPP_ContinuousMode` example
this repository was founded on in Session 03. Neither number was ever derived
from what MedSight needs, and between them sat **513,024 bytes of AXISRAM2 that
nothing has ever claimed**.

Measured immediately before the change — and this is the finding:

| | used | of | | free |
|---|---|---|---|---|
| **Debug ROM** | 474,080 | 523,264 | **90.6%** | **48 KB** |
| Debug RAM | 802,304 | 1,047,552 | 76.6% | 239 KB |
| Release ROM | 342,720 | 523,264 | 65.5% | 176 KB |
| Release RAM | 786,944 | 1,047,552 | 75.1% | 254 KB |

Forty-eight kilobytes is roughly one feature away from a link failure.

`ROM` now takes **all** of AXISRAM2: origin `0x34100000`, length 1024 KB,
ending exactly where the framebuffer begins. `RAM` is untouched — AXISRAM1 is
physically 1 MB, the first 1 KB belongs to the boot ROM, so 1023 KB is already
all of it, and at 77% it is not the constraint anyway.

After, including everything this session added:

| | used | of | | **headroom** |
|---|---|---|---|---|
| Debug ROM | 496,000 | 1,048,576 | **47.3%** | **539 KB** |
| Debug RAM | 812,680 | 1,047,552 | 77.6% | 229 KB |
| Release ROM | 356,736 | 1,048,576 | 34.0% | 675 KB |
| Release RAM | 797,312 | 1,047,552 | 76.1% | 244 KB |

**What was deliberately not changed:** `.rodata` stays in `RAM`. Moving it into
the now-roomy `ROM` region would work, but the NPU cannot reach either bank so
it buys nothing that matters, and this project's one lesson about `.rodata`
placement (`AI_LESSONS.md` — `.rodata` in AXISRAM1 hard-faulted the NPU) is a
reason to leave a working arrangement working.

### 5. `AI_ARENA`

```
_ai_arena_start = 0x34388000
_ai_arena_end   = 0x343BF000
size            = 225,280 bytes (220 KB)
section         = .ai_arena, (NOLOAD)
```

Confirmed in the `.elf`, not just in the script. A 4 KB guard band sits above
it because `npuRAM6`'s own declaration stops 8 bytes short of the bank boundary
(`size=458744`) with no in-tree explanation, and the arena's last page should
not depend on an unexplained 8 bytes.

**The self-test** (`ms_memtest.c`) writes and reads back the whole arena three
times: `0xA5A5A5A5`, `0x5A5A5A5A`, and **address-in-address** — the only
pattern that catches an aliased or short-decoded bank, where a write lands
somewhere else and both constant patterns still "pass". Each pass does a
`SCB_CleanInvalidateDCache_by_Addr()` between the write and the read, because
the arena is ordinary cacheable memory and a write-back cache would otherwise
satisfy every read from L1 and let an unpowered bank pass — the same asymmetry
Addendum 9 is about.

**Not yet run on hardware. See the checklist.** Until that PASS line is in a
log, the arena is a hypothesis with good arithmetic behind it.

### 6. The `.xspi2` layout was verified unchanged

`ENGINEERING_LESSONS.md` requires this whenever anything about the model files
or their compilation changes, and moving the ROM origin is close enough to
count. Compared against the Session 13 Debug ELF:

```
71000000 b _ec_blob_faceid_1
71000700 b _ec_blob_faceid_6
71000c40 b _ec_blob_faceid_10
710016c0 b _ec_blob_faceid_14
```

Identical in Session 13 Debug, Session 15 Debug **and** Session 15 Release. The
already-flashed weight image still serves this build; **nothing needs
re-flashing to external memory for Session 15.**

### 7. Does a third model fit? Numbers first, then the answer

**On memory alone, a small pill classifier now fits** — and that is a change
from the answer this project has given since Session 12. 539 KB of ROM free in
Debug, a 220 KB NPU-reachable arena, and 64 MB of external NOR of which 290 KB
is used. That is not the constraint any more.

**Action recognition: this said "still does not fit" and it was wrong.** See
Addendum 2 — the 393 KB frame ring never had to be in SRAM, and there is 16 MB
of NPU-reachable PSRAM that §1b of `MEMORY_MAP.md` lists and this analysis
failed to carry into the budget.

**What decides it is time.** Integrating a third network on this board is not a
small job with a known cost, and the evidence is this project's own history:
Session 08A spent a whole session proving the toolchain with a *trivial* model
and hit an NPU bus fault and an IDE lock-up doing it; Session 08B needed
weights flashed to **three separate addresses** across two rounds of debugging;
Session 12 found Debug and Release emitting the same blobs in *opposite order*,
so only one configuration could ever run against one flashed image. None of
that is a memory problem and all of it would recur.

The owner's standing instruction is that this happens only if it is a
certainty, not a maybe. **It is a maybe. So: no.** What the session leaves
behind instead is a foundation — the arena exists, its address and size are
documented, and `MEMORY_MAP.md` §5 is a runbook for dropping a model into it.

---

## Part B5 — current measurement: not attempted

`tools/x-cube-n6-ai-power-measurement` has been in this repo unused since the
beginning, and "42 mA idle against 180 mA during inference" would be a
categorically stronger claim than a percentage.

**Not attempted, and the reason is simple rather than technical: it needs
hardware, and this session had none.** The measurement requires a board, a
current probe or the DK's own sense resistor, and somebody watching an
instrument — none of which a code session can do. The prompt's own instruction
was "cheap if it works; abandon it quickly if it does not — it is a
nice-to-have, not this session's job", and starting it would have consumed time
that Part B4's arena work needed.

It stays available: the tool is in the tree, the idle percentage is already
measured, and the hardware checklist below is where it would slot in if there
is time before the deadline.

---

## Parts C and D — the submission

`README.md` was rewritten rather than appended to, because Part C is about what
a judge reads first and the old file opened with a paragraph about pill
organisers.

It now opens with **the numbers** — 89.6% idle, 209 ms inference, the memory
footprint, six modified µT-Kernel files out of ~230 with none of them a system
call — each traceable to a session's notes. Then **what makes this a µT-Kernel
application specifically** (event-flag inference dispatch, the alarm handler,
the rate-monotonic derivation, `low_pow()` as a real WFI, deferred object
creation, and the two idioms deliberately rejected). Then **the debugging
stories**, seven of them, each in a short paragraph with a pointer to the
addendum that has the full account. Then **the honest limitations**, including
the ones nobody would notice: that the gallery-full path has never been
exercised on hardware, and that in demo mode the schedule rides a tick counter
that wraps after 49 days.

Three claims in the old README were **wrong** and are fixed:

1. "As of Session 13 the UI owns **two** framebuffers … so the screen no longer
   visibly corrupts during face capture." Session 13 reverted that; there is
   one framebuffer and the panel is blanked instead.
2. "**No on-device patient deletion**" and "**Enrollment is unauthenticated**"
   were listed as limitations scoped for Session 15. Both are now built.
3. Build instructions pointed at `sessions/session_13`.

**Part D's roadmap is in the README's future-work section**, per the explicit
instruction not to create a separate document — seven items, each with what it
is, why it was deferred, what it needs first, and a rough cost. One of them is
new to this session: persisting the passcode lockout across a power cycle, with
the RTC backup registers as the obvious home.

**No Session 16 is proposed.** Every prior changelog entry that invented an
optional stretch session — 12B, and the old numbering's 15 and 16 — ended with
it being dropped, and the calendar does not support adding to that list.

---

## Files changed

**New:**

| File | What |
|---|---|
| `FSBL/Src/schedule_time_source.c` + `Inc/` | RTC / demo-clock behind one interface |
| `FSBL/Src/ui/carer_ui.c` + `Inc/ui/` | the passcode gate and seven carer screens |
| `FSBL/Src/ms_memtest.c` + `Inc/` | the AI arena self-test |
| `documents/PROGRAM_PLAN_RECONCILIATION.md` | Part A |
| `documents/MEMORY_MAP.md` | Part B4 |
| `documents/milestones/session_15_notes.md` | this file |

**Changed:**

| File | What |
|---|---|
| `STM32CubeIDE/FSBL/STM32N657X0HXQ_AXISRAM2_fsbl.ld` | `ROM` 511K→1024K at `0x34100000`; new `AI_ARENA` region and `.ai_arena` section |
| `FSBL/Inc/ms_osal.h`, `Src/ms_osal.c` | `osal_alarm_create/start/stop` over `tk_cre_alm`/`tk_sta_alm`/`tk_stp_alm`, with deferred creation in `usermain()` |
| `FSBL/Inc/ai_vision.h`, `Src/ai/ai_vision.c` | `PatientRecord` +schedule; format v2→v3; `gallery_set_schedule/set_dose/delete_patient` |
| `FSBL/Inc/ui/state_machine.h`, `Src/ui/state_machine.c` | ten new states, the gesture, the gate, the schedule engine, the dose-due banner |
| `FSBL/Inc/sd_logger.h`, `Src/sd_logger.c` | `SD_Read_File_Tail()` for log review |
| `FSBL/Src/main.c` | `time_source_init()`, `state_machine_service_init()`, corrected sleep-clock comment |
| `FSBL/Inc/stm32n6xx_hal_conf.h` | `HAL_RTC_MODULE_ENABLED` |
| `STM32CubeIDE/FSBL/.project` | five new linked sources (three app, two HAL RTC) |
| `README.md` | rewritten — Parts C and D |
| `documents/MASTER_PROJECT_PLAN.md` | v12 changelog; §6 time source and deletion now built; §7; §10 index |
| `documents/SOFTWARE_ARCHITECTURE.md` | §1, §2, §3, §4 (alarms), §5 (MASCOT_ACTIVE), §6 (v3 record), §7 (gate + schedule), §9, §10, new §11 |
| `documents/COMPLIANCE_PRIVACY_POSTURE.md` | §1 note, §2, §4, §5 (delete built), §6 rewritten |
| `documents/UI_SCREEN_INVENTORY.md` | screens 13–22, the gesture, the banner |
| `documents/THIRD_PARTY_SOFTWARE.md` | HAL row: RTC and RAMCFG modules |

---

## Self-review — the project's standing greps

- **No `tk_*` call outside `ms_osal.c`.** Two hits in application code, both in
  comments explaining why the OSAL wraps what it wraps. The new
  `osal_alarm_*` primitives keep every `tk_cre_alm`/`tk_sta_alm`/`tk_stp_alm`
  inside `ms_osal.c`, as Session 12 did for event flags.
- **No embeddings or biometric payloads over UART.** Confirmed across the three
  new modules and the changed ones. The only new secret in the firmware — the
  passcode — is never printed in any form.
- **No `printf` on a hot path or in handler context.** The alarm handler is
  three lines and calls one non-blocking OSAL function.
- **Every new SD write checks its result and the UI says which happened.**
  Session 13 Addendum 9's lesson applied to five new save paths: a carer edit
  that reached RAM but not the card raises an alert saying the change works now
  and reverts on the next restart, rather than reporting "Saved".

---

## Hardware round 1 — results

Rounds 1 to 3 of the checklist below were run on the board. **All three
passed.** The parts that matter:

### The arena is proven

```
ARENA SELFTEST: testing 0x34388000-0x343BEFFF (225280 bytes)...
ARENA SELFTEST PASS: 225280 bytes writable and readable (3 patterns, 28ms).
```

From a genuinely cold boot, all three patterns including
address-in-address, over the full 220 KB. `MEMORY_MAP.md` §3's caveat is
lifted: the arena is a resource, not a hypothesis.

### The rest of round 1

| | Result |
|---|---|
| RTC oscillator | **LSE started** — `time_source: RTC up on LSE (32.768 kHz crystal)`. The LSI fallback path is therefore untested on this board, which is the good outcome: LSE is the accurate one. |
| `MEMLPENR` | `0000000F` — AXISRAM3–6, as intended. `BUSLPENR=00000003`, `APB5LPENR=00000046`, `AHB5LPENR=80000082`, all unchanged from Session 13. |
| Cold-boot display | Correct, and **held flat at 88.8% idle for a full 30-minute soak**. This mattered more than usual: the linker change moved every byte of code to a new address, and Session 12's display fault was density-of-sleep dependent. |
| Clock not set at boot | `schedule: clock not set - no dose windows armed.` — the intended non-fatal path. |
| Default passcode warning | Printed, as designed. |

### Round 2 — the gate holds

The passcode prompt appears **before the camera runs**. Five wrong entries
produced the lockout and the audit line. `1379` let the Session 09 flow
through unchanged, and the capture came back at **209 ms** — identical to
Session 13, which is the invariance `AI_PIPELINE.md` §5 predicted.

`SD_Write_File(patients.dat): 1732 bytes written.` is the v3 format
round-tripping: 12-byte header + 10 × 172-byte record = 1732 exactly.

### Round 3 — carer mode

Gesture, clock set (`CARER: clock set to 10 Sep 2026  16:39`), schedule,
dose size, log review, passcode change and `carer.cfg` write, and the delete
all worked. `schedule: next dose 16:45 for 'DOUSIK' in 360000ms` is
arithmetically right — six minutes from 16:39.

**An unplanned test passed in the middle of it.** The card was pulled during
carer mode and the log shows the Session 13 hot-plug path doing its job:
`media fault ... card unmounted, will retry` → two
`disk_initialize: HAL_SD_Init failed (1)` → `SD: card remounted OK.` That is
Session 13 Addendum 9's `HAL_SD_DeInit()` fix, still working three sessions
later, with Session 15's carer writes on top of it.

### Two things that did NOT get tested in round 1 — one closed in round 3

- **A carer edit with no card — CLOSED.** Round 3 caught it exactly as
  designed:
  ```
  SD_Write_File(patients.dat): f_open failed 1
  gallery_set_schedule: slot 0 now has 1 dose time(s) (NOT SAVED TO SD).
  STATE_ALERT: NOT SAVED TO CARD
  ```
  The edit is live in RAM, the screen says it reverts on restart, and the
  card remounted cleanly afterwards. Session 13's honesty rule holds through
  five new save paths.
- **The v2 → v3 card rejection — still unexercised.** The card was empty on
  the first run and has been v3 ever since, so `gallery_init()` has never met
  a v2 file on this hardware. The path is reviewed but untested, and it is
  now awkward to test without a preserved v2 card. Do not claim it.

---

## What still needs a hardware run

Rounds 1–3 above are done. **Rounds 4 and 5 are not, and neither are the two
gaps just listed.**

Run them roughly in this order — each later item assumes the earlier ones
worked.

### Cold boot first, and read the UART  — DONE, see results above

1. **`time_source:` line.** Says which oscillator started (LSE or LSI), which
   mode, and whether the clock has ever been set. If neither oscillator starts,
   the device should still boot and dispense — that path matters as much as the
   happy one.
2. **`ARENA SELFTEST PASS: 225280 bytes …`** — the Part B4 deliverable. A FAIL
   line names the first bad word and its address. **From a cold boot**, because
   AXISRAM6's power-up is part of what is being tested.
3. **`sleep clocks:` readback** unchanged from Session 13.
4. **The home screen comes up** and the 30-minute idle soak still passes. The
   linker change moved every byte of code to a new address; the display fault
   Session 12 spent six rounds on was density-of-sleep dependent, so a short
   run is not evidence.

### The gate (B2a) — verify the hole is actually closed  — DONE

5. Tap REGISTER PATIENT. The passcode prompt must appear **before the camera
   ever runs**. Cancel: back to home, nothing enrolled.
6. Enter a wrong code five times: the prompt refuses for 30 seconds and
   `SECURITY: carer passcode locked out` reaches the log.
7. Enter `1379`: the Session 09 registration flow runs unchanged, end to end.

### Carer mode — DONE apart from the no-card edit (item 14)

8. Five taps on the home title bar within three seconds → the prompt. Then try
   four taps, and taps spread over ten seconds: neither should open it.
9. Set the clock. Check the strip on every carer screen agrees.
10. Give a patient two dose times. Reopen the screen and confirm they came back
    sorted and correct — this is also the v3 `patients.dat` round-trip test.
11. Change the dose size. Delete a test patient and confirm both the name and
    the face are gone (re-run DISPENSE against that face: it must not match).
12. Review the log with a patient selected.
13. Change the passcode, power-cycle, and confirm the new one still works —
    that is the `carer.cfg` round trip.
14. **Pull the SD card** and repeat one carer edit. It must say "works now,
    reverts on the next restart", not "Saved".

### Scheduled dosing (B3) — easiest in demo mode

15. Build with `-DMEDSIGHT_FAST_CLOCK=1`. Set a dose time a few compressed
    minutes ahead.
16. **Window opens:** the banner names the patient and the time, and
    `SCHEDULE: dose due HH:MM for <name>` reaches the log.
17. **Dose taken inside the window:** the log line must be
    `CONFIRMED: <name> took the HH:MM dose (on time)`, not the generic one.
18. **Window closes unserved:** `MISSED: <name> did not take the HH:MM dose`.
    This is the line the whole feature exists for.
19. Repeat once in real-clock mode with a dose time a few minutes ahead, to
    confirm the RTC path and the long alarm delay behave the same way.

### The old flows, unchanged

20. Register → dispense → confirm, and the unrecognised-face retry, exactly as
    Session 13 left them. Nothing in this session should have touched them, and
    that is the assertion to test.
21. `patients.dat` from a Session 13 card: the log must say it is format v2 and
    this firmware wants v3, and the gallery must start empty rather than
    silently loading.

### Optional, if there is time

22. **Part B5's current measurement.** The tool is in `tools/`; the numbers
    would be worth having.

---

## Addendum 1 — what the first hardware round changed

Four things came back from the board. Two were layout defects I shipped, one
was a usability failure that was my design decision rather than a bug, and one
was a new requirement.

### A1.1 — "the text inside the boxes is not within"

**Root cause, and it is a defect in the shared primitive rather than a
coordinate that needed nudging.** `gui_draw_button()` chose its typeface from
the button's HEIGHT alone (`h >= 78` → `ui_font_lg`, the 34 px display face)
and then applied that same face to BOTH lines. The carer menu's buttons are
340×92 with a subtitle, so `"schedule, dose, delete"` was being set in the
display face at roughly 440 px inside a 340 px box.

Nothing anywhere measured a label against the box it had to fit in —
`gui_font_width()` has existed since Session 13 and simply was not being
consulted.

Two fixes, both in the primitive so every caller benefits:

1. **A subtitle is subordinate type and is never the same size as its
   label.** `line2` now always renders at least one step down the face
   ladder, and in `THEME_INK_SOFT` rather than full ink.
2. **Both lines step down until they fit**, measured, and anything still too
   long is truncated with `..` by a new `gui_font_text_ellipsis()`.

That second one matters beyond this bug: **patient names are up to 31
characters and this UI does not control them.** The patient list and the
patient-menu title were both drawing a name at whatever width it happened to
be. They now clip to a measured column. `gui_draw_title_bar()` cannot do this
itself (it takes no width), so the patient menu draws the bar with an empty
title and places the name itself.

### A1.2 — "line spacing issues in the subtexts beneath titles"

**Root cause: there was no shared vertical rhythm, so six screens each picked
a y by eye and two of them collided.**

`gui_draw_title_bar()` puts the title at `TITLE_Y` (38) in the 41 px face and
a 5 px accent rule at `TITLE_Y + TITLE_H + 4`, so the header block ends at
**y = 91**. Against that:

| Screen | drew subtext at | consequence |
|---|---|---|
| passcode prompt, log | 92 | one pixel below the rule — visibly cramped |
| clock, dose times, dose size | 96 | five pixels |
| dose times | 96 **and** a two-line clock strip hard-coded at 108 | **10 px overlap — the two drew through each other** |

`gui_draw.h` now states the rhythm once — `HEADER_BOTTOM` (91), `SUBTEXT_Y`
(100), `STATUS_Y` (126) — and every carer screen uses those instead of a
literal. `draw_clock_strip()` became **one** line at a caller-chosen y, with
the "clock never set" case folding into that line rather than adding a second
one, so no screen can collide with it any more.

### A1.3 — the clock screen: right constraint, wrong optimisation

Reported as "clicking plus is kind of annoying", which is generous. Setting a
year, a day and a time from the power-on default with a ±1 stepper is on the
order of **a hundred taps**.

The original design optimised for *"impossible to enter an invalid value"* —
month lengths and leap years clamped the day as you moved. That goal was
achieved and it was the wrong goal. **A carer sets the clock while looking at
their phone; they already know the date. The job is to let them type it.**

It is now the **same numeric keypad as the passcode prompt**: twelve digits
landing in `DD/MM/YYYY  HH:MM` with `_` for what is still to come, the current
clock underneath as a reference, DEL to backspace, OK to validate and save.
Thirteen taps against a hundred, and validation moved to one function call at
the end — which also lets the error name the field that is wrong ("That day
does not exist in that month") instead of just refusing.

Reusing the passcode keypad rather than building a second numeric input means
there is **one** numeric input in this UI, and a carer who has already entered
their code knows how the screen works. Its digit buffer is deliberately
separate from the passcode's: a half-typed date and a half-typed passcode must
never be able to reach each other.

**Stepping is kept on the two screens where it is genuinely the right
control** — dose times (15-minute steps inside a day the carer can already
see) and dose size (1–10). Both are a handful of taps, which is what stepping
is good at. A four-digit year is what it is worst at.

### A1.4 — the demo clock was unusable, and the arithmetic says so

`MEDSIGHT_FAST_DAY_SECONDS` was 240 (a day in four minutes). That makes one
simulated minute **1/6 of a real second**, so a 30-simulated-minute dose
window lasts **five real seconds** — and a dispense takes fifteen to
twenty-five seconds of tapping through instruct, preview, capture, dispense
and confirm. **It was physically impossible to complete a dose inside its own
window.** Round 4 could not have passed as shipped.

Now 1440 s, which makes **one simulated minute exactly one real second**: a
window lasts 30 real seconds, setting a dose two minutes ahead is a two-second
wait, and a missed dose costs half a minute to demonstrate. The constraint is
simply that *the window has to be longer than the flow it is a window for*.

It is also the easiest ratio to explain to whoever is holding the camera:
minutes on screen are seconds in the room. Nobody ever waits a whole simulated
day — you wait until the next dose — so the day length only ever matters
through this ratio.

`MEDSIGHT_FAST_CLOCK=1` is now set in the **Debug** configuration in
`.cproject`, and not in Release. Release is the honest wall-clock build.

### A1.5 — timestamps, requested after round 3

The audit trail had no time on it, because until this session the device had
no clock. Every `SD_Log_Event()` line — on the card and on the UART — is now
prefixed `YYYY-MM-DD HH:MM:SS `.

**Conditional, and the condition is the point.** The stamp appears if and only
if a carer has set the clock. Before that the RTC is counting from its
power-on default and a line stamped `2000-01-01 00:04` would be *worse* than
an unstamped one, because it looks like data.
`time_source_format_stamp()` returns an empty string in that case, so the
concatenation costs nothing and the two eras are distinguishable at a glance
in the file.

ISO order rather than the human "10 Sep 2026" the UI shows, because this one
is read in a text file where sorting and grepping matter more than reading
aloud. In demo builds the time of day comes from the compressed axis, so the
log agrees with the screen rather than with the calendar.

**And it surfaced a real concurrency problem.** Reading the RTC calendar is a
*pair* of HAL calls — `HAL_RTC_GetTime()` locks the shadow registers and
`HAL_RTC_GetDate()` unlocks them — and two tasks interleaving that pair can
hand one of them the other's date. That was theoretical while the RTC had one
reader. It is not any more: the **logger task (priority 2)** now stamps every
line while the **UI task (priority 4)** reads the clock to draw and to
schedule, and the UI preempts the logger by construction.

The pair is now serialised with an OSAL mutex — and that is worth recording
for the submission, because **it is the first genuine consumer of the OSAL's
mutex primitive.** It has been in `ms_osal.h` since Session 07 and
`session_12_notes.md` records that nothing in the codebase used it. Four
primitives were defined in Session 07; the fourth finally has something to
protect.

### A1.6 — one small audit-trail correction

The round 2 log read as four `SECURITY: wrong carer passcode` lines followed
by a lockout, because the fifth wrong entry returned `LOCKED_OUT` and skipped
the `REJECTED` path that does the logging. Five wrong codes is what happened
and five is what the log should say. Fixed.

### Build after all of the above

Both configurations clean, warning counts unchanged (2 Debug / 4 Release, all
pre-existing ST code).

| | text | data | bss | ROM used | RAM used |
|---|---|---|---|---|---|
| Debug (demo clock) | 936,408 | 4,040 | 663,888 | 47.6% | 77.6% |
| Release (wall clock) | 780,544 | 4,036 | 663,872 | 34.2% | 76.2% |

---

## Addendum 1b — the second hardware round (Round 4 passed)

The full scheduled-dose flow ran end to end in demo mode:

```
schedule: dose window OPEN - 04:00 for 'DOUSIK'
...
CONFIRMED: DOUSIK took the 04:00 dose (on time)
...
schedule: dose window CLOSED - MISSED (DOUSIK, 06:15).
MISSED: DOUSIK did not take the 06:15 dose
```

Both edges of a window; the on-time confirmation distinguished from the
generic one; and the missed-dose line this device has never been able to
write. Timestamps appear on every line from the moment the clock was set.
Idle held at 88.8% throughout. **Part B3 is done.**

Four things came back with it.

### 1b.1 — the crying mascot drew over the home screen's buttons

A missed dose selects `MASCOT_ERROR` while the HOME screen is up, and the
crying frames were drawn at `MASCOT_SAD_*` (56,112) — the box the
*two-choice* screen reserves for them. On the home screen that rectangle
sits on top of REGISTER and DISPENSE, so the mascot was painted over both,
half-covering them.

The renderer had no way to know which screen it was drawing onto, because the
position was a `#define` rather than a parameter. `anime_ui_set_error_box()`
makes it one, the sprite is centred in whatever box a screen nominates, and
every screen that selects `MASCOT_ERROR` now says where it wants it.

The general lesson is one this project keeps meeting: **a constant that
encodes a position is a constant that assumes a caller.** It was correct for
the one screen it was written for and silently wrong for the second.

### 1b.2 — "what is the point of setting the clock if it doesn't take it"

Fair, and it was a design flaw in demo mode rather than a bug in the setting.
`time_source_minute_of_day()` derived the compressed day from
`HAL_GetTick() % period` — a free-running axis anchored to boot, which
**ignored whatever a carer typed**. Setting 04:00 did not make the clock read
04:00.

It is anchored now: `time_source_set()` records both the minute-of-day chosen
and the tick at that instant, and the compressed day runs forward from there.
The demo clock reads what the carer set, and *then* runs fast — which is what
the mode was always supposed to do. On boot it re-anchors from the RTC, so a
reboot does not silently move it either.

`demo_second_of_day()` is now the single source for the demo axis, so the
minute the scheduler acts on, the time on screen, and the seconds in the log
stamp cannot disagree.

### 1b.3 — the clock is live on every screen that shows one

It was drawn once on screen entry and never again. In demo mode, where a
simulated minute passes every real second, that is a **visibly frozen clock
on a screen whose entire job is scheduling** — worse than no clock at all.

`carer_ui_clock_tick()` runs once per UI pass, redraws only when the
displayed text changes, and flushes only that 24-pixel band: about once a
real second in demo mode, once a minute in a real build. It is armed by
whichever screen draws a strip and retired automatically on any screen
change, so it cannot go stale or be inherited by the next screen.

**The home screen has one now too** — it is where the device spends its life,
and in demo mode it is the only place you can watch the compressed day run.
The demo strip shows seconds as well: at one simulated minute per real
second, a minutes-only clock sits still for a whole second and then jumps,
which reads as a bug rather than as a fast clock.

Guarded on `s_cap_in_flight`: between the capture request and its result the
AI task owns the framebuffer and nothing in `state_machine.c` may draw. The
capture states show no strip, so the guard is belt-and-braces — but the
invariant is worth restating at every site that could break it.

### 1b.4 — dose times went to the keypad too

The 15-minute stepper survived the first round's clock redesign. The second
round's log killed it: **the schedule screen was opened and saved six times
in one session.** Moving from 04:00 to 06:15 is nine taps, and moving to an
arbitrary time is worse.

Tapping a slot now opens the same numeric keypad as the passcode and the
clock — four digits into `HH : MM`, OK commits back to the grid. Two modes on
one screen, so no new `AppState` was needed and `state_machine.c` is
untouched.

**Three screens, one keypad.** That is the point: a carer learns it once.
Stepping survives on exactly one screen — dose size, 1 to 10 — where every
value is at most five taps away and the control shows the whole range
implicitly.

### 1b.5 — the keypad screens repainted the whole display on every tap

Reported after the third round: "every change felt like the whole screen
changing, like a transition which was visible". It was, literally.

Every keypad tap called the screen's full draw function. `gui_draw_frame()`
repaints all 800×480, strokes the rounded border and blits four corner
motifs; then twelve buttons are drawn; then `gui_draw_flush()` cleans the
entire 768 KB framebuffer out of D-cache. **Roughly 380,000 pixel writes to
change two glyphs** — and the LTDC is scanning out of that same buffer
throughout, so the repaint is visible as a wipe.

The passcode prompt never had the problem, because `draw_entry_dots()` had
always repainted only its own band. The three screens that grew out of it did
not inherit that discipline:

| Screen | was | now |
|---|---|---|
| Set clock | full redraw per digit | entry line + status line, `gui_draw_flush_rows(96, 150)` |
| Dose-time entry | full redraw per digit | subtext + `HH:MM` line, rows 100–150 |
| Dose size | full redraw per ± tap | the number and its lentil row, rows 132–291 |

A full redraw is kept for *entering* a screen, where the whole layout
genuinely does change. It is the same principle `carer_ui_clock_tick()`
already used for the live clock: **touch the smallest rectangle that actually
changed.**

Worth noting as a general point about this UI: with a single framebuffer and
no double buffering (Session 13 proved a second one does not fit — see
`MEMORY_MAP.md` §2), *every* full-screen redraw is visible while it happens.
That is tolerable at a screen transition, where the user expects a change,
and it is not tolerable on a keystroke.

### One observation from the log, not a defect

```
Gallery: best similarity 65/100, threshold 65/100 -> MATCH
```

That match succeeded by exactly zero margin. `session_12_notes.md` Addendum 3
already flags the 0.65 threshold as inherited from a reference project that
used 16-bit embeddings where this one uses int8, and as needing real
measurement rather than a blind adjustment. **Nothing was changed** — but it
is worth knowing before the demo video: one point of similarity lower and
that dispense would have gone to the retry screen instead. Enrolling in the
lighting the demo will actually use is the cheap mitigation.

### Build after all of the above

Both configurations clean, warning counts unchanged (2 Debug / 4 Release, all
pre-existing ST code).

---

## Addendum 2 — action recognition: the memory answer was wrong

The project owner's friend has built an action-recognition model, and it is
coming. That changes the calendar argument in §B4/§7 — the model does not have
to be trained — so the memory argument had to be re-checked rather than
repeated. **It does not hold.**

`MEMORY_MAP.md` §6b and `PROGRAM_PLAN_RECONCILIATION.md` §8 both said an
8-frame ring at 128×128×3 is 393 KB against a 220 KB arena, so a temporal
model does not fit. The arithmetic is right and **the premise is wrong**: the
frame ring never had to be in SRAM.

There is **16 MB of NPU-reachable external PSRAM at `0x90000000`**, declared
`READ_WRITE` in *both* networks' own generated memory-pool tables
(`hyperRAM`, `size=16777208`), and MobileFaceNet already uses it as
activation scratch. It is in this document's own §1b table under external
memory, and I did not carry it into the budget. That is my error and it
changes the conclusion.

**So the constraint is not memory. It is the camera.**

The DCMIPP currently DMAs into `BUFFER_ADDRESS`, which is the display
framebuffer, and the camera is **stopped** for the entire dispense flow after
the face capture — Session 09 found that resuming it lets the DMA
continuously overwrite whatever the UI has drawn, which is why every screen
from the capture onward is static UI. Watching a patient during the confirm
screen means the camera running *while the UI keeps drawing*, which means
DMAing somewhere that is not the framebuffer — PSRAM — plus re-deriving the
`LPEN` question for that destination from scratch, per §3's standing rule.

That is the piece to solve before anything about the model matters.

**Decided: the model corroborates the "I Took It" button, it does not replace
it.** The button is proven on hardware and stays the path that actually
confirms a dose; the model's verdict becomes evidence in the log
(`CONFIRMED: <name> took the 08:00 dose (on time, gesture confirmed)`), behind
a build switch on the `MEDSIGHT_PHYSICAL_DISPENSER` pattern so it stays
cuttable. A model failure must never be able to mean a dose that cannot be
confirmed at all.

It runs on the **existing** `ai` task at priority 3 with the same event-flag
handshake — not a second AI task, because the frame-buffer ownership argument
in `SOFTWARE_ARCHITECTURE.md` §9 depends on there being one owner of
`BUFFER_ADDRESS` at a time.

Both documents above are corrected.

---

## Addendum 3 — the buzzer is Session 14's, and it is for the carer

Confirmed with the project owner after round 3. Recording the design here
because Session 15 built the event it fires on.

**It sounds on the MISSED edge, not at the patient.** A dose window closing
unserved is the one event in this device that genuinely needs sound: it is the
only thing that happens when nobody is looking at the screen. The buzzer
brings a **carer** to the device, who opens carer mode → DOSE HISTORY and sees
who missed and when. Beeping at a patient who has already not responded is
nagging, not helping.

**The firmware hook already exists.** `schedule_service()`'s
`SCHED_FLAG_CLOSE` branch in `state_machine.c` fires exactly once per missed
window and is where the `MISSED:` line is written. Session 14 adds one call
there. The carer-facing half — the filtered, red-highlighted DOSE HISTORY
screen — is already built and tested.

**This closes a Program Plan gap.** `PROGRAM_PLAN_RECONCILIATION.md` §3
currently records "audio and/or visual alert feedback" as promised-and-dropped,
visual only. Session 14 should revise that section, and the three documents
that state "there is no buzzer and no audio in this project" — the README,
`MASTER_PROJECT_PLAN.md` §7 and `SOFTWARE_ARCHITECTURE.md` §7 — the same way
it revises the "no motors, ever" statements.

---

## Session 15 is closed

Every Definition-of-Done item is met and verified on hardware across three
rounds. The last outstanding one — a real-clock run from the **Release**
build — was run and passed, **including a power cycle**: the RTC kept time
across it and the schedule re-armed from the restored clock. That is the
backup domain doing precisely what `schedule_time_source.c` was designed
around, and it is the strongest single confirmation that B1 works.

One test remains unexercised and is not claimed: the v2 → v3 `patients.dat`
rejection (no v2 card survives to feed it).

**What follows Session 15 changed after it closed**, and so did the
numbering — see `MASTER_PROJECT_PLAN.md` v13:

- **Session 16 is action recognition.** A collaborator delivered a trained
  model (`tools/action_recogntion/`), which removes the cost that actually
  blocked it. **Addendum 2 above is superseded on its conclusion**: it said
  the decision turned on time rather than memory, and that it was "a maybe,
  so no". A delivered model makes it a certainty. The corrected *memory*
  analysis in that addendum still stands, and so does the corroborate-not-
  replace decision.

  Worth adding, because Addendum 2 could not have known it: what the
  collaborator built is **not** the temporal CNN this project always assumed.
  It is a YOLOv8n single-class pill detector, geometric features, and a
  rule-based state machine — only the detector needs the NPU. And mouth
  tracking comes free from CenterFace landmarks this firmware has emitted
  since Session 08B and never read (`FD_OUT_LANDMARKS`, a 32×32×10 tensor:
  five landmarks including both mouth corners). `prompts/session_16.md`
  Part 0 has the full analysis.

- **Session 14 is a retired number.** The physical-dispensing prompt became
  **Session 17**; the owner is doing hardware interfacing last. The section
  below was written as "what Session 14 will collide with" and applies
  unchanged to Session 17 — plus everything Session 16 adds, if it runs
  first, which it is intended to.

---

## What Session 17 will collide with, if it runs after this

Listed so the later session reconciles rather than reverts, as both prompts
require.

- **`state_machine.c`** — ten new states, a schedule engine, and a
  `schedule_service()` call at the top of `state_machine_update()`.
  `STATE_DISPENSING` is untouched by this session and is where Session 17's
  work goes.
- **`main.c`** — two new calls before `osal_scheduler_start()`
  (`time_source_init()`, `state_machine_service_init()`). The task table is
  unchanged: five tasks, and `OSAL_MAX_TASKS` is 8, so a dispenser task fits.
- **The linker script** — `ROM` moved to `0x34100000` and grew to 1 MB, and
  there is a new `AI_ARENA` region at `0x34388000`. **A Session 17 DMA buffer
  must not be placed in `AI_ARENA`**, and if one goes anywhere outside
  AXISRAM3–6 it needs an `LPEN` bit in `ms_configure_sleep_clocks()` in the
  same change, tested from a cold boot.
- **`ms_osal.c`** — `OSAL_MAX_ALARMS` is 2 and one is used;
  `OSAL_MAX_FLAGS` is 4 and two are used. A dispenser handshake has room.
- **`PatientRecord` is format v3.** If Session 17 adds a `hopper_id`, it
  becomes v4 and carers re-register again — worth doing in one go if both are
  wanted.
- **No GPIO was assigned by this session.** The pin map in
  `SOFTWARE_ARCHITECTURE.md` §8 is untouched, so Session 17 has a free hand.
