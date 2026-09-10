# PROGRAM_PLAN_RECONCILIATION.md — what we promised against what we built

**Written:** Session 15.
**Compares:** `tools/Program Plan 54916.pdf` (submitted 31 March 2026, for the
round-1 proposal) against the firmware in `sessions/session_15/`.
**Method:** every row below was checked against the actual source, not against
this project's own later documents. Where a document and the code disagreed,
the code won and the document is corrected.

## Why this document exists

The Program Plan is the only document in this project the judges have already
read. It is what won the board and the place in round 2. Several things in it
were never built; several things that were built are not in it. A submission
that quietly drops a promised feature reads far worse than one that says "we
replaced X with Y, and here is why Y is better" — so this is the honest
version, in both directions.

Two of the divergences are genuine gaps and are named as such. Most are the
delivered system being **more** than the sketch. One — pill classification —
is a substitution that has to be argued rather than glossed over, and §1 does
that first, because it is the one a judge will look for.

---

## 1. The big one: on-device pill classification was never built

**The Program Plan said** (Core Functions, and again in §6 Feature
Description): *"On-device CNN-based pill/packet visual classification using
the board's NPU."* The device in the plan was camera-only: a user held their
medication up to the camera, a CNN identified the pill or packet, and the
result was checked against a stored schedule.

**What exists instead:** one-shot **face recognition** — CenterFace detector
plus a MobileFaceNet embedder, both INT8 on the Neural-ART NPU — matching the
person in front of the camera against a small gallery of enrolled patients.
It verifies *who is taking the medication*, not *what the medication is*.

**Verified in the code:** `FSBL/Src/ai/fd.c` records
`--onnx-input ".../centerface_OE_3_3_1.onnx"` and `FSBL/Src/ai/faceid.c`
records `.../mobilefacenet_int8_faces_OE_3_3_1.onnx`. There is no third
network anywhere in the tree, and no classification head of any kind.

### Why it changed

The decision was taken early (`MASTER_PROJECT_PLAN.md` §8) under a design
that no longer exists: the project had by then grown a physical multi-hopper
dispenser, and under a hopper design each hopper holds one known medicine by
construction, so classifying the pill mostly re-confirms what the mechanism
already guarantees.

**That argument does not fully carry, and it should not be offered as though
it does.** The Program Plan's device had no hopper at all — the user held the
pill up to the camera, so there was nothing else in the system that knew what
the pill was. Under the plan as submitted, classification was load-bearing.

### Why it is not being built now

Not preference — measured memory. Taken at the end of Session 12 and
re-checked in Session 15 (`documents/MEMORY_MAP.md`):

| Constraint | Measured | Consequence |
|---|---|---|
| Debug-build ROM before Session 15's linker work | 474,080 of 523,264 bytes — **90.6% full, 48 KB free** | The two existing networks' generated forward-pass code is the bulk of a 469 KB `.text`. A third does not fit in 48 KB. |
| NPU-reachable SRAM | AXISRAM3–6 only, 1,835,008 bytes, of which the two networks address **0x34200000–0x34387FFF contiguously (1,605,632 bytes)** | 225,280 bytes remain (the `AI_ARENA` this session claimed and proved). |
| Free NPU-reachable SRAM after Session 15 | **220 KB** | Enough for a small classifier's activations; see §8 for the honest answer on whether that settles it. |

Session 15 has since raised the ROM ceiling substantially — Debug now sits at
47.3% of 1 MB with 539 KB free — so the ROM half of that argument is weaker
than it was. The remaining reasons are the ones that actually decide it, and
they are about calendar and evidence rather than bytes: see §8.

### The honest framing, and it is a strong one

The delivered system verifies the **patient** by face and — once Session 17's
IR break-beam counter lands — physically **counts** the pills that leave the
hopper. Together those prove more about adherence than identifying a pill
that a single-medication hopper already knows the identity of:

- classification answers *"this looks like the right pill"*;
- face + count + schedule answers *"this specific person was given exactly
  three pills, inside their 08:00 window, and here is the log entry"*.

The second is a stronger adherence claim, and it is measured rather than
inferred. What was lost is the ability to catch a hopper loaded with the
wrong medication — a real gap, and the right place to say so is here.

---

## 2. Schedule validation — was a named core function, was never built, is
built now

**The Program Plan said:** *"Schedule validation — compares identified
medicine against the patient's stored plan"*, a dedicated **Validation Task**,
and *"The Camera Task wakes either on a scheduled µT-Kernel alarm (aligned to
dose times) or on user button press."*

