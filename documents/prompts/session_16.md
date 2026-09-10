# Session 16 — Action Recognition: Did They Actually Take It?

## What this session is for

Session 10 replaced action recognition with a manual "✓ I Took It" button.
That button is honest, it works, and it has been on hardware since Session 10
— but it confirms that *somebody tapped a button*, not that a pill went in a
mouth. This session adds the second thing, as **corroboration**, not as a
replacement.

The reason it is possible now and was not before: **a collaborator has
built and trained the vision half**, and it is in
`tools/action_recogntion/`. That removes the training cost — which
`PROGRAM_PLAN_RECONCILIATION.md` §8 identified as the real blocker, not
memory.

---

## Session ordering — read this before anything else

**This session follows Session 15.** Session 15 is complete and
hardware-verified (`milestones/session_15_notes.md`).

1. `ls sessions/` and `ls documents/milestones/`. Base on the
   highest-numbered `sessions/session_NN` with a matching
   `session_NN_notes.md` recording a completed hardware-verified run.
   That should be `sessions/session_15`.
2. Copy it to `sessions/session_16/` and say which base you chose and why.
3. **Session 17 (physical dispensing hardware + buzzer) has NOT run**, and is
   deliberately last. It was written as Session 14; the owner moved hardware
   interfacing to the end after Session 15. If for any reason it *has* run,
   its dispenser task, EXTI ISR, GPIO assignments and DMA buffers are your
   baseline and none of it may be regressed — in particular Part E below must
   not take memory or a DMA channel it is using.
4. Record the base at the top of `session_16_notes.md`.

---

## How to start

1. **READ `tools/action_recogntion/summary.md` IN FULL.** It is the
   collaborator's own architecture document and it is good. It is also
   honest about which parts are not verified for STM32 — believe those
   flags, and Part 0 below resolves them.
2. **READ `documents/MEMORY_MAP.md` §3, §5 and §6b.** §5 is a runbook for
   adding exactly this. §6b is the corrected verdict on whether it fits.
3. **READ `documents/AI_LESSONS.md` IN FULL.** Every failure it records will
   be waiting for you: the NPU bus fault from a bad `.rodata` placement, the
   IDE lock-up under the external loader, weights needed at three separate
   flash addresses, and the flashing commands.
4. **READ `milestones/session_12_notes.md` Addendum 9** before you touch a
   DMA destination. It is the six-round `LPEN` story and Part E is the first
   thing since to add a DMA master.
5. **READ `milestones/session_15_notes.md` Addendum 2** — the corrected
   memory analysis and the decision to corroborate rather than replace.

---

## Part 0 — What the collaborator built, and what of it can run here

**This analysis is already done; verify it, do not redo it.** It was
performed at the end of Session 15 and it is the reason this prompt is shaped
the way it is.

The delivered system is a **three-stage pipeline**, not a single temporal
model, and that distinction is the whole opportunity:

| Stage | What it is | Runs on the N6? |
|---|---|---|
| **1A — pill detector** | YOLOv8n, transfer-learned on a single `pill` class. `models/pill_detector/best.onnx`, 12 MB FP32, 320×320, static shape, opset 12, simplified. Input `images`, output `output0`. | **Yes, after INT8 quantisation.** This is the one new NPU model. |
| **1B — mouth** | MediaPipe Face Mesh (Python). Their summary flags it 🟡 "needs verification / possible replacement". | **No — and it does not need to.** See below. |
| **2 — geometry** | Distance, velocity, acceleration, overlap, visibility. Pure maths. | **Yes.** Plain C, no NPU. |
| **3 — decision** | Their summary proposes a "Tiny TCN" 🟡. What is *actually implemented* in `main/main.py` is `GuardedIntakeStateMachine` — a **rule-based state machine**. | **Yes, and no NN is needed.** Port the state machine. |

### The finding that makes this session cheap

**MedSight already has mouth landmarks on the NPU, for free.**

The CenterFace detector this project has run since Session 08B emits five
landmarks — two eyes, nose, and **two mouth corners** — on the same 32×32
grid as its heatmap. Confirmed in the generated header:

```
STAI_FD_OUT_2_CHANNEL (10)      /* 5 landmarks x (x,y)          */
STAI_FD_OUT_2_HEIGHT  (32)      /* same grid as the heatmap     */
STAI_FD_OUT_2_WIDTH   (32)
```

`ai_vision.c` already `#define`s `FD_OUT_LANDMARKS 1` and **has never read
that tensor**. So Stage 1B needs no MediaPipe, no second new model, and no
extra inference: it is a decode of an output the existing 209 ms detector
already produces.

