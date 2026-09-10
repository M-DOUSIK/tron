# AI_PIPELINE.md — MedSight

## 1. Two Model Families, One NPU

**Corrected in Session 16.** This section was headed "One Model, One NPU" from
Session 08B until Session 16, and it is no longer true: a third network — a
single-class **pill detector** — now runs on the same Neural-ART accelerator,
on the same task, behind the same event-flag handshake. §6 covers it. The
face-recognition pipeline below is unchanged.

**One-shot face recognition + small-gallery matching** — verifies which enrolled
patient (of potentially several sharing the device) is present, before dispensing
or logging anything. "One-shot" means enrollment from a single reference photo per
patient (Session 09's registration flow), not a large training set per person.
Matching is against a gallery of all currently enrolled patients, not a single
hardcoded reference face — this is a shared, multi-person device (see
`MASTER_PROJECT_PLAN.md` §1). Implemented Session 08B (**CenterFace** detector +
**MobileFaceNet** embedder), confirmed working end-to-end on real hardware.

**Model names, settled in Session 12.** This section previously said "CenterFace
detector + FaceID embedder" and an earlier draft of `prompts/session_13.md` said
"SCRFD + MobileFaceNet"; the two disagreed, so Session 12's third-party-software
inventory checked the generated sources themselves. `FSBL/Src/ai/fd.c` records
`--onnx-input = ".../centerface_OE_3_3_1.onnx"` and `FSBL/Src/ai/faceid.c`
records `--onnx-input = ".../mobilefacenet_int8_faces_OE_3_3_1.onnx"`. So the
detector is **CenterFace** (session_13.md's "SCRFD" was wrong) and the embedder
is **MobileFaceNet** — "FaceID" is ST's wrapper/module name (`stai_faceid`), not
the architecture. Both documents are now corrected. Full provenance and
licensing in `THIRD_PARTY_SOFTWARE.md` §2.7.

**This was the only model in the pipeline until Session 16.** Two others were
considered and both cut at the time (full history in `MASTER_PROJECT_PLAN.md` §8);
one of them has since been built:
- **Pill/packet classification** — dropped early: under the physical multi-hopper
  design that existed at the time, each hopper would have dispensed a single, known
  medicine by construction, so classifying the dispensed pill's type mostly
  re-confirmed what the mechanism already guaranteed, at real NPU/development cost.
- **Action/consumption recognition** (a lightweight model detecting the hand-to-mouth
  pill-taking gesture, proposed to replace pill classification) — planned as
  Session 08C, never built then, and **built in Session 16**. Once the physical
  dispensing hardware was cut, consumption confirmation became a simple UI problem —
  a manual "✓ I Took It" button (Session 10). It came back because the thing that
  actually blocked it was the cost of building a model, and a collaborator supplied
  the design. `session_08C.md` does not exist; `session_16.md` does. See §6.

The face-recognition model runs as an INT8 model on the Neural-ART Accelerator via
STM32Cube.AI-generated C code, not on the Cortex-M55 CPU — this is a hard constraint
(see `SOFTWARE_ARCHITECTURE.md` module rules): CPU cycles stay free for UI/camera
pipeline work.

## 2. Toolchain

- **STM32Cube.AI** (X-CUBE-AI) — converts a trained/quantized model into C code mapped
  to the Neural-ART Accelerator.
- **ST Model Zoo** (`stm32ai-modelzoo-services` on GitHub) — good source of pretrained,
  already-STM32-optimized starting models; check here before training from scratch.
- Reference implementations worth reviewing for the face-recognition approach (not
  necessarily reused verbatim — check licenses before importing code):
  one-shot/few-shot face recognition projects targeting STM32N6-class NPUs.

## 3. De-Risking Plan (per risk register in MASTER_PROJECT_PLAN.md) — as actually executed

Sequence across Sessions 08A-B (08C, action recognition, was planned but never run —
see §1):
1. **Session 08A:** Get *any* trivial pretrained INT8 model (e.g. a stock
   classification model from the Model Zoo) running through STM32Cube.AI and
   executing on the NPU, with output visible over UART. This proves the
   toolchain/runtime path works before you add model-specific risk.
2. **Session 08B:** Swap in the real one-shot face recognition model, wired to a
   multi-entry gallery-matching function — verify correct identification and correct
   rejection of unenrolled faces against a placeholder test gallery. Confirmed working
   end-to-end on real hardware.
3. **Session 09:** Wire real enrollment (replacing 08B's placeholder test gallery with
   real registered-patient embeddings from the SD card). Also confirmed working
   end-to-end on real hardware.
4. **Session 10:** Wire the face-recognition model's output into the actual (now
   simulated) dispense flow and mascot UI states, alongside the manual "I Took It"
   confirmation button that replaced action recognition.

## 4. Data Handling

Face enrollment images/embeddings, and now also the patient's name, dose size and
(from Session 15) their dose schedule — see `SOFTWARE_ARCHITECTURE.md` §6 for the
record as actually implemented; an earlier draft of the data model also planned a
phone number field for a future notification feature, but that was never built —
are sensitive personal data — see `COMPLIANCE_PRIVACY_POSTURE.md`. All of it is
written only to the local SD card, never transmitted, and never leaves the device.
None of it is ever logged over the UART debug channel, in any session.

## 5. Performance Budget

Target: face-recognition inference (the only model in the pipeline — see §1) fast
enough not to visibly stall the mascot animation or camera preview (Session 04
already establishes non-blocking rendering via DMA2D — inference must not violate
that). Profile actual NPU inference time in Session 08A/08B and record it here once
measured; don't guess a number in advance.

**Measured, Session 13, on hardware (DEBUG build, -O0):**

| Path | Wall-clock | Notes |
|---|---|---|
| Successful capture (face found, embedded) | **209 ms** | Identical to the millisecond across four separate captures - registration and dispense, three different faces/poses (detector confidence 0.76, 0.78, 0.83, 0.89) |
| Failed capture (no face, 3 attempts) | **1111 ms** | 3 x detector + the two 500 ms inter-attempt waits; the embedder never runs |

209 ms is the figure to quote. It is measured from
`ai_vision_capture_request()` to the result being ready, so it includes the
CenterFace detector pass, the MobileFaceNet embedder pass, and the
event-flag IPC and task scheduling between the UI task and the AI task - not
just the two NPU calls in isolation. The real inference time is therefore
somewhat under 209 ms.

Two things worth noting about that number:

- **It was taken from a `-O0` DEBUG build.** The Release build is untested
  for latency; it will not be slower.
- **The invariance is the interesting part.** Four captures, four different
  images, four different confidences, and the same 209 ms every time. The
  Neural-ART runtime executes a fixed epoch schedule for a fixed input
  shape, so the cost does not depend on image content - which is exactly
  what makes it safe to hold the UI still for the capture window rather
  than showing an indeterminate spinner.

Against the target: 209 ms is well inside "does not visibly stall the UI",
and the measured idle figure over the same runs was 86.7-88.9% (the
`power: idle N% of last 10s` lines), i.e. the NPU work is not displacing
the rest of the system.

---

## 6. The Second Model Family: Action Recognition (Session 16)

A three-stage pipeline that corroborates the "✓ I Took It" button. It never
replaces it and it cannot gate a dose — see `prompts/session_16.md` Part D and
`milestones/session_15_notes.md` Addendum 2 for the decision. Behind
`MEDSIGHT_ACTION_RECOGNITION`; with it off the firmware behaves exactly as
Session 15 did.

| Stage | What | Where it runs |
|---|---|---|
| 1A — pill detector | YOLOv8n, single class `pill`, 160×160, INT8, **head cut** | Neural-ART NPU |
| 1B — mouth | Decoded from CenterFace's landmark head | **no extra inference** |
| 2 — geometry | distance, velocity, acceleration, overlap, visibility | Cortex-M55 |
| 3 — decision | a rule-based state machine, not a network | Cortex-M55 |

### Stage 1B costs nothing, and that is the interesting part

CenterFace has emitted five landmarks — two eyes, a nose and **both mouth
corners** — on every capture since Session 08B, on a tensor `ai_vision.c`
defines as `FD_OUT_LANDMARKS` and never read (`STAI_FD_OUT_2`, `{1,32,32,10}`
float32, 40,960 bytes). Session 16 reads it. The collaborator's design used
MediaPipe Face Mesh, which cannot run here; it did not need replacing, it
needed decoding.

**The limitation is real and is not papered over.** Two mouth corners give a
centre and a width, not an upper and lower lip, so **mouth-open detection does
not exist on this device**. The state-machine transition that depended on it
resolves to `UNCERTAIN` rather than to a fabricated verdict.

### Stage 1A — trained for this project, and why

The collaborator's delivered weights were tested before being adopted and were
**not usable**: 0 detections on 60 independent close-up pill photographs, and
0 on 115 blister-pack photographs, while scoring 0.83 on a synthetic pill
shape. They had memorised their own footage, and their training images are not
available. A replacement was trained on the Roboflow RF100 `pills` dataset
(451 images, **CC BY 4.0**) by
`tools/action_recogntion/build_pill_detector.py`, which reproduces the whole
path from download to generated C.

**Cutting the YOLOv8 head is what made INT8 viable**, and it is the most
transferable finding of the session:

| | Detections |
|---|---|
| INT8, **full graph** (mid-training checkpoint, 90 val images) | **0/90** |
| FP32, full graph — final model, 45-image **test** split | 42/45 |
| INT8, head cut + CPU decode — same test split | **42/45, agreeing with FP32 image for image** |

The DFL softmax and box arithmetic in YOLOv8's tail do not survive per-tensor
activation quantisation. Cutting after the six raw head convolutions and doing
the decode on the M55 — idle ~88% of the time — restores full accuracy.

### Measured footprint

| | Bytes |
|---|---|
| Activations, INT8 @160 (shipped) | **208,000** — in `AI_ARENA` at `0x34388000`, 17,280 spare |
| Weights, INT8 | **3,049,505** — external OSPI NOR at `0x73000000` |
| Activations @192 (rejected) | 267,264 — 42 KB over the arena |
| Activations @320 FP32 (as delivered) | 4,505,600 |

`AI_ARENA` was claimed and pattern-tested in Session 15 without a consumer.
It has one now, and the numbers came from `stedgeai analyze`, not from
arithmetic. **`MEMORY_MAP.md` §5's runbook was followed and found to be
right — with one correction**: under `--st-neural-art` the memory pool comes
from a profile in a `user_neuralart.json`, not from the CLI's `--memory-pool`
flag. Left to default, the tool placed activations at `0x342E0000`, inside the
face networks' block.

### Latency, and what has actually been observed

**Inferred, not directly instrumented.** On hardware the watch loop completes
roughly **68 ms per frame**, of which 33 ms is the deliberate
`osal_delay_ms()` pacing — so the NPU pass plus the CPU-side DFL decode is on
the order of **35 ms**. That is an arithmetic inference from frame counts and
wall time, not a measured interval like the face pipeline's 209 ms, and it is
labelled as such deliberately.

It matters because the state machine's thresholds are **frame counts**
calibrated to ~30 fps. At ~15 fps effective they are approximately half the
intended duration in wall-clock terms — tolerable, and recorded rather than
silently absorbed. If the pacing is ever changed, `LOCK_FRAMES`,
`RETREAT_CONFIRM_FRAMES` and `REARM_FRAMES` must be revisited together.

### What has been demonstrated on hardware, and what has not

Three rounds (`session_16_notes.md` Addenda 3, 4, 6):

- **Demonstrated**: the detector initialises on the NPU and fires 8-18 times
  per watch; the mouth landmarks decode to a real face with the corners at
  indices 3 and 4; the camera streams to PSRAM while the UI keeps drawing,
  with no display corruption and ~9,000 `WFI` entries per 10 s; and the state
  machine tracks an approach and **correctly declines** to certify an intake
  that did not occur.
- **Not demonstrated**: a successful `CONSUMED` verdict. That requires an
  object a person can swallow, and none was available. The distinction between
  "the pipeline works" and "the feature is validated" is real and is kept.

## 7. Stage 1C — the hand, and why it replaced the pill as the decision

> **Superseded by §9.** The skin-tone-and-baseline stage described here was
> replaced by a real hand model before Session 16 ended. This section is kept
> because the geometry argument in it — why an 8 px pill could never work —
> is still the reason the pipeline is shaped the way it is.

Session 16 shipped Stage 1A (a YOLOv8n pill detector on the NPU) and then
demoted it. This section records what runs today.

### What the arithmetic said, before any training

```
ROI = 1.5 × face width ≈ 225 mm across a 160 px input = 0.71 px/mm

    a 12 mm tablet   →   8.5 px
    a hand           →  ~64 px
```

An 8 px object in a frame measuring 63..78 mean luminance is not reliably
distinguishable from wall grain. Five hardware doses confirmed it: 2/54,
53/420, 6/243, 0/121 and 5/210 frames with a pill, boxes frequently on the
wall. A hard-negative retrain improved every offline number (mAP50 0.958 →
0.974, detection at deployment scale 57% → 79%) and changed none of that,
because the ceiling was pixel count, not training.

This is also not how the field does it. Medication-adherence systems decompose
the act into mini-activities — hand-to-mouth, pill-into-mouth, hand-off-mouth
— and detect the **hand** and the **mouth** (AiCure, US10402982 and family).
The smartwatch literature detects the same gesture from wrist accelerometry
and never sees a pill (JMIR Hum Factors 2023;10:e42714).

### The stage

`Src/ai/intake_hand.c`. No model, no NPU, no extra camera work: it runs over a
40×40 decimation of the same crop Stage 1A is handed.

1. **Skin mask**, ratio-based rather than absolute — skin is red-dominant and
   stays red-dominant in a dark frame, whereas the usual published `R > 95`
   rule fails outright at these exposures. Measures ~21-26% of the ROI on
   hardware, about what the face alone should occupy.
2. **Baseline subtraction**, not frame differencing. The baseline is captured
   six frames in, once the ISP has settled. Against a still reference the
   whole *area* of the hand differs; against the previous frame only its
   edges do. The face, the wall and the room are all in the baseline, so they
   cancel and what remains is what **arrived**.
3. **Illumination-invariant differencing** — the difference of the two frame
   means is subtracted, because the ISP hunts during a watch (`roi_mean`
   swings 85..127 within one dose) and a uniform shift otherwise lights up
   every skin cell at once.
4. **Largest connected component**, via an iterative 4-connected flood fill.
   This is what asks "is there *one* region big and solid enough to be a
   hand?" Scattered noise makes many tiny components and no large one.
5. **Gates**: ≥50 cells, ≥25% density within the component's own box, and a
   total-foreground ceiling for genuine exposure blowouts.
6. The baseline adapts ⅛ per frame **only on frames where nothing was found** —
   adapting on a frame containing a hand would absorb it into the background.

Output is a centroid and box in ROI pixel space, mapped into frame space by
the service exactly as the pill's is.

### How it feeds Stages 2 and 3

Unchanged. Stages 2 and 3 were written against *one tracked object* and do not
care what it is — they measure its distance to the mouth in face widths and
time the approach. The service picks the hand when there is one and the pill
otherwise; when both fire, the pill adds at most 0.25 to confidence. It can
strengthen a detection, never create one.

`MOUTH_ZONE_NORM_DIST` moved 0.15 → 0.30 as part of this, and that is a
recalibration rather than a relaxation: a pill's centroid *is* the thing
entering the mouth, but a hand's centroid is the middle of a ~90 mm blob whose
fingertips reach the lips, so at delivery it sits 40-60 mm away. The guards
against false confirmation are elsewhere and untouched — `SIMPLE_HOLD` (three
consecutive frames in the zone) and Stage 1C's own size and density gates.

### Measured

| | previous-frame differencing | baseline + components |
|---|---|---|
| `flood` rejections | 418 / 429 | 0, 0, 0 |
| `sparse` rejections | — | 0, 0, 0 |
| hand detected | 0 / 316 | 232/376, 69/258, 30/88 |
| best blob | 460 cells @ 28% | 427 @ 46%, 226 @ 45%, 324 @ 41% |

Stage 1A now samples at one frame in four. Idle during a watch went from 42%
to 59-78%, against 88% either side — the NPU was the entire cost, and the
overlay (which two rounds were spent optimising) never was.

### Still open

Multi-pill is untested. See `session_16_notes.md` Addendum 19.

## 8. Recorded hypothesis — the cup is probably the easiest target of the three

**Status: not planned, not scheduled, not endorsed as a requirement.** This is
a design note written down while the reasoning was fresh, because the argument
is short and the next person to touch this stage should not have to rediscover
it. Nothing in the shipped firmware depends on it.

The observation is that of the three objects this pipeline could track — a
pill, a hand, a drinking vessel — **the cup is the one best suited to the
detector we already built**, and it is the one we never tried.

### Size, which is the argument that sank the pill

The same arithmetic from §7, at the deployment ROI of 1.5 × face width:

| object | typical size | px in a 160 px input |
|---|---|---|
| tablet / capsule | 10-12 mm | **8.5** |
| hand | ~90 mm | ~64 |
| **cup or glass** | **70-90 mm across** | **50-64** |

A cup is in the same regime as a hand and six to seven times a pill. Whatever
killed Stage 1A does not apply.

### Why it should beat the hand as well

1. **It is a rigid manufactured object.** Detectors are good at canonical
   shapes: a cup is a near-constant silhouette with strong vertical sides and
   a rim ellipse, and it looks the same from most angles a patient will hold
   it at. A hand is articulated, self-occluding, and presents a different
   outline in every frame of the gesture it is performing.
2. **It removes this project's most uncomfortable dependency.** Stage 1C keys
   on skin tone, and its thresholds (`SKIN_MIN_R`, `SKIN_R_OVER_G`,
   `SKIN_R_OVER_B`) were measured against exactly one subject in one room.
   Skin varies enormously across patients and a ratio rule tuned on one person
   is a robustness problem and a fairness problem at once. A cup detector has
   no equivalent — a mug is a mug.
3. **The training data already exists, free and unencumbered.** `cup` and
   `wine glass` are COCO classes (41 and 46), so a stock pretrained YOLOv8n
   detects them with no custom dataset, no Roboflow licensing question, and no
   retrain. Compare with Stage 1A, which needed a CC BY dataset hunt, a
   hard-negative mining pass, and an unresolved note in
   `THIRD_PARTY_SOFTWARE.md` about whether trained weights are a derivative
   work of an AGPL framework.
4. **It is per-patient enrollable in a way a hand is not.** The same person
   uses the same cup every day. A future version could capture it once at
   registration and match appearance, not just class.
5. **Drinking is part of the actual protocol.** Most oral medication is taken
   with water, and the mini-activity decomposition the field uses lists
   `pick-up-water` and `drink-water` as first-class steps alongside
   `hand-to-mouth` (see §7's references). A cup reaching the mouth is a real
   adherence signal, not a proxy invented for our convenience.

### It drops into this pipeline with no structural change

Stages 2 and 3 take **one tracked object** and measure its distance to the
mouth in face widths. `intake_service.c` already chooses between two producers
(hand, then pill). A cup detector is a third producer and nothing downstream
would know the difference.

The NPU slot is already built and would be reused as-is: 160×160 INT8, DFL
head cut with decode on the M55, activations in `AI_ARENA` at `0x34388000`,
weights at `0x73000000`. A COCO-pretrained YOLOv8n filtered to one class goes
through the identical `export → cut → quantise → generate` path in
`AI_LESSONS.md` §2c. In principle this is a weekend, not a session.

### The honest case against

* **A cup at the mouth proves drinking, not swallowing.** The semantic link to
  "the dose was taken" is weaker than pill-to-mouth would have been — though
  it is no weaker than hand-to-mouth, which is what ships today, and the
  button remains what actually records the dose either way.
* **It assumes the protocol involves a cup.** Chewable tablets, sublinguals
  and doses taken dry would produce no signal at all, and a stage that is
  silent for a legitimate patient is worse than one that is merely imperfect.
* **It adds a second NPU network to a part where memory is the binding
  constraint.** `MEMORY_MAP.md` §6b shows the arena has 17,280 bytes of slack
  with the pill detector in it. A cup network would have to replace it, not
  join it.
* **The strongest version is a conjunction, not a substitution** — cup to
  mouth *and* hand to mouth, in sequence — which is more state machine, more
  tuning, and more ways to be wrong than anything here has budget for.

**If this is ever picked up**, the first experiment is cheap and should be run
before any firmware work: point a stock YOLOv8n at frames captured from this
device's own camera at the deployment ROI, filter to class 41, and measure the
hit rate the way `tools/action_recogntion/eval_hn.py` measures Stage 1A's.
That one number decides whether the rest of the argument matters.

## 9. What actually ships — the three models, and what each one decides

Sections 7 and 8 are the record of how this was arrived at. This section is
the state of the delivered system, and it supersedes §7's description of the
skin-tone heuristic, which no longer runs.

### The models

| stage | model | licence | decides |
|---|---|---|---|
| Face detection | CenterFace, INT8 | ST terms | who is at the device; also supplies the mouth |
| Face embedding | MobileFaceNet, INT8 | ST terms | whether they are an enrolled patient |
| **Hand** | **MediaPipe hand landmarks, 224×224 INT8, 21 keypoints** | **Apache-2.0** | **whether a hand reached the mouth** |
| Pill | YOLOv8n single-class, 160×160 INT8, head cut | AGPL question, see THIRD_PARTY §2.7b | corroboration only — never creates a detection |

The mouth costs nothing: CenterFace already emits both mouth corners on a
tensor this firmware defines. Stage 1B is a decode, not an inference.

### The intake pipeline, as built

```
mouth landmarks (free, from the last face capture)
      -> ROI, 1.5 face widths, centred on the mouth, camera -> PSRAM
      -> hand landmark model -> presence + 21 keypoints
      -> fingertip = midpoint of thumb tip (4) and index tip (8)
      -> distance to mouth in FACE WIDTHS, and mouth-inside-hand-box
      -> three consecutive frames -> dose observed
```

The pill detector runs on one frame in four over its own crop, and its only
effect is on the wording of the audit line: `pill reached mouth` when it saw a
pill in the same window, `hand reached mouth` when it did not.

### What it does, stated honestly

**It looks for a hand near the mouth.** That is the whole claim, and the
wording is deliberate.

It is *not* general hand tracking. MediaPipe's landmark model is not a
detector — in its own pipeline a palm detector finds the hand and hands it a
tight crop with the hand filling the frame. We do not ship that palm detector,
so presence is reliable when the hand is centred and large in the crop, and
unreliable elsewhere. Since the ROI is centred on the mouth, "centred and
large" and "at the mouth" are the same condition — which is why this works for
the question being asked and would not work for "where is the hand now".

Measured on hardware:

| input | hand presence |
|---|---|
| a face | 0.0078 |
| 400 random images | never above 0.035 |
| an ear, waved deliberately | does not fire |
| a hand at the mouth | ~0.50, and confirms |

The ear case is the one worth keeping in view. Four rounds of skin-tone
heuristics could not separate a moving ear from a hand, because at that level
of description they are the same thing: skin-coloured, solid, moving. A model
that knows what a hand is settles it in one step.

### Costs, so they are not discovered later

* A hand inference measures **285–302 ms**. ST publishes 20.75 ms for this
  model; that is an all-internal figure and ~978 KB of our activations are in
  PSRAM. See MEMORY_MAP §8b.
* A watch therefore samples the gesture roughly 3–5 times, which is enough for
  a three-frame hold and not much more.

### Power: the watch is loud and rare, and rare is what matters

Idle falls to **7–50% during an intake watch**, against ~88% either side. That
looks alarming quoted on its own, and it is worth doing the arithmetic rather
than either hiding it or letting it undermine the figure it does not actually
threaten.

A watch runs only on the confirm screen, lasts at most 30 s, and ends early
the moment the patient taps. Even at **four doses a day with the full 30 s**
every time:

```
    4 x 30 s  =  120 s of watch      per 86,400 s day  =  0.14 % of the day

    day average = 0.9986 x 88 %  +  0.0014 x 10 %  =  87.9 %
```

So the daily average moves from about 88.0 % to about **87.9 %** — a tenth of
a percentage point. The headline power figure is a statement about the device
at rest, and the device is at rest 99.86 % of the time.

The right way to quote it is therefore both numbers and their duty cycle, not
a caveat that reads like a retraction:

> Idle 88 % at rest; ~10 % for at most 30 s per dose while the intake watch
> runs, which is 0.14 % of a four-dose day.

What would change that conclusion is the watch running when nobody is being
dispensed to — which is why `intake_end()` is called on every exit from the
confirm screen and the watch has its own 30 s timeout. Both are what keep this
a rounding error rather than a duty cycle.

### Why the pill detector is still here

The Program Plan commits to CNN classification of pills. The honest position,
recorded in `PROGRAM_PLAN_RECONCILIATION.md` §1, is that **the hopper does the
classification** — one medicine per hopper, mechanically, which is foolproof in
a way vision is not. The detector's contribution is to corroborate that a pill
was present in the frame where a hand reached the mouth. That is a real part of
the adherence claim without pretending to identify the medicine.

Face recognition + action recognition + pill corroboration, with mechanical
classification, is a stronger and more truthful system than vision-based pill
identification would have been.