**What existed until Session 15:** nothing. No RTC, no time-of-day logic, no
schedule in the patient record. `schedule_time_source.c` had been listed in
`SOFTWARE_ARCHITECTURE.md` §2 as "DESIGNED, NOT YET WRITTEN" since Session 10.
Dispensing was entirely user-initiated.

**What Session 15 built** (Part B):

- `FSBL/Src/schedule_time_source.c` — the RTC behind one swappable interface,
  with a compressed-day demo mode behind the same interface.
- `PatientRecord` grew `dose_time[4]` and `dose_time_count`; `patients.dat`
  went from format v2 to v3.
- A dose window that **opens** (an on-screen reminder naming the patient) and
  **closes** (a `MISSED:` line in the audit log if nobody dispensed), driven
  by a µT-Kernel **alarm handler** — `tk_cre_alm`/`tk_sta_alm`, reached
  through a new `osal_alarm_*` primitive.

That last point is worth spelling out for a µT-Kernel-literate reader,
because it is the *same mechanism the Program Plan named*. The plan's sentence
about the camera task waking on a scheduled alarm went unimplemented for five
months; it is implemented now, in the idiomatic way, with the handler doing
nothing but setting an event flag because it runs in handler context.

**What is still different from the plan:** the plan compared an *identified
medicine* against the plan. This device compares the *identified patient* and
the *time* against the plan. Given §1, that is the only version of schedule
validation this system can perform.

---

## 3. Audio and/or visual alert feedback

**The Program Plan said:** *"Audio and/or visual alert feedback on incorrect
medication, missed dose, or confirmation"*, and in §6, *"The Alert Task
drives a buzzer and LED (green = correct, red = incorrect/missed)."*

**What exists:** visual only. Verified: there is no SAI code anywhere —
`HAL_SAI_MODULE_ENABLED` is still commented out in
`FSBL/Inc/stm32n6xx_hal_conf.h` and no audio path was ever written. The two
board LEDs are used, but as a heartbeat and a fault indicator, not as a
correct/incorrect signal.

**Bearing:** modest, and the "and/or" in the plan's own wording gives honest
cover. The claim was **dropped from every document in Session 12** rather than
half-built, which is the outcome this project prefers over a feature that
exists in prose.

**Update, after Session 15's first hardware round: this gap is scheduled to
close in Session 17, and the design is decided.** Session 15 gave the device
the one event that genuinely needs sound — a dose window closing unserved —
and a buzzer will sound on exactly that edge.

Two things about the design are worth stating, because they are the
difference between satisfying the Program Plan's sentence and satisfying its
intent:

- **It alerts the CARER, not the patient.** A missed window is, by
  definition, the case where nobody responded to the on-screen reminder.
  Beeping harder at that person is nagging. The buzzer brings a carer to the
  device, who opens carer mode → DOSE HISTORY and sees who missed and when —
  a screen that already exists, already filters to missed doses, and already
  shows them in red.
- **The firmware hook already exists.** `schedule_service()`'s
  `SCHED_FLAG_CLOSE` branch fires exactly once per missed window and is where
  the `MISSED:` line is written. Session 17 adds one call there.

Until that lands, the accurate statement remains: alerts are on-screen, the
SAI codec and speaker are available and unused, and this document should be
revised — not quietly, but with the reason — when Session 17 builds it.

---

## 4. Caregiver notification flag in non-volatile memory

**The Program Plan said:** *"Caregiver notification flag stored in
non-volatile memory for later review."*

**What exists: considerably more.** A full append-only event log on the SD
card (`events.log`, since Session 06), recording every state transition, every
dispense with the patient's name and dose, every confirmation, every skip,
every unrecognised face, and — from this session — every missed dose window
and every carer action.

And from Session 15 it is **readable on the device**, in carer mode, filtered
to one patient. A flag would have told a carer that *something* happened; this
tells them what, to whom, and when, without taking the card to a laptop.

**Bearing: favourable.** Say so.

---

## 5. Four tasks, Camera / Inference / Validation / Alert

**The Program Plan said:** four concurrent µT-Kernel tasks communicating via
message queues and semaphores, sketched as Camera → Inference → Validation →
Alert.

**What exists: five tasks, decomposed differently**, with a written
rate-monotonic derivation (`SOFTWARE_ARCHITECTURE.md` §9, evidence in
`session_12_notes.md` Part A3):