**The limitation this creates, and you must not paper over it.** Two mouth
corners give a mouth *centre* and *width*. They do **not** give an upper and
lower lip, so **mouth-open detection is not available**. The collaborator's
state machine uses `mouth_data["is_open"]` in two transitions. You must
either drop those conditions or find a proxy, and whichever you choose, say
so plainly in the notes and in the accuracy claim. Do not fake an
`is_open` signal.

### What to verify rather than assume

- That `fd_outputs[FD_OUT_LANDMARKS]` decodes to sane pixel coordinates for a
  real face. Print them and look at them before building anything on top.
- The landmark ordering. CenterFace's conventional order is
  (eye_l, eye_r, nose, mouth_l, mouth_r) but **check it against a real
  capture** rather than against this sentence.

---

## Part A — the pill detector onto the NPU

The single riskiest part of the session. `AI_LESSONS.md` and
`MEMORY_MAP.md` §5 are the runbook; follow them rather than improvising.

1. **Quantise to INT8.** The supplied `best.onnx` is FP32 and will not map
   usefully onto the Neural-ART. You need a representative calibration set —
   ask the collaborator for a sample of the training images; do not calibrate
   on synthetic data.
2. **Consider truncating the head.** YOLOv8's DFL decode and NMS are
   awkward on an NPU and are conventionally cut and run on the CPU. The
   Cortex-M55 has cycles to spare here (it is idle ~88% of the time). Decide
   deliberately and record why.
3. **Measure the activation working set** and place it. `AI_ARENA` is
   **225,280 bytes at `0x34388000`**, proven writable on hardware in Session
   15. **320×320 YOLOv8n INT8 may not fit in 220 KB** — this is the
   measurement the session turns on. If it does not:
   - drop the input to 256×256 or 192×192 and re-measure (a pill held near a
     mouth is a large object in frame; the collaborator chose 320 for a
     webcam, not for this camera), **or**
   - put activations in PSRAM at `0x90000000` (16 MB, NPU-reachable,
     `THROUGHPUT=MID LATENCY=HIGH`) and accept the latency cost.
   Record the measured number either way. **A negative result is a result.**
4. **Weights go to external OSPI NOR**, tagged `.xspi2`, flashed separately.
   ~3.2 MB INT8 against 64 MB mapped and ~290 KB used — not a constraint.
5. **Re-check the `.xspi2` layout** against the flashed image afterwards
   (`arm-none-eabi-nm -n <elf> | grep '^71' | head`). Session 12 lost a round
   to Debug and Release emitting blobs in opposite order. Keep
   `-fno-toplevel-reorder` on Release.
6. **Measure the added latency.** The face pipeline is 209 ms and invariant.
   Report the pill detector's number the same way.

---

## Part B — the camera path (do this BEFORE Part A if you can)

**This is the real engineering problem of the session, and it is not about
the model.** Read this before committing to a schedule.

Today the DCMIPP DMAs into `BUFFER_ADDRESS` — which *is* the display
framebuffer — and the camera is **stopped** for the entire dispense flow
after the face capture. Session 09 found that resuming it lets the DMA
continuously overwrite whatever the UI has drawn, which is why every screen
from the capture onward is static UI rather than a preview.

Action recognition needs the camera running **while the UI keeps drawing**,
for the whole `STATE_CONFIRM_TAKEN` window. So:

1. **Point the DCMIPP somewhere that is not the framebuffer.** PSRAM at
   `0x90000000` is the obvious destination and is where a frame ring wants to
   live anyway.
2. **Re-derive the `LPEN` question for that destination from scratch.** A DMA
   master writing into a bank whose clock stops on `WFI` is precisely the
   fault that cost six rounds of debugging on the display
   (`session_12_notes.md` Addendum 9). XSPI1 is **not** in
   `ms_configure_sleep_clocks()`'s set today. Add what is needed, in the same
   change, and **test from a cold boot**.
3. **Decide what happens to `task_camera_isp`**, which currently assumes one
   pipe into one buffer.
4. **Keep the frame-buffer ownership invariant** in
   `SOFTWARE_ARCHITECTURE.md` §9 intact: exactly one owner of
   `BUFFER_ADDRESS` at a time.

If this cannot be made to work cleanly, **stop and say so.** A working
Session 15 build with an honest "we could not get the camera and the display
to coexist" is a far better outcome than a device that glitches during the
one screen a judge will be watching.

---

## Part C — Stage 2 and Stage 3 in C

Both are plain C and neither needs the NPU.