| Pri | Task | Period | Maps to the plan's… |
|---|---|---|---|
| 5 | `cam_isp` | 1 ms | Camera Task |
| 4 | `ui` | 10 ms | Validation + Alert, plus everything the plan's sketch had no box for |
| 3 | `ai` | on demand | Inference Task |
| 2 | `logger` | event-driven | (no equivalent in the plan) |
| 1 | `heartbeat` | 500 ms | (no equivalent in the plan) |

**Bearing: favourable, and worth being specific about why.** The plan's four
boxes were a pipeline sketch. The delivered set is derived — shortest period
gets the highest priority — and each number is justified in writing. The AI
sits *below* the UI on purpose, so inference is preemptible and touch stays
alive during a capture; that is a decision the sketch could not have made
because it had not yet met the hardware.

The communication mechanisms also went further than the plan's "message
queues and semaphores": the AI request/response handshake is a µT-Kernel
**event flag** (`tk_wai_flg` with a three-outcome OR-wait), and Session 15's
dose scheduling is a µT-Kernel **alarm handler**. Two idioms were also
evaluated and deliberately *rejected* with their evidence written down
(a fixed-size memory pool, and an event flag for `STATE_CONFIRM_TAKEN`) —
`session_12_notes.md` Part A2 and A4.

---

## 6. Extensibility — the plan named three; two landed

The Program Plan's Extensibility paragraph: *"future work could include
multi-patient support, additional sensor fusion (weight sensors for blister
pack detection), or Bluetooth-based caregiver notifications."*

| Named as future work | Status |
|---|---|
| **Multi-patient support** | **Delivered.** A 10-patient gallery with face-based identification, per-patient dose size and, from Session 15, per-patient schedule. A stated *future* enhancement shipped inside the project. |
| **Sensor fusion (weight sensors)** | Not built. Session 17 builds a different sensor for a related purpose — an IR break-beam that counts pills as they physically drop — which is the same idea (measure the physical event rather than assume it) applied to dispensing rather than to blister-pack detection. |
| **Bluetooth caregiver notifications** | **Not built, and deliberately never will be.** Zero-network is a documented design principle of this project, not an omission: there is no code path capable of transmitting anything off-device, which is a stronger guarantee than a policy. `COMPLIANCE_PRIVACY_POSTURE.md` §2. Verified by grep: no Wi-Fi, BLE, lwIP, Ethernet or socket code exists anywhere in the firmware. |

---

## 7. The remaining rows

| Program Plan said | Reality | Bearing |
|---|---|---|
| **Development scope ~3,300 lines** across six modules | **~11,800 lines** of hand-written C in 28 project-owned files, excluding all vendored code (HAL, BSP, FatFs, Cube.AI-generated networks, µT-Kernel) and excluding a 10,494-line generated sprite/font asset blob. | Favourable, framed as scope delivered rather than lines typed. The estimate was for a camera-only device with no UI to speak of. |
| **Team of 4**, with named roles: Lead/RTOS, AI/Vision, Firmware/Drivers, UI/Testing | **Solo for Sessions 01–13.** A hardware teammate joined for Session 17. | State the truth. Worth noting that all four of the plan's role descriptions were genuinely covered — RTOS integration, AI/vision, firmware/drivers, UI/testing — by fewer people, over fifteen working sessions with the record of each one kept. |
| **AI Framework: STM32Cube.AI / Renesas RA Smart Configurator + TensorFlow Lite for Microcontrollers** | **STM32Cube.AI / ST Edge AI only.** No TFLM, no Renesas toolchain — the board preference resolved to the STM32N6570-DK, so the RA-side alternatives never applied. | Neutral. The plan listed both board options' toolchains because the board had not been allocated yet. |
| **Board preference: STM32N6570-DK / EK-RA8P1** | STM32N6570-DK throughout. | Neutral. |
| **"Open Source Commitment:** full source code, model training scripts, and documentation released openly upon submission" | Source and documentation: delivered, including fifteen session prompts and fifteen milestone notes. **Model training scripts: not applicable** — no model was trained for this project. Both networks are pretrained, publicly available ONNX models converted with ST Edge AI; their provenance and licensing are in `THIRD_PARTY_SOFTWARE.md` §2.7. | Say it precisely rather than letting "training scripts" stand unqualified. There is nothing to withhold; there is nothing to release. |
| **Physical dispensing** | **Not in the Program Plan at all** — the plan's device was camera-only verification. Session 17 builds a real single-hopper stepper turntable with an IR pill counter. | Favourable, and worth being explicit: this is scope *added* beyond what was promised, not scope restored. |
| **"Privacy-First, Offline Design"** | Delivered exactly as described, and gone further: minimal collection (four fields, now five with the schedule), embeddings rather than photographs, and — Session 15 — a password-gated delete so a patient's face embedding can actually be removed. | Favourable. `COMPLIANCE_PRIVACY_POSTURE.md` is written strictly against what the code does, including the limitations. |
| **"Real-time camera capture triggered by a scheduled RTOS task or user button press"** | Both, as of Session 15. Button press since Session 05; the scheduled half is §2. | Closed this session. |