**Stage 2** (`FSBL/Src/ai/intake_features.c`, suggested): normalised pill
centre, mouth centre, distance, distance velocity, acceleration, overlap with
the mouth region, visibility flags. The collaborator's `main/main.py` and
`hand_pill_tracker.py` are the reference; keep the same feature names so the
two can be compared.

**Stage 3** (`FSBL/Src/ai/intake_fsm.c`, suggested): port
`GuardedIntakeStateMachine` from `main/main.py`. It is already a state
machine with named states —
`SEARCHING → LOCKED → APPROACHING → AT_MOUTH → RETREATING_CHECK →
CONSUMED / NOT_CONSUMED_* / UNCERTAIN` — plus drop detection and a recovery
path. That maps to C directly.

Two things to handle deliberately:

- **The `is_open` conditions** have no signal behind them (Part 0). Decide,
  record, move on.
- **`UNCERTAIN` is a first-class outcome** and must stay one. A system that
  only ever says CONSUMED or NOT CONSUMED will be wrong confidently, which is
  the worst behaviour for this device.

---

## Part D — corroborate the button, never replace it

**Decided by the project owner; do not reopen.**

- The **"✓ I Took It" button remains the confirming action.** It is proven,
  it works, and it is the path a patient understands.
- The model's verdict is **evidence in the log**, not a gate:
  `CONFIRMED: <name> took the 08:00 dose (on time, gesture confirmed)` or
  `... (on time, gesture NOT observed)`.
- A model failure must **never** be able to mean a dose that cannot be
  confirmed at all.
- Behind a build switch — `MEDSIGHT_ACTION_RECOGNITION`, on the
  `MEDSIGHT_PHYSICAL_DISPENSER` pattern — so it stays cuttable near the
  deadline. The Session 15 behaviour must remain intact with it off.
- It runs on the **existing** `ai` task at priority 3, with the same
  event-flag handshake. **Do not create a second AI task** — the
  frame-buffer ownership argument in `SOFTWARE_ARCHITECTURE.md` §9 depends on
  there being one owner.

---

## Part E — what this does and does not do for the Program Plan

`PROGRAM_PLAN_RECONCILIATION.md` §1 records pill classification — a named
core function — as never built and substituted with face recognition. Be
precise about what this session changes:

- **It does not close that gap.** A single-class `pill` detector finds *a*
  pill. It does not identify *which* medication, which is what the plan
  promised.
- **It does narrow it honestly.** The NPU now sees the pill, and the device
  can say a pill physically went to the patient's mouth.
- Update §1 and §8 to say exactly that, and no more. The reconciliation's
  value is that it does not overclaim; do not spend that.

---

## Definition of Done

- [ ] Part 0's two verifications done on real hardware: the CenterFace
      landmark tensor decodes to sane coordinates, and the landmark ordering
      is confirmed rather than assumed.
- [ ] The pill detector runs on the NPU, with the **measured** activation
      footprint and added latency recorded — or a documented, numbers-backed
      statement of why it does not fit, which is an acceptable outcome.
- [ ] The camera writes somewhere other than the display framebuffer, with
      the UI drawing at the same time, verified **from a cold boot**, and any
      new `LPEN` bit added in the same change.
- [ ] Stage 2 and Stage 3 in C, with `UNCERTAIN` preserved as an outcome and
      the missing `is_open` signal recorded rather than faked.
- [ ] The "I Took It" button still confirms the dose. With
      `MEDSIGHT_ACTION_RECOGNITION` off, the build behaves exactly as
      Session 15 did.
- [ ] An honest accuracy claim from a real run — how many intakes out of how
      many attempts, and what the failure modes looked like. **Do not quote
      the collaborator's webcam numbers as this device's numbers.**
- [ ] Build 100% clean, both configurations; `.xspi2` layout re-verified.
- [ ] `documents/milestones/session_16_notes.md` written, with the base
      folder at the top.
- [ ] `PROGRAM_PLAN_RECONCILIATION.md`, `AI_PIPELINE.md`,
      `SOFTWARE_ARCHITECTURE.md`, `MEMORY_MAP.md` and the README updated —
      including `AI_PIPELINE.md`'s "one model, one NPU" title, which stops
      being true.

---

## What this session does NOT do

- **No networking.** Permanent.
- **Not a replacement for the button.** Part D.
- **No mouth-open detection** unless a real signal is found for it.
- **No hardware interfacing** — that is Session 17, deliberately last.
- **No overclaiming.** Especially not about pill classification (Part E), and
  not about accuracy measured on somebody else's webcam.
- **No third NPU model beyond the pill detector.** Stage 3 is a state
  machine; if you find yourself training a TCN, stop and re-read Part 0.