---

## 8. So does a third model fit? The answer with numbers

Session 15's Part B4 was asked to settle this rather than leave it as a
question. The memory answer and the answer are different things.

**Memory (measured, `MEMORY_MAP.md`):**

- ROM: 539 KB free in Debug, 675 KB in Release. A small INT8 classifier's
  generated forward-pass code would fit comfortably. **This is no longer the
  binding constraint** — Session 15's linker work removed it.
- NPU-reachable activation space: **220 KB**, at `0x34388000`, claimed as
  `AI_ARENA`, pattern-tested on hardware. A 96×96×3 INT8 classifier of the
  MobileNet-v1-0.25 class (there is one sitting unused in `tools/`) needs
  activations well inside that.
- Weights: external OSPI NOR at `0x71000000`, 64 MB mapped, of which about
  290 KB is used. Not a constraint at all.

**So on memory alone, a small pill classifier fits.** That is a change from
the answer this project has been giving since Session 12, and it is only true
because B4 went and looked.

**Action recognition — corrected after the first hardware round.** This
paragraph originally said it does not fit, on the grounds that an 8-frame ring
at 128×128×3 is 393 KB against a 220 KB arena. **The premise was wrong**: the
ring never had to be in SRAM. There is 16 MB of NPU-reachable external PSRAM
at `0x90000000`, declared `READ_WRITE` in both generated networks' own
memory-pool tables and already used as MobileFaceNet's activation scratch.
Memory does not rule it out.

A model has since been built by a collaborator, so training cost is not the
constraint either. What remains is the **camera path**: the DCMIPP DMAs into
the display framebuffer and the camera is stopped for the whole dispense flow,
so watching a patient during the confirm screen needs the camera writing
elsewhere while the UI keeps drawing. That is a firmware question, not a model
one. `MEMORY_MAP.md` §6b has the analysis.

**What decides it is time, not memory.** The remaining calendar to the
30 September deadline is measured in days, and integrating a third network on
this board is not a small job with a known cost — the evidence is this
project's own history:

- Session 08A spent an entire session proving the toolchain path with a
  *trivial* model, and hit an NPU bus fault and an IDE lock-up doing it.
- Session 08B needed the weights flashed to **three separate external
  addresses** across two rounds of debugging before the face pipeline worked,
  with the failure presenting each time as one unhelpful assertion.
- Session 12 found that Debug and Release laid the same weights out in
  *opposite order*, so only one configuration could ever run against one
  flashed image.

None of that is a memory problem, and all of it would recur. The owner's
standing instruction is that a second model happens only if it is a certainty,
not a maybe. **It is a maybe. So: no.**

The result is recorded here so the next person inherits a foundation rather
than a question: the arena exists, it is proven, its address and size are
documented, and `MEMORY_MAP.md` has a runbook for dropping a third model into
it. What is missing is time, and that is the one thing this document cannot
create.

---

## 9. Summary for the submission

| | Count |
|---|---|
| Promised and delivered as described | 5 (offline operation, NPU inference, non-volatile caregiver record, multi-patient, RTOS task architecture) |
| Promised and delivered **better than described** | 4 (event log vs. a flag; five derived tasks vs. four sketched; on-device log review; multi-patient shipped rather than deferred) |
| Promised and **substituted**, with the reasoning stated | 1 (pill classification → face recognition — §1) |
| Promised, **never built, and built in Session 15** | 1 (schedule validation — §2) |
| Promised, dropped in Session 12, and **scheduled for Session 17** with the design decided | 1 (audio alerts — §3; a carer alert on the missed-dose edge) |
| **Not promised** and delivered anyway | physical dispensing (Session 17), carer mode, gated enrolment, password-gated delete, the µT-Kernel migration itself |

The one thing this reconciliation cannot make favourable is §1, and the right
response to a judge asking about it is the one in §1: we measured, we chose,
and what we built proves more about adherence than what we promised would
have.
