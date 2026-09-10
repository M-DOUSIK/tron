# Session 16 Notes — Action Recognition: Did They Actually Take It?

## Base folder, and why

**Built on `sessions/session_15/`, copied to `sessions/session_16/`.**

`prompts/session_16.md` says to take the highest-numbered `sessions/session_NN`
with a matching `session_NN_notes.md` recording a completed, hardware-verified
run. At the time this session started:

- `ls sessions/` ended at `session_15`. There is no `session_14` and there
  never will be — it is a **retired number** (`MASTER_PROJECT_PLAN.md` v13).
- `ls documents/milestones/` ended at `session_15_notes.md`.
- `session_15_notes.md` records two hardware rounds, Addendum 1b's Round 4
  passing end to end, and both build configurations verified.

So Session 15 was both the highest-numbered folder and a completed,
hardware-verified one. **Session 17 has not run**, so there is no dispenser
task, no EXTI ISR and no new GPIO to collide with.

---

## Summary

The headline is not the one this session was expected to have.

**The collaborator's trained model does not work outside their own footage,
and that was established by measurement before anything was built on it.** A
replacement was trained from scratch, on openly-licensed data, and it is what
ships. The collaborator's *design* — the three-stage pipeline and the guarded
state machine — is intact and is the valuable half of what they contributed.

| Part | Outcome | On hardware |
|---|---|---|
| **0** | Mouth landmarks decoded from `FD_OUT_LANDMARKS`. Free, as predicted. | **VERIFIED** — ordering measured, mouth at indices 3 and 4 (Addendum 6) |
| **A** | Collaborator's detector measured unusable → **new detector trained**, quantised, 208,000 B activations, **fits `AI_ARENA`** | **VERIFIED** — initialises on the NPU, 8-18 detections per watch |
| **B** | Camera repointed to PSRAM; UI keeps the framebuffer; two new `LPEN` bits | **VERIFIED** — no display corruption, ~9,000 WFI entries per 10 s while the camera DMA'd to PSRAM |
| **C** | Stages 2 and 3 in C. `UNCERTAIN` preserved; the missing `is_open` recorded, not faked | **PARTIALLY** — the declining path ran correctly and repeatedly; the confirming path is untested (see below) |
| **D** | Corroboration only. The button still confirms every dose | **VERIFIED** — verdict appears as a log suffix, never as a gate |
| **E** | Docs corrected — the pill-detection framing added without overclaiming | n/a |

**Three hardware rounds ran** (Addenda 3, 4 and 6). The pipeline works end to
end: the detector finds a pill-like object, Stage 1B places the mouth, Stage 2
computes the geometry and Stage 3 tracks the approach — and correctly
**declines** to certify an intake that did not happen.

**The one thing nobody has seen is a successful CONSUMED verdict**, because
that requires an object a person can actually swallow, and none was available.
That gap is stated plainly here rather than papered over, and it is the
difference between "the pipeline works" (true, demonstrated) and "the feature
is validated" (not yet).

**One observation needs its caveat carried with it**: face matching rejected
the enrolled patient about one time in three during this round — but the
project owner identified **varying lighting** across the test as the cause,
and that is a sufficient explanation. It is therefore a property of the test
conditions, not of the device. The threshold was left alone regardless, since
no impostor score has ever been measured here. Addendum 6 has the numbers and
the correction.

---

## Part 0 — verifying the analysis rather than redoing it

Both claims in the prompt's Part 0 hold, and both were checked against the
generated header rather than against the prose.

**The landmark tensor is real.** `FSBL/Inc/stai_fd.h` line 397:

```
#define STAI_FD_OUT_2_CHANNEL (10)      /* 5 landmarks x (y,x) */
#define STAI_FD_OUT_2_HEIGHT  (32)
#define STAI_FD_OUT_2_WIDTH   (32)
#define STAI_FD_OUT_2_SHAPE   { 1, 32, 32, 10 }     <- channel-LAST
#define STAI_FD_OUT_2_SIZE_BYTES (40960)
```

`ai_vision.c` has `#define FD_OUT_LANDMARKS 1`, which indexes `fd_outputs[1]`,
which is OUT_2. It has been computed on every capture since Session 08B and
never read. It is read now.

**One thing the prompt did not mention, and it matters.** The three output
tensors the pipeline already consumed were each being D-cache-invalidated
before use; the landmark tensor was not, because nothing read it. Reading
40 KB of NPU output without invalidating it would have returned whatever the
CPU happened to hold — the same class of fault as the existing three lines and
exactly as invisible. A fourth `SCB_InvalidateDCache_by_Addr()` was added.

**What is still unverified, and is a hardware check:** the landmark ORDERING.
The conventional CenterFace order puts the mouth corners at indices 3 and 4,
but that is a convention, not a guarantee for this export.
`ai_vision_dump_landmarks()` prints all five so a human can look at real
numbers from a real face. If the "mouth" lands on the eyes, change
`LM_MOUTH_L`/`LM_MOUTH_R` in `ai_vision.c` and nothing else.

---

## Part A — the detector, and the measurement that changed the session

### A1. The delivered model does not generalise

Before adopting `tools/action_recogntion/models/pill_detector/best.onnx`, it
was tested. It is a genuine trained YOLOv8n — 320×320, single class `pill`,
opset 12, `nms: False`, trained from `D:\College\TRON\datasets\pill_dataset`
per its own ONNX metadata. It runs. It just does not work on anything but its
own footage:

| Input | Result |
|---|---|
| Synthetic white ellipse on a skin-tone ground, r=40 px | **conf 0.83** |
| Same, r=70 px | conf 0.83 |
| 60 real close-up pill photos (Roboflow RF100 `pills`) | **0 detections** |
| 115 blister-pack photos (Ultralytics `medical-pills`) | **0 detections** |
| Random noise, flat grey | ~0.000 |

Preprocessing was ruled out as the cause before drawing that conclusion: RGB
vs BGR, 0–1 vs 0–255, full-frame vs progressively tighter centre crops. The
synthetic-pill result is what proves the pipeline was correct — the model
responds strongly to the *abstract shape* of a pill filling the frame, and not
at all to real pills. It has memorised its training set.

**Why that ended the "just quantise it" plan.** INT8 calibration derives
activation ranges from images the model actually responds to. Calibrating on
images where every output is ~0 produces meaningless ranges. Their training
images are not available and the collaborator was not reachable, so the model
could be neither calibrated honestly nor validated. It is kept in the tree as
the provenance of the design; the weights are replaced.

### A2. Training a detector that does generalise

`tools/action_recogntion/build_pill_detector.py` does the whole thing
reproducibly: fetch → convert → train → export → cut → quantise → validate →
generate.

- **Data:** Roboflow Universe `pills-sxdht` (RF100), 451 images, 496 boxes,
  **CC BY 4.0**, via the HuggingFace mirror `Francesco/pills-sxdht`. Eight
  medication sub-classes collapsed to one `pill` class — MedSight never needs
  to know *which* medication (see Part E), and collapsing multiplies the
  per-class data by eight, which for 451 images matters.
- **CC BY was chosen over the obvious alternative deliberately.** Ultralytics'
  `medical-pills` set is AGPL-3.0, and every other third-party component in
  this firmware is SLA0044, BSD or similar. See "Licensing" below — the
  question is not fully closed and is recorded rather than glossed.
- **Model:** YOLOv8n from COCO-pretrained weights, 192 px, 80 epochs, CPU.
  Augmented for a handheld shot: ±25° rotation, 0.6 scale jitter, both flips,
  strong HSV, mosaic.

Validation mAP50 passed 0.90 by epoch 13 and finished at **0.972** after the
full 80 epochs (35 minutes on CPU). Final precision 0.96, recall 0.96.

### A3. Quantisation destroyed the model, and cutting the head fixed it

This is the most useful engineering result of the session.

A straight full-graph INT8 quantisation measured **0 of 90** detections on
held-out images, against the FP32 model's 84 of 90. Not degraded — dead.

The cause is YOLOv8's decode tail: a softmax over the DFL bins, then
Slice/Sub/Add/Div box arithmetic, then a Concat of box coordinates with class
scores. Those tensors' dynamic ranges differ from each other and from the
convolutional feature maps by orders of magnitude, and per-tensor activation
quantisation cannot serve them all at once.

The fix is the one the prompt's Part A step 2 suggested and ST's own YOLO
deployment guidance uses: **cut the head.** The graph is cut after the six raw
head convolutions; the NPU runs the convolutional body and the Cortex-M55 does
the decode (`FSBL/Src/ai/intake_detect.c`), which it can well afford at ~88%
idle.

Measured twice — once mid-training on an early checkpoint (which is where the
"quantisation is free once the head is off" finding came from), and again on
the final 80-epoch model:

| Final model, 160 px | val (90 imgs) | **test (45 imgs)** |
|---|---|---|
| FP32, full graph | 89/90 — 98.9% | 42/45 — 93.3% |
| FP32, cut + the C decode's reference implementation | 89/90 — **decode verified correct** | 42/45 — **verified** |
| INT8, cut + same decode | 88/90 — 97.8% | **42/45 — 93.3%** |
| Agreement, FP32 full vs INT8 cut | 98.9% | **100%** |

**The test column is the honest one** — that split was used for neither
training nor calibration. On it, INT8 with the head cut matches FP32 exactly,
image for image.

The earlier mid-training measurement, kept because it is what made the
decision:

| Early checkpoint, 192 px | Detections on 90 val images |
|---|---|
| FP32, full graph | 84/90 |
| **INT8, FULL GRAPH (head attached)** | **0/90** |
| INT8, cut + decode | 85/90 |

Quantisation is effectively free once the head is off. The middle row is the
one that matters for the firmware: it proves the decode ported into C is
numerically right, because it reproduces the full graph's result exactly.

**Two toolchain facts worth writing down**, both of which cost a round:

- **Export at opset 13, not 12.** Per-channel weight quantisation emits
  `DequantizeLinear` with an `axis` attribute, which opset 12 does not define.
  An opset-12 export produces a model onnxruntime refuses to load
  (`Unrecognized attribute: axis for operator DequantizeLinear`). The
  collaborator's export used opset 12, which is fine for them and not for us.
- **Activations must be SIGNED int8.** ST Edge AI rejects unsigned outright:
  `NOT IMPLEMENTED: Onnx exporting model with quantized unsigned integer
  format is not supported`. `QuantType.QUInt8` is the onnxruntime default for
  activations, so this is an easy trap.

### A4. Memory — measured, and it decided the input size

`stedgeai analyze`, ST Edge AI Core v4.0.0 (STM32CubeAI 12.0.0):

| Model | Activations | Weights |
|---|---|---|
| Collaborator's, 320×320, **FP32 as delivered** | 4,505,600 | 12,065,412 |
| Ours, 192×192, INT8, head cut | **267,264** | 3,047,777 |
| Ours, 160×160, INT8, head cut (early checkpoint) | **201,600** | 3,046,129 |
| **Ours, 160×160, INT8, head cut — SHIPPED** | **208,000** | **3,049,505** |

`AI_ARENA` is **225,280 bytes**. So 192 overruns it by 41,984 bytes and the
shipped 160 px model fits with **17,280 bytes spare**.

The arena cannot simply be grown to take 192: AXISRAM6 ends at `0x343BFFF7`,
which caps a region starting at `0x34388000` at 229,368 bytes — still short.

PSRAM was the other option and was rejected **on latency, not capacity**
(12.94 MB is free there — see Part B). PSRAM is `THROUGHPUT=MID LATENCY=HIGH`
against the arena's `HIGH/LOW`, and the ported state machine's frame counts
are calibrated to ~30 fps. A detector running at a few frames per second would
stretch a 15-frame retreat confirmation into several seconds and quietly
change what every threshold means.

**And 160 costs nothing measurable**: 84/90, exactly matching FP32 at 192. It
is free because this device feeds a tight mouth-centred ROI (two face widths)
rather than a whole scene, so the pill is already large in frame. The
collaborator chose 320 for a webcam and a hand-centred crop; we are not in
that situation.

### A5. The default allocation would have collided, silently

Worth recording because it is the kind of thing that costs a hardware round.

Run without a memory-pool constraint, ST Edge AI placed the activations at
**`0x342E0000` (npuRAM5)** — which is inside the block the two face networks
already occupy (`0x34200000`–`0x34387FFF`, `MEMORY_MAP.md` §2) and inside the
region the framebuffer aliases. Nothing warns about this; the tool is
allocating from a description of the *chip*, not of this application.

`--memory-pool` alone does **not** fix it: with `--st-neural-art` the pool
comes from a profile in a `user_neuralart.json`, and the CLI's own
`--memory-pool` flag applies to a different path. Verified by reading the
generated `network.c`'s own pool comments, which still showed the stock
layout.

The working form is a profile file:

```
stedgeai generate --model cut160_s8.onnx --target stm32n6 --name pill \
  --st-neural-art "medsight-arena@medsight_cfg/medsight_neuralart.json"
```

where the profile names an `.mpool` with every pool zeroed except `npuRAM6`,
redefined as `0x34388000` / 220 KB, and `octoFlash` for the weights. Both
files are produced by `build_pill_detector.py`.

**`MEMORY_MAP.md` §5 step 2 already warned about exactly this** — "give ST
Edge AI an explicit memory-pool description naming that address and size
rather than letting it default". It was right, and it did not know the
mechanism had moved under `--st-neural-art`. That is now recorded.

---

## Part B — the camera path

The real engineering problem of the session, and it is firmware, not model.

Since Session 03 the DCMIPP has written into `BUFFER_ADDRESS` (`0x34200000`),
which *is* the display framebuffer. Session 09's "Bug 2" found that resuming
the camera behind a drawn screen erases it at ~30 fps, which is why every
screen after the face capture is static UI with the camera stopped.

Action recognition needs the camera running for the whole `STATE_CONFIRM_TAKEN`
window **while the UI keeps drawing**. So the camera is pointed elsewhere.

### B1. Where, and why that address

PSRAM (XSPI1) is 16 MB at `0x90000000` and is **not empty** — MobileFaceNet
uses it as activation scratch. The extent was established the same way
`MEMORY_MAP.md` §2 established the AXISRAM one: by extracting every address
literal from the generated sources rather than trusting the pool declaration,
which claims all 16 MB.

```
0x90000000 .. 0x90310000    3,211,264 bytes   MobileFaceNet scratch
0x90310000 .. 0x91000000    12.94 MB          free
```

The intake frame sits at **`0x90400000`** — the next 4 MB boundary, leaving
~0.9 MB of unclaimed slack above the embedder rather than butting against it.
One 800×480 RGB565 frame is 768,000 bytes, ending at `0x904BB800`.

### B2. `LPEN` — two bits, not one

`session_12_notes.md` Addendum 9 is a standing constraint and this is the
first new DMA destination since Session 12. The destination is outside
AXISRAM3–6, so the rule applies in full.

Two bits were needed:

| Bit | Why |
|---|---|
| `RCC_AHB5LPENR_XSPI1LPEN` | the PSRAM controller itself |
| `RCC_AHB5LPENR_XSPIMLPEN` | the **XSPI manager**, which every memory-mapped access is routed through — gating it stalls the DCMIPP's writes just as completely, with the identical silent signature |

The second is the one easy to miss, and missing it would have presented as an
intake verdict that is wrong or `UNCERTAIN` on a device that is asleep ~88% of
the time, with the confirm screen drawing perfectly and every register
reporting health. Addendum 9 again, one peripheral over.

**A latent issue found while checking, not a live bug.** `XSPI2LPEN` is also
clear, and the NPU reads its weights from OSPI NOR at `0x71000000` during
inference. That has never bitten because the ST runtime is built
`LL_ATON_OSAL_BARE_METAL` and **polls**, so the CPU is awake for the whole run
and never sleeps mid-inference. Recorded because it stops being latent the
moment inference is made to block on an OS primitive instead of spinning.

### B3. What was deliberately not changed

- **`task_camera_isp` is untouched and `g_isp_suspend` is not asserted.**
  Unlike a face capture, the point here is that the camera keeps running and
  keeps its auto-exposure converging while the patient moves a pill toward
  their mouth. Suspending the ISP would leave the frames the detector sees
  badly exposed.
- **The frame-buffer ownership invariant is intact.** The AI task reads the
  PSRAM frame; it never touches `BUFFER_ADDRESS` during an intake watch. There
  is still exactly one owner at a time.
- **`intake_camera_stop()` restores the pipe to `BUFFER_ADDRESS`.** Session
  15's `STATE_CAMERA_DISPENSE` calls `camera_start()` expecting the display
  framebuffer, and a pipe left aimed at PSRAM would give it a preview that
  never appears — a regression that would look like a camera fault.

---

## Part C — Stages 2 and 3 in C

`FSBL/Src/ai/intake_features.c` and `FSBL/Src/ai/intake_fsm.c`. Feature names
and state names match `main/main.py` deliberately, so the C and the Python can
be read side by side when a threshold is tuned.

### What was lost relative to their Python, and how it is handled

Their Stage 1B was MediaPipe Face Mesh (468 landmarks) and their pill tracker
also used MediaPipe Hands. Neither runs on this part. Three capabilities do
not exist here:

1. **Mouth-open detection is gone.** Two mouth corners give a centre and a
   width; they do not give an upper and a lower lip. Their state machine used
   `mouth_data["is_open"]` in two transitions.
2. **The inner-lip polygon is gone.** Their `APPROACHING → AT_MOUTH` test was
   a point-in-polygon against the inner lip contour. Ours is a radial test
   about the mouth centre at half the corner-to-corner width. That is a proxy
   for the **polygon** and a fair one, because a mouth is roughly an ellipse
   about that centre. It is **not** a proxy for `is_open`.
3. **Hand tracking is gone**, so there is no pinch point and no kinematic
   estimation of an occluded pill. Their tracker kept a pill alive for up to
   30 occluded frames by offsetting from the hand; we cannot. It also means
   the detector's ROI is **mouth**-centred rather than **hand**-centred.

**The `is_open` transition is not faked.** Their `AT_MOUTH` state, on losing
sight of the pill, split on `mouth_was_recently_open and entered_inner_mouth`
→ `RETREATING_CHECK`, else → `NOT_CONSUMED_CLOSED`. Without the signal we
cannot distinguish "it went in" from "it was held against closed lips", so:

- the branch that **produces CONSUMED** is unchanged — pill entered the mouth
  zone, vanished, stayed vanished through a retreat;
- the branch that produced a confident `NOT_CONSUMED_CLOSED` now resolves to
  **`UNCERTAIN`**.

That is the cheap direction to lose confidence in. This subsystem only ever
corroborates a button, so losing certainty about one *failure* mode costs
little; inventing a signal would have cost the whole feature's credibility.

`UNCERTAIN` is a first-class outcome throughout, per the prompt: a system that
only ever says CONSUMED or NOT CONSUMED will be wrong confidently, which for a
medication device is the worst available behaviour.

---

## Part D — corroboration, never replacement

- The **"✓ I Took It" button still confirms every dose.** Nothing in this
  session can prevent a confirmation being recorded.
- The verdict is read **at the moment of the tap** and only chooses a suffix:
  `CONFIRMED: DOUSIK took the 08:00 dose (on time, gesture confirmed)` /
  `(on time, gesture NOT observed)` / `(on time, gesture uncertain)`.
- Every failure path — no detector, no camera, no face, a stalled pipe, a
  timeout, the feature compiled out — ends in "uncertain". None can block a
  dose.
- Behind **`MEDSIGHT_ACTION_RECOGNITION`** (default 1). With it 0 every entry
  point is an empty function and the firmware behaves exactly as Session 15
  did.
- It runs on the **existing** `ai` task at priority 3, woken by a fourth bit
  (`AI_FLAG_INTAKE`) on the **same** event flag. No second AI task, because
  the frame-buffer ownership argument in `SOFTWARE_ARCHITECTURE.md` §9 depends
  on there being one owner.

That fourth bit is worth a sentence for a µT-Kernel-literate reader: the AI
task must wait for "a capture request **or** an intake request" in one
blocking call, which is exactly what an event flag's OR-wait expresses and
what a queue or a semaphore cannot. It is the same argument Session 12 made
for the original three bits, continuing to pay.

---

## Licensing — stated, not resolved

Recorded here and in `THIRD_PARTY_SOFTWARE.md` because a submission that
quietly ships a licence question is worse than one that names it.

- **Training data: CC BY 4.0** (Roboflow RF100 `pills`). Clean; needs
  attribution, which it has.
- **Training framework: Ultralytics YOLOv8, AGPL-3.0.** Whether trained
  weights are a derivative work of the framework that produced them is
  genuinely unsettled, and this project is not the place to decide it. What is
  certain is that the *architecture* is YOLOv8 and the exporter stamps
  `license: AGPL-3.0` into the ONNX metadata.
- **The tension** is that every other third-party component here is SLA0044,
  BSD or similar, and AGPL is strong copyleft.
- **The clean escape, if it is ever wanted:** an ST model-zoo detector under
  SLA0044, fine-tuned on this same CC BY dataset. Nothing else in the pipeline
  changes — the cut, the quantisation, the memory profile and the C decode are
  all architecture-independent.

The project owner asked for the ST model-zoo route and it was not taken *this
session* for one reason: ST's zoo has no pill class, so it needs the same
fine-tuning step, and doing that well needs more than the 451 images
available. Taking the shortest path to a working detector was the explicit
instruction. This is the deviation, recorded rather than left silent.

---

## Files

**New**

| File | What |
|---|---|
| `FSBL/Inc/ai/intake.h` | The whole subsystem's contract, and the honest account of what MediaPipe gave them that we do not have |
| `FSBL/Inc/ai/intake_camera.h` | The PSRAM address derivation and the `LPEN` story |
| `FSBL/Src/ai/intake_features.c` | Stage 2 — geometry, normalised by face width |
| `FSBL/Src/ai/intake_fsm.c` | Stage 3 — the ported guarded state machine |
| `FSBL/Src/ai/intake_detect.c` | Stage 1A — NPU wrapper + the DFL decode the cut head left us |
| `FSBL/Src/ai/intake_camera.c` | Part B — DCMIPP to PSRAM |
| `FSBL/Src/ai/intake_service.c` | Orchestration on the existing AI task |
| `tools/action_recogntion/build_pill_detector.py` | The whole model pipeline, reproducible |

**Modified**

| File | Change |
|---|---|
| `FSBL/Src/ai/ai_vision.c` | Landmark decode, the missing D-cache invalidate, `AI_FLAG_INTAKE` |
| `FSBL/Inc/ai_vision.h` | `ms_face_landmarks_t`, `ai_vision_get_mouth()`, the dump diagnostic |
| `FSBL/Src/main.c` | `XSPI1LPEN` + `XSPIMLPEN` in `ms_configure_sleep_clocks()` |
| `FSBL/Src/ui/state_machine.c` | `intake_begin()` on entry, `intake_end()` on **all three** exits, verdict suffix |

`FSBL/Src/ai/` is a **linked folder** (`<type>2</type>`) in `.project`, so the
new `.c` files are picked up without per-file `<link>` entries — unlike
Session 09's `registration_ui.c`, which was in `Application/User` and had to be
listed individually (Bug 1 in `session_09_notes.md`).

---

## Build status

**Both configurations build clean through the real headless STM32CubeIDE
build** — the one that regenerates `Debug/`, `Release/` and every `subdir.mk`
from `.project`/`.cproject` first, which is the check
`ENGINEERING_LESSONS.md` requires and the one that catches what a raw
command-line `make` cannot.

```
19:31:58  Build Finished. 0 errors, 2 warnings.   (Debug)
19:32:45  Build Finished. 0 errors, 4 warnings.   (Release)
```

Warning counts are **unchanged from Session 15** (2 Debug / 4 Release, all in
pre-existing ST code). Nothing this session added warns.

| | text | data | bss | ROM | RAM |
|---|---|---|---|---|---|
| Debug | 1,088,856 | 4,056 | 664,200 | **75.5%** (251 KB free) | **63.8%** (370 KB free) |
| Release | 916,184 | 4,052 | 664,176 | **59.1%** | **63.8%** |
| Debug, `MEDSIGHT_ACTION_RECOGNITION=0` | 938,912 | 4,052 | 664,000 | — | — |

Session 15's Debug baseline was ROM 47.3% / RAM 77.6%. **Both regions are
healthier now than they were then, with a third network added** — see below.

### The RAM scare, and the two fixes

The first successful link put Debug **RAM at 95.9%, with 42,464 bytes free**.
That is precisely the condition Session 15 described for Debug ROM at 90.6%:
"roughly one feature away from a link failure, and a link failure at 3 a.m.
before a deadline is not a good way to discover a linker script nobody
re-derived." It was not shippable and was not left.

Two changes fixed it, and both are improvements rather than workarounds:

1. **Deleted a 76,800-byte staging image buffer.** `intake_service.c`
   originally held its own CHW image and copied it into the network's input.
   The camera's ROI now writes *directly* into `intake_detect_input()`, and
   the int8 bias is applied in place. One image buffer in the system instead
   of two, and one fewer full copy per frame.
2. **Moved `.rodata` from `RAM` to `ROM`.** `MEMORY_MAP.md` §4 considered
   exactly this in Session 15 and declined it, correctly, because "the NPU
   cannot reach either bank, so moving it buys nothing that matters". The
   premise changed: it now buys 260 KB of headroom in the region that is
   actually constrained. It does **not** re-run `AI_LESSONS.md`'s bus fault —
   that was the NPU failing to read *weights* in AXISRAM1, and the NPU can
   reach neither AXISRAM1 nor AXISRAM2, so moving CPU-read constant data
   between them changes nothing the NPU sees.

| Debug | ROM | RAM |
|---|---|---|
| Session 15 | 47.3% | 77.6% |
| Session 16, first link | 50.7% | **95.9%** |
| Session 16, after both fixes | 75.5% | **63.8%** |

### `.xspi2` layout — verified unchanged

`ENGINEERING_LESSONS.md` requires this check whenever anything about the model
files or their compilation changes, and Session 12 lost a round to Debug and
Release emitting the blobs in opposite order:

```
$ arm-none-eabi-nm -n <elf> | grep '^71' | head -1
Debug:   71000000 b _ec_blob_faceid_1
Release: 71000000 b _ec_blob_faceid_1
```

Identical to each other and to the Session 15 layout, so **the weight image
already flashed at `0x71000000` still serves this build**. The pill detector
was generated *without* the epoch controller, so it adds no blob there — only
its own weight region at `0x73000000`, which is a separate one-time flash.
`-fno-toplevel-reorder` remains on Release.

### Things that had to be fixed to get here

Recorded because each would otherwise be rediscovered:

- **`STAI_PILL_OUT_n_SCALES` is a braced initialiser, not an array.**
  `STAI_PILL_OUT_1_SCALES[0]` does not compile — it subscripts `{ 0.111f }`.
  A compound literal indexes correctly but is not a constant expression, so it
  cannot initialise a static either. The quantisation tables are filled at
  init by a small macro instead. Transcribing the twelve values as literals
  would have compiled and then silently rotted at the next regeneration — and
  a wrong dequantisation scale does not fail loudly, it produces confident,
  plausible, wrong boxes.
- **The pill network's input is `PREALLOCATED` and SIGNED**, unlike the
  CenterFace detector's (bound with `set_inputs()`) and unlike both face
  networks' raw 0..255 byte convention. Its scale is 1/255 with zero point
  -128, so the conversion is `rgb - 128`. Writing raw bytes through — the
  habit both existing networks teach — would have handed the model an image
  whose mid-grey reads as white.
- **`FSBL/Src/ai/` is a linked FOLDER in `.project`** (`<type>2</type>`), so
  the new `.c` files were picked up without per-file `<link>` entries. This is
  the opposite of Session 09's Bug 1, where `registration_ui.c` sat in
  `Application/User` and had to be listed individually. Worth knowing which
  directories are which.
- **A raw command-line `make` in `Release/` built a stale tree.** Its
  `subdir.mk` had zero references to the new files while `Debug/`'s had 21.
  Only the IDE regenerates them from `.cproject`. This is Session 11's
  Addendum 1 and Session 12's headless-build lesson, met again.

---

## What has NOT been done — the hardware checklist

Everything above is desktop-measured or code-complete. **None of it has run on
the board.** These are the Definition-of-Done items that remain, in order:

1. ~~**Copy the generated network into the tree**~~ - **DONE.** `pill.c`,
   `pill.h`, `stai_pill.c`, `stai_pill.h` are in `FSBL/Src/ai/` and
   `FSBL/Inc/`. No `static` fixes were needed (checked: **zero** global symbol
   collisions against the two existing networks - `--name pill` prefixes
   everything) and no `.xspi2` tagging was needed either, because this network
   references its weights by absolute address rather than through linked
   arrays.

2. **Flash the pill detector's weights - the one hardware step that must
   happen before anything else works.** 3,049,505 bytes to `0x73000000`, a
   region nothing else uses. Per `AI_LESSONS.md`, `HOTPLUG` mode so the loader
   is not fighting a held reset:

   ```powershell
   $cli    = "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"
   $loader = "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\ExternalLoader\MX66UW1G45G_STM32N6570-DK.stldr"
   $raw    = "C:\Users\Dousik\Workspace\TRON\sessions\session_16\STM32CubeIDE\FSBL\Debug\weights_flash\pill_atonbuf.xSPI2.raw"

   & $cli -c port=SWD mode=HOTPLUG -el $loader -w $raw 0x73000000
   & $cli -c port=SWD mode=HOTPLUG -el $loader -r32 0x73000000 8
   ```

   **The three existing weight regions do NOT need re-flashing.** Verified:
   `arm-none-eabi-nm -n <elf> | grep '^71' | head -1` gives
   `71000000 b _ec_blob_faceid_1` in both configurations, identical to Session
   15, so the image already in flash still serves this build. The pill
   detector was generated **without** the epoch controller, so it contributes
   nothing at `0x71000000`.

   **If the board hangs at boot right after this flash** - typically inside a
   `HAL_XSPI_GET_FLAG` polling loop - do a full power cycle, unplugging the
   USB cable rather than resetting. `AI_LESSONS.md` documents this exactly;
   the flash content survives, only the chip's live bus state needs clearing.

3. ~~**Build both configurations clean**~~ - **DONE**, via the real headless
   IDE build. See "Build status" above.
4. **Verify the landmark ordering** — `ai_vision_dump_landmarks()` on a real
   face. This gates everything in Stage 1B.
5. **Verify the arena placement** — the generated `network.c`'s pool comments
   must say `0x34388000`, not `0x342E0000`.
6. **Cold boot, with the camera writing to PSRAM.** This is the `LPEN` test
   and it is the one that has cost this project six rounds before. A warm
   re-flash will hide the fault.
7. **Measure the detector's latency** the way the face pipeline's 209 ms was
   measured, and record whether the ~30 fps the state machine assumes is
   actually achieved. **If it is not, the frame-count thresholds in
   `intake_fsm.c` are wrong and must be rescaled** — they are frame counts,
   not times.
8. **An honest accuracy claim from a real run.** How many intakes out of how
   many attempts, and what the failure modes looked like. The desktop numbers
   in Part A are from a public dataset on a PC and **must not be quoted as
   this device's accuracy**.
9. **Confirm `MEDSIGHT_ACTION_RECOGNITION=0` still behaves exactly as
   Session 15 did.**

A note for whoever runs item 8: there are no physical pills available to the
project owner at the time of writing. Until there are, the honest statement is
that the detector is validated on a public dataset and unvalidated on this
device — and that is what the documents say.

---

## One packaging item that is easy to miss

**`/tools/` is in `.gitignore`.** That is a long-standing and sensible rule —
the directory holds vendored reference repositories and multi-megabyte model
binaries. But Session 16 put a genuine deliverable in there:
`tools/action_recogntion/build_pill_detector.py`, the script that reproduces
the pill detector from scratch.

The Program Plan's **Open Source Commitment** promises "full source code,
model training scripts, and documentation released openly upon submission".
`PROGRAM_PLAN_RECONCILIATION.md` §7 answered the "training scripts" clause
with "not applicable — no model was trained for this project", and that answer
is no longer true. The row has been rewritten, and it now carries the caveat:
**the script must be added to the submission bundle explicitly**, or the
commitment is satisfied inside the working tree and missed in the thing the
judges actually receive.

The files in question:

```
tools/action_recogntion/build_pill_detector.py        the pipeline
tools/action_recogntion/medsight_neuralart.json       the ST Edge AI profile
tools/action_recogntion/medsight_arena.mpool          the AI_ARENA pool
tools/action_recogntion/models/pill_detector/
    medsight_pill_yolov8n.pt                          trained weights
    pill_cut_int8_160.onnx                            the quantised graph
```

The collaborator's original `best.pt` / `best.onnx` stay beside them as the
provenance of the design, clearly not the shipped model.

---

## Addendum — the arena self-test was about to write over the model

Found while writing the hardware procedure, before any of it was run.

`ms_memtest_arena()` fills **all 225,280 bytes** of `AI_ARENA` with three
patterns and reads them back. Session 15 placed the call in
`state_machine_init()`, immediately after `ai_vision_wait_init()` returns —
which was exactly right when the arena was empty, because that wait is also
the precondition for AXISRAM5/6 being powered.

Session 16 put a model in the arena. `AI_FLAG_INIT_DONE` — the flag
`ai_vision_wait_init()` waits on — is now set *after* `intake_detect_init()`,
so by the time the self-test runs, the pill detector's **208,000 bytes of
activations** are sitting in the region it is about to overwrite.

**Is it actually harmful today? No — and that is the problem with leaving
it.** The generated pool declares `use4initializers=NO`, so nothing durable
lives there and activations are rewritten on every `stai_pill_run()`. The
overwrite would have been invisible and benign. But:

- "benign because of a flag in a generated comment" is not a property to rely
  on across a regeneration — flip `use4initializers` to `YES` and it becomes
  silent corruption of a model's constants, which would present as a detector
  that returns plausible nonsense;
- and a self-test whose whole claim is *"this memory is free and writable"*
  should not be run over memory that is now spoken for. The claim stops being
  true even when the test still passes.

**The fix keeps full coverage rather than shrinking the test.** There is
exactly one window where the whole arena can be exercised without touching a
live model: inside `task_ai_fn`, after `ai_vision_init()` has powered
AXISRAM5/6, and before `intake_detect_init()` claims the region. The call
moved there. With `MEDSIGHT_ACTION_RECOGNITION=0` nothing occupies the arena
and the call stays exactly where Session 15 put it, guarded by `#if`.

The boot log therefore shows the `ARENA SELFTEST PASS` line slightly earlier
than it used to — from the AI task rather than the UI task — and it should
still report the full **225280 bytes**. If it reports a smaller number,
something has changed about the linker region and that is worth stopping for.

**The general shape of this**, which this project keeps meeting: a check
written when a resource was unowned, left in place after the resource
acquired an owner. The same sentence describes Session 13's
`MASCOT_SAD_*` constant (`session_15_notes.md` 1b.1) — *a constant that
encodes a position is a constant that assumes a caller* — and it is worth
noticing that self-tests age the same way code does.

### Cut-path verification, after the self-test move

The `MEDSIGHT_ACTION_RECOGNITION=0` build was re-checked *after* the arena
self-test was relocated, because that change touched the `#if` guards on both
sides of the switch and the earlier check predated it.

With the feature off, Debug builds clean and the ELF contains:

| | Count |
|---|---|
| `stai_pill*` symbols (the 3 MB network) | **0** — fully dropped by the linker |
| `intake*` symbols | 2 — the empty stubs only |
| `ms_memtest_arena` | present, called from `state_machine_init()` as in Session 15 |

So the cut is real rather than nominal: the network does not merely go unused,
it does not get linked, and the arena self-test returns to exactly where and
when Session 15 ran it. `MASTER_PROJECT_PLAN.md` §3 lists Session 16 first in
the cut order, and this is what makes that a one-line change.

---

## Addendum 2 — the weights are flashed (and the project's own rule was bent to do it)

**Deviation, recorded rather than left silent.** `MASTER_PROJECT_PLAN.md` §4
step 4 says "**You** compile in STM32CubeIDE, flash the STM32N6570-DK, and
manually verify... Antigravity never touches hardware." That rule was
overridden by the project owner in this session, explicitly and directly: they
flashed the application from the IDE as usual and instructed the agent to
flash the weight blob. So the external-NOR write below was done by the agent
against a connected board. The rule stands for everything else.

### What was written

```
Board       : STM32N6570-DK      ST-LINK V3J17M10   SN 0038003A3235511038363730
Voltage     : 3.29V              SWD 8000 kHz       Hot Plug
Device      : ST32N657 Rev B     Cortex-M55
File        : pill_atonbuf.xSPI2.bin   2.91 MB      -> 0x73000000
Erased      : external sectors [768 814]
Result      : File download complete, 00:00:19.438
```

### Verified after the write

| Address | First word | Meaning |
|---|---|---|
| `0x73000000` | `03F6FFCA 1807E1FD` | pill weights, start — not blank |
| `0x732E8000` | `2F6E3FD3` | pill weights, near the end — not blank |
| `0x70380000` | `D0FB0F0D` | CenterFace pools — **intact** |
| `0x71000000` | `CA057A7A` | epoch-controller blobs — **intact** |
| `0x72000000` | `05F60309` | MobileFaceNet pools — **intact** |

The erase touched sectors 768–814 only, which is the `0x73000000` region. The
three pre-existing weight images are untouched, as the layout analysis said
they would be.

### One trap, now fixed in the tooling

**`STM32_Programmer_CLI` refuses a `.raw` extension outright**, and ST Edge AI
emits the weight blob as `.raw`:

```
Error: The download command you trying to perform (-w) is missing the filePath
to be loaded or it has a wrong extension, please note that the supported
extension are .bin, .hex, .srec, .s19, .elf, ...
```

Renaming a copy to `.bin` fixes it, and `.bin` is what the two face networks
already use (`fd_data.xSPI2.bin`, `faceid_data.xSPI2.bin`). Both files are now
kept side by side in `Debug/weights_flash/`, and
`build_pill_detector.py --generate` makes the `.bin` copy automatically so the
next person does not meet this at all.

### THE NEXT STEP IS A POWER CYCLE, AND IT IS NOT OPTIONAL

Five external-loader operations ran back to back here — one write and four
readbacks — with no power cycle between them. `AI_LESSONS.md` records exactly
this situation from Session 08B: the flash chip's live bus state gets confused
by consecutive loader operations, and the board then hangs at boot inside
`HAL_XSPI_GET_FLAG`'s polling loop, right where `aiPreInitialize()` sets up
memory-mapped mode. **The flash contents are fine** — it is the peripheral's
live state, and only a genuine power cycle clears it.

So: unplug the USB cable completely, wait a few seconds, plug it back in, and
only then run the application. This is also convenient, because the cold boot
is the `LPEN` test that Part B needs anyway (checklist item 6).

---

## Addendum 3 — first hardware round: boot is clean, three of four gates passed

The board booted with the pill detector flashed and the application built from
this session's tree. Full boot log in the session transcript; the parts that
matter:

### Gate 1 — the pill detector is alive on the NPU. PASSED.

```
intake: pill detector ready (YOLOv8n 160x160 INT8, head cut, decode on CPU).
```

This one line settles a lot at once: the 2.91 MB weight image at `0x73000000`
is readable by the NPU, `stai_pill_init()` succeeded, the input and output
descriptors were fetched, and the twelve quantisation parameters loaded. A
third network is running on this device alongside the two face models.

No `stai_pill_init failed`, no bus fault, no `Error: Epoch Controller binary is
invalid` — which is worth noting, because that assertion burned three separate
sessions in 08A/08B/12 and this network deliberately does not use the epoch
controller at all.

### Gate 2 — the arena self-test, in its new position. PASSED.

```
ai_vision_init: face detector + embedder ready.
ARENA SELFTEST: testing 0x34388000-0x343BEFFF (225280 bytes)...
ARENA SELFTEST PASS: 225280 bytes writable and readable (3 patterns, 27ms).
intake: pill detector ready ...
```

**Read the ordering, because it is the whole point of Addendum 1's fix.** The
self-test now sits *between* `ai_vision_init()` (which powers AXISRAM5/6) and
`intake_detect_init()` (which claims the arena). Full 225,280-byte coverage,
all three patterns, and not one byte of it written over a live model. 27 ms,
consistent with Session 15's 28 ms.

### Gate 3 — the two new LPEN bits. SET.

```
sleep clocks: BUSLPENR=00000003 MEMLPENR=0000000F APB5LPENR=00000046 AHB5LPENR=800020A2
```

`0x800020A2` decodes as:

| Bit | Signal | State | |
|---|---|---|---|
| 1 | `DMA2D` | SET | pre-existing |
| **5** | **`XSPI1`** | **SET** | **Session 16** |
| 7 | `SDMMC2` | SET | pre-existing |
| 12 | `XSPI2` | clear | latent, documented in Part B2 |
| **13** | **`XSPIM`** | **SET** | **Session 16** |
| 31 | `NPU` | SET | pre-existing |

No spurious bits (`other bits set: 0x00000000`). The PSRAM controller and the
XSPI manager will both keep their clocks through `WFI`.

**This is not yet the LPEN test.** Setting the bits is necessary, not
sufficient — the fault Addendum 9 describes only appears when a DMA master is
actually writing into the gated destination while the CPU sleeps. That is the
confirm screen with the camera streaming to PSRAM, and it has not been reached
yet. Gate 4 remains open.

### Gate 4 — the camera path. NOT YET EXERCISED.

No `intake: camera streaming to PSRAM 0x90400000` line, because the run did
not reach `STATE_CONFIRM_TAKEN`. See the blocker below.

### The blocker: the gallery is empty

```
SD_Read_File(patients.dat): 1732 bytes read.
gallery_init: loaded 0 patient(s) from patients.dat (format v3).
```

1,732 bytes is exactly right for v3 — a 12-byte header plus ten 172-byte
records — so the file is **valid and current, and simply contains no enrolled
patient.** Nothing is wrong; there is just nobody to dispense to, and the
dispense flow is the only route to the confirm screen where action recognition
runs.

Two things follow for the next round:

- **A patient must be registered first**, which needs the carer passcode. The
  log says `carer_pass: passcode loaded from carer.cfg (changed from the
  default)`, so the build-time default will not work — it is whatever the owner
  set. If that is lost, deleting `carer.cfg` from the card restores the
  build-time default.
- **The clock is not set** (`schedule: clock not set - no dose windows
  armed`), so a dispense will log the generic
  `CONFIRMED: <name> took pills (<verdict>)` rather than the
  `took the HH:MM dose (on time, <verdict>)` form. Both paths carry the
  verdict; only the scheduled one needs the clock. Setting it is optional for
  testing action recognition and required for demonstrating the two together.

---

## Addendum 4 — second hardware round: the pipeline runs, and it was aimed at the wrong place

The full flow ran end to end: carer mode, clock set, patient enrolled, face
matched at 91/100, dispense, confirm screen, button. Action recognition ran
and put its verdict in the audit log. And it detected nothing.

```
STATE_CONFIRM_TAKEN
intake: camera streaming to PSRAM 0x90400000 (UI keeps the framebuffer).
power: idle 81.9% of last 10026ms (9032 WFI entries)
intake: watched 117 frames (0 with a pill), verdict=gesture uncertain (monitoring).
CONFIRMED: DOUSIK took pills (gesture uncertain)
```

### What this round proves

- **Part B works.** The DCMIPP repointed to PSRAM, the UI kept drawing the
  confirm screen, and the display did not corrupt. The invariant Session 09
  established — one owner of `BUFFER_ADDRESS` — held.
- **The LPEN bits are doing their job.** 9,032 `WFI` entries at 81.9% idle
  *while the camera was DMAing into PSRAM*, with no visible fault. That is the
  condition Addendum 9's original bug needed, and it did not reproduce.
- **The detector is fast enough.** 117 inferences in roughly 8 real seconds is
  ~68 ms per frame, of which 33 ms is the deliberate sleep. So the NPU pass
  plus the CPU decode is on the order of **35 ms**, comfortably inside the
  ~30 fps the state machine's frame counts assume.
- **Part D holds.** The button confirmed the dose, the verdict was a suffix,
  and `gesture uncertain` is the correct answer for a machine that never left
  `SEARCHING`.
- **Face recognition is unaffected** by anything this session did: 209 ms,
  the same figure to the millisecond as Sessions 13 and 15, and 91/100 match.

### The bug: a coordinate space carried across a boundary

`ai_vision.c` runs CenterFace on a **480x480 centre crop** of the 800x480
frame, so `decode_best_face_box()` and therefore `decode_landmarks()` produce
coordinates in *hold-buffer* space, whose origin is `(160, 0)` in the frame.
`ai_vision_get_mouth()` handed those out unchanged, and
`intake_camera_grab_roi()` used them to index the full 800x480 PSRAM frame.

**The pill detector's ROI was therefore aimed 160 pixels to the left of the
patient's mouth for the entire watch.**

Two things make this worth writing down rather than just fixing:

- `ai_vision.h`'s own comment *documented* the hold-buffer space correctly —
  "coordinates are in the same space as the detector's own box" — and the
  consumer was written anyway as though they were frame coordinates. A
  correct comment is not a correct interface.
- `intake_service.c` carries a comment warning about precisely this class of
  error, for the *pill* coordinates: "getting this wrong would put every
  distance in the wrong units and every threshold out by the ROI scale factor
  — silently, since the numbers would still look plausible." The pill path was
  converted carefully; the mouth path, one layer up, was not.

**Fix:** `ms_face_landmarks_t` now records the crop origin, and
`ai_vision_get_mouth()` returns **full-frame** coordinates. The header says so
in capitals. The self-test path sets the origin to `(0, 0)`, since it feeds
the debug image at native size with no crop.

### The diagnostic gap, also fixed

`0 with a pill` was not enough to act on. It is equally consistent with three
completely different situations, and the round could not distinguish them:

1. nothing pill-like was actually held up;
2. the ROI was aimed at the wrong place (which it was);
3. the DCMIPP was not delivering frames to PSRAM at all.

The summary line now carries the evidence to separate them:

```
intake: watched N frames (M with a pill), best conf=X%, ROI WxH at (x,y),
        roi_mean A..B, verdict=... (...)
```

- **`roi_mean A..B`** — the mean green value of the grabbed ROI, min and max
  across the watch. Flat, 0 or 255 means the camera path is dead; a plausible
  varying number means frames are arriving.
- **`best conf=X%`** — the detector's highest confidence all watch, so "saw
  nothing at all" is distinguishable from "saw something at 20%".
- **`ROI WxH at (x,y)`** — where it was actually looking, which is what would
  have exposed this bug in one line.

Plus an explicit warning line when `roi_mean` is flat, so the next reader does
not need this file open to interpret their own log.

### `ai_vision_dump_landmarks()` was never called

It was written for the Part 0 verification the Definition of Done requires —
that the landmark tensor decodes to sane pixels and that the mouth really is
at indices 3 and 4 — declared, documented, and then never invoked from
anywhere. It now runs once per dose at the start of the watch, printing both
coordinate spaces:

```
landmarks (expect eye,eye,nose,mouth,mouth) crop-space -> frame-space,
crop origin (160,0), face_w=NNN:
  [0] crop(x,y) -> frame(x,y)
  ...
```

That is the check that gates every mouth-derived number in Stages 2 and 3, and
it had no way to run. **Still open** until a round produces those five lines.

---

## Addendum 5 — the project identity says 16 now

The session folder has been `sessions/session_16/` since the first hour, but
the **Eclipse project names and the build artefact** still said 15, inherited
from the copy. Every previous session carried the same inconsistency forward;
Session 09's prompt flagged fixing it as "optional, just for clarity when you
have multiple session folders' `.elf` files around", and it was never done.

It matters more now than it did then, for one practical reason: there are
sixteen session folders, several of them still buildable, and an `.elf` whose
name does not identify which folder produced it is a real way to flash the
wrong firmware and spend an evening confused about why a fix did not take.

Renamed, in the four files that own the identity:

| File | From | To |
|---|---|---|
| `.project` | `MedSight_Session15` | `MedSight_Session16` |
| `STM32CubeIDE/.project` | `MedSight_Session15_CubeIDE` | `MedSight_Session16_CubeIDE` |
| `STM32CubeIDE/FSBL/.project` | `MedSight_Session15_FSBL` | `MedSight_Session16_FSBL` |
| `STM32CubeIDE/FSBL/.cproject` | 7 references | updated |
| `…/MedSight_Session15_FSBL.launch` | 5 references | renamed to `…Session16…` |

**What was deliberately NOT changed.** Three source files match a grep for
`session_15` — `intake.h`, `ai_vision.h`, `intake_service.c` — and all three
are citations to `session_15_notes.md` in comments. Those are historical
references to a document that exists and will keep existing. A blind
tree-wide replace would have corrupted them into references to a document that
does not, which is the same family of mistake as `ENGINEERING_LESSONS.md`'s
"never `sed` the whole tree" rule (Session 09 corrupted `.o` files that way).
Each of the four identity files was edited by name.

The generated `Debug/`/`Release/` makefiles, `sources.mk`, `objects.list` and
the old-named `.elf`/`.map`/`.bin`/`.list` were deleted rather than edited —
they are 100% regenerated from `.project`/`.cproject`, and hand-editing them
is the thing `ENGINEERING_LESSONS.md` hard rule 2 exists to forbid.
`Debug/weights_flash/` was preserved, because the flashed pill weights are not
a build artefact.

**One consequence for whoever has the project open.** The Eclipse project name
changed, so an IDE that already has `MedSight_Session15_FSBL` imported is
holding a stale reference. Close/delete that project from the workspace (do
NOT tick "delete contents on disk") and re-import from
`sessions/session_16/STM32CubeIDE/FSBL`. The run configuration comes across
under its new name.

---

## Addendum 6 — third hardware round: the pipeline works, and it found a different bug

The coordinate fix landed and the whole three-stage pipeline ran on real
hardware, repeatedly. This round closes the last open Part 0 item and turns up
a problem that has nothing to do with Session 16.

### Part 0 is CLOSED: the landmark ordering is confirmed

Six captures, all consistent. A representative one:

```
landmarks (expect eye,eye,nose,mouth,mouth):
  [0] x=237 y=231     [1] x=309 y=227    <- eyes:  same y, 72 px apart
  [2] x=274 y=266                        <- nose:  centred, lower
  [3] x=243 y=304     [4] x=302 y=301    <- mouth: lower still, 59 px apart
```

Two points at one height, one below and between them, two more below that at a
narrower spacing. That is a face, and **the mouth corners really are at indices
3 and 4** — the conventional CenterFace order, which this session refused to
assume and has now measured. Every mouth-derived quantity in Stages 2 and 3
rests on this, and it is no longer an assumption.

The y-progression is worth keeping as a sanity rule for anyone who
regenerates the detector: **eyes ≈ 220-231, nose ≈ 257-266, mouth ≈ 294-304**,
in a 480-tall crop, for a face at normal standing distance.

### Stage 1A works, and the verdicts are RIGHT

Five watches, with the object detected in each:

| Frames watched | With a pill | Verdict | Reason |
|---|---|---|---|
| 19 | 12 | gesture NOT observed | pill reappeared in hand during retreat |
| 27 | 8 | gesture NOT observed | pill reappeared in hand during retreat |
| 132 | 16 | gesture uncertain | pill approaching mouth zone |
| 129 | 18 | gesture uncertain | pill lock acquired |
| 31 | 14 | gesture NOT observed | pill reappeared in hand during retreat |

**This reads like failure and is the opposite.** The test object was a
pill-sized household item, and nobody swallowed it — so it *always* reappeared
in the hand after approaching the mouth. `RETREATING_CHECK` saw a real detector
hit during the retreat window and correctly refused to certify an intake.

That is the entire point of the word "Guarded" in the collaborator's
`GuardedIntakeStateMachine`, and it is the behaviour that makes the feature
safe to have: a machine that had answered CONSUMED there would be the broken
one. The sequence detect → lock → approach → mouth zone → retreat → **decline**
ran end to end, on hardware, with a real object.

**What remains untested is the positive path**, and it cannot be tested without
something a person can actually swallow. Until then the honest claim is: the
device detects a pill-like object, tracks it to the mouth, and correctly
declines to confirm an intake that did not occur. Nobody has yet seen it
confirm one that did.

### The real finding, and it is NOT about action recognition

Face matching rejected the enrolled patient **one time in three**:

```
similarity, same person, same session, minutes apart:
  57, 63, 64, 67, 71, 72, 78, 83, 88     (threshold 65)
  median 71   spread 31   -> 3 of 9 rejected as INTRUDER
```

`session_12_notes.md` Addendum 3 flagged that the 0.65 threshold was inherited
from a reference project whose embeddings are **16-bit**, where this project's
are **int8**, and said it needed real measurement rather than a blind
adjustment. Session 15 then recorded a match that succeeded "by exactly zero
margin". This round is that warning arriving.

**The threshold was deliberately NOT changed.** Every score above is the
enrolled patient; there is not one impostor score in the dataset, so there is
no way to know what lowering the bar would cost in false acceptances — and on
a device that dispenses medication, guessing in that direction is the wrong
error to make. For the record, on this sample alone a threshold of 55 would
have rejected nobody, and 60 would have rejected one.

**The measurement that would settle it** is cheap and specific: with one
patient enrolled, have a *different* person tap DISPENSE five or six times and
record their similarity scores. If impostor scores cluster well below the
enrolled patient's minimum, the gap is real and the threshold can be moved with
evidence behind it. That is a ten-minute test and it belongs before the demo.

**Severity, stated fairly:** this is a usability problem, not a security hole.
`STATE_FACE_RETRY` rescued every single rejection on the second attempt — which
is Session 12's hardening doing its job — so the failure mode is "sometimes you
have to look at it twice", not "the device let the wrong person through".

### CORRECTION, from the project owner, immediately after this round

**The lighting was varying during the test.** The scores above were gathered
across a session in which the illumination on the subject's face changed, and
the enrolment capture was taken under different light again. That is a
sufficient explanation for a 31-point spread, and it means **the 33% figure is
a property of the test conditions, not of the device.** It should not be quoted
as a device characteristic, and the paragraphs above stand only as the record
of what was seen and why it was not acted on blindly.

What survives the correction, and is still worth carrying:

- **The pipeline is sensitive to an enrolment/dispense lighting mismatch.**
  That is an ordinary property of a one-shot face embedder, not a defect, but
  it is a real deployment consideration for a device that sits in a bedroom
  where the light changes through the day.
- **The mitigation is already on record** and is unchanged from Session 15's
  advice: enrol in the lighting the device will actually be used in — and, for
  the submission, in the lighting the demo will be filmed in.
- **The threshold question is unresolved rather than closed.** 0.65 is still
  inherited from a reference project with 16-bit embeddings
  (`session_12_notes.md` Addendum 3), and no impostor score has ever been
  measured on this device. Under controlled lighting the margin may be
  entirely adequate. That is now the thing to check, and it is cheap.

**Re-measure under steady light before drawing any conclusion from this
section.**

### Two smaller observations

**UART output interleaves between tasks.** One line came through as
`CONFIRMED: DOUSIK took pills (=gesture uncertain (STATE_HOME`, which is the
logger task's echo and the AI task's summary `printf` colliding on a port
neither locks. The SD card's own copy is written by a different path and is
almost certainly intact — **but that should be confirmed by reading
`events.log` off the card**, because the audit trail is the one artefact in
this device that has to be trustworthy. Recorded as open.

**Idle drops to 59-75% during a watch**, from 85-90% at rest. That is the
honest power cost of running the NPU every 33 ms for up to 30 seconds, and it
is the number to quote for the feature rather than the idle figure.

### A process failure of mine, recorded

The round before this one was wasted. Two `printf` replacements did not apply,
and I reported them as done because the verification was `grep | head` — which
matched the *tracking variables* and let me conclude the *printing* was there.
It was not, so the board ran the old diagnostics and produced a log that could
not answer the question it was flashed to answer.

Repairing it then corrupted the CRLF line endings in two files, and the
residue was a **bare `\r`** that GCC treats as a line break while `sed` and
`grep` do not show it at all — so the source looked correct and would not
compile. Both files are now verified structurally (balanced braces, all public
symbols present, zero stray control characters).

**The rule this earns**, and it is the same one `ENGINEERING_LESSONS.md`
already states for the `.elf` after a file restore: *verify the artefact, not
the source, and not the build log.* The check that finally worked was

```
strings <elf> | grep -E "intake: watched|landmarks \(expect"
```

which reads what is actually in the binary. Every earlier check in this episode
inspected something upstream of the thing that mattered.

---

## Addendum 7 — the ROI multiplier, decided by measurement rather than by feel

Before a round with GEMS-sized objects, the obvious question is whether the
detector can see something that small at all. It was measured rather than
guessed, offline, on the held-out validation set.

### The geometry, and the counter-intuitive part

The ROI is defined as a multiple of the **face width**, so the pill and the
face shrink together with distance. **Standing closer to the device does not
make the pill bigger to the detector.** The apparent size is fixed by one
ratio and nothing else:

```
pill_px  =  pill_diameter / (mult x face_width) x 160
```

For a 150 mm face and the 160 px detector input:

| object | | at mult 2.0 | at mult 1.0 |
|---|---|---|---|
| typical tablet | 10 mm | 5.3 px | 10.7 px |
| GEMS / small tablet | 12 mm | 6.4 px | 12.8 px |
| bottle cap (what was actually tested) | 28 mm | **14.9 px** | 29.9 px |

### The measurement

Validation images were progressively shrunk inside the 160 px input to
simulate a smaller object, and the detection rate recorded:

| shrink | pill px | detected | rate |
|---|---|---|---|
| 1.00 | 29.9 | 88/90 | 98% |
| 0.80 | 23.9 | 89/90 | 99% |
| 0.65 | 19.4 | **90/90** | **100%** |
| 0.50 | **14.9** | 85/90 | **94%** |
| 0.40 | 12.0 | 75/90 | 83% |
| 0.30 | 9.0 | 60/90 | 67% |

Three things fall out of this, and the first is a genuine check on the whole
line of reasoning:

1. **The 14.9 px row is the bottle cap**, and it predicts 94% — which is
   consistent with the hardware, where that object was detected on 8-18 frames
   of every watch. An independent offline measurement landing on the observed
   hardware behaviour is the strongest evidence available that the size model
   above is actually right.
2. **Degradation is graceful, not a cliff.** The earlier worry — that the
   deployment scale was hopelessly outside the training distribution — was
   overstated. Detection is still 83% at 12 px. Mosaic augmentation is the
   likely reason: tiling four images into one halves object size, so the model
   saw a great deal of small-object data even though the raw label statistics
   say 17-49 px.
3. **Detection PEAKS at 19.4 px (100%), better than at full scale.** Same
   cause. The model is, if anything, tuned for objects slightly smaller than
   its own dataset's median.

### The decision

`ROI_FACE_W_PCT` set to **100** (was 200). A GEM moves from 6.4 px — below the
bottom of the measured curve, and hopeless — to 12.8 px, where the measured
rate is roughly 83-90%.

**It was not set tighter**, and the reason is a hard constraint rather than
caution. Stage 3's thresholds are expressed in face widths:

```
AT_MOUTH triggers at   norm_dist < 0.15 face widths
RETREAT  triggers at   norm_dist > 0.35 face widths
```

The ROI must therefore span at least +/-0.35 face widths about the mouth — a
multiplier of 0.7 — or the pill leaves the crop before it is far enough away
for the retreat transition to fire, and `RETREATING_CHECK` can never complete.
A multiplier of 1.0 gives +/-0.5 face widths: the full range the state machine
reasons about, with margin.

### The caveat that must travel with these numbers

**This is a desktop measurement on clean dataset photographs.** The device sees
frames through its own ISP, with motion blur from a moving hand, a hand partly
occluding the object, and household lighting. The real rate will be lower than
83%. The curve establishes the *shape* of the relationship and justifies the
constant; it is not a prediction of on-device accuracy and must not be quoted
as one.

---

## Addendum 8 — tuning the ROI on the wrong variable, and the measurement that caught it

Addendum 7 measured the detector's tolerance for small objects and set
`ROI_FACE_W_PCT` from 200 to 100 on the strength of it. **That made the device
worse, by a lot**, and the reason is a variable that was never measured.

### What the board said

```
landmarks ... crop origin (160,0), face_w=159:
  [3] crop(224,291) -> frame(384,291)      <- mouth corners
  [4] crop(285,291) -> frame(445,291)
intake: watched 79 frames (1 with a pill), best conf=38%,
        ROI 159x159 at (335,212), roi_mean 43..147,
        verdict=gesture uncertain (monitoring)
```

Everything about the plumbing is right, and the new diagnostics prove it:

- **The ROI lands exactly where it should.** Mouth centre from the landmarks is
  frame (414, 291); ROI origin should be (414-79, 291-79) = **(335, 212)**, and
  that is precisely what was reported. The crop-to-frame coordinate fix from
  Addendum 4 is confirmed correct on hardware.
- **`roi_mean 43..147`** — frames are live, with real dynamic range. The camera
  path is not the problem.
- **`best conf=38%`** — the detector did see it, once, barely over the 0.35
  threshold.

And yet: **1 detection in 79 frames**, against 8-18 in 19-31 frames on the
previous build.

### The trade that was made without measuring both halves

| | field of view | object size | detections |
|---|---|---|---|
| mult 2.0 (bottle cap) | ±1.0 face widths | 14.9 px | 8-18 of 19-31 ≈ **50%** |
| mult 1.0 (GEM) | **±0.5 face widths** | 12.7 px | **1 of 79 ≈ 1.3%** |

Halving the multiplier doubled the resolution and **halved the field of view**.
A hand carrying a pill up to a mouth spends most of its travel more than half a
face width away, so for most frames the object was simply not inside the crop.

There is also a hard ceiling that Addendum 7 did not notice: **a 159 px ROI
into a 160 px input is already 1:1.** Tightening further would upsample and
gain nothing. 12.7 px is the maximum achievable size for a 12 mm object against
a 159 px face — the ratio `pill / (mult x face_width)` is all there is, which is
also why standing closer does not help.

### The number that exposes it

The current model, measured offline at the sizes the device actually produces:

| ROI mult | GEM px | offline rate | on hardware |
|---|---|---|---|
| 2.0 | 6.4 | 33% | — |
| 1.5 | 8.5 | 57% | — |
| 1.0 | 12.8 | **92%** | **1.3%** |

**Offline predicted 92%; hardware delivered 1.3% — a factor of 70.** The same
comparison for the bottle cap at mult 2.0 is 94% offline against ~50% on
hardware, a factor of 2. Occlusion, motion blur and the ISP explain a factor of
2. They do not explain 70. The difference between those two ratios is the
field-of-view loss, isolated.

### The lesson, which is a general one

**Addendum 7's curve measured pill SIZE, so pill size is what got optimised.**
The field of view was changed at the same time, by the same constant, and was
never measured at all — so a real improvement in the measured variable was
bought with a larger regression in the unmeasured one.

This is the same shape as `session_12_notes.md` Addendum 9's fifth hard rule
("change one variable per hardware run"), one level up: *when a single constant
moves two variables, measuring one of them is not measuring the change.*
`ROI_FACE_W_PCT` controls resolution **and** field of view, and only the first
had evidence behind it.

### What follows

The plan is now a measurable target rather than a preference:

- **Return the multiplier to 2.0**, the field of view that demonstrably worked.
- **Make the model work at 6.4 px**, where it currently manages 33%.
- **Acceptance bar: >=80% offline at 6.4 px**, on the grounds that hardware has
  been running at roughly half the offline rate once the object is actually in
  frame.

A retrain is running for exactly this: `imgsz=160` to match deployment, with
`scale=0.9` and `mosaic=1.0` so the model sees pills down to a few pixels
rather than the 17-49 px the raw dataset contains. The shipped model and its
flashed weights are untouched until that clears the bar.

---

## Addendum 9 — the state machine was proposed for removal, and kept

Recorded because this project's convention is to write down decisions that
went a particular way, including the ones that were nearly reversed.

**The proposal**, from the project owner: drop the staged machine entirely and
declare a pill taken whenever the pill coordinates and the mouth coordinates
coincide. "Why assume they go like taking pill and not. That's kinda overkill."

**The evidence against it came from this session's own hardware runs.** Three
separate watches ended with:

```
verdict=gesture NOT observed (pill reappeared in hand during retreat)
```

That is the object reaching the mouth and coming back down. Under a pure
coincidence rule, **all three would have been logged as
`CONFIRMED ... gesture confirmed`** — three false positives out of the three
relevant events actually observed on hardware. Approaching a pill to the mouth
and then withdrawing it is not an edge case; it is what people do when they
hesitate, reposition or change hands.

It also matters for what the submission claims.
`PROGRAM_PLAN_RECONCILIATION.md` §1 now rests part of its adherence argument on
"the camera saw a pill physically go to the patient's mouth". Backed by a rule
that cannot separate swallowing from holding, that sentence would be weaker
than it reads.

**A middle option was offered and declined**: keep the one guard, delete the
staging around it — pill seen in the mouth zone (any frame, no consecutive
run), then absent for N frames and not seen again. Two conditions instead of
five states, dropping the lock counter, the approach-velocity test and the
distance-trend tracking, and tolerant of a far less reliable detector.

**Decision: keep the full `GuardedIntakeStateMachine` as ported.** The owner's
call.

### The consequence, which is now load-bearing

`LOCK_FRAMES` requires **3 consecutive** direct detections, so the detector's
per-frame reliability is effectively cubed:

| per-frame rate | P(lock in a given triple) |
|---|---|
| 30% | 3% |
| 50% | 12% |
| 70% | 34% |
| 80% | 51% |
| 90% | 73% |

This is why the retrain's acceptance bar is **>=80% at 6.4 px** and not a
rounder, friendlier number. Keeping the staged machine means a detector that
merely improves is not sufficient; it has to be reliable enough that three
consecutive hits are ordinary. A retrain landing at 60-70% would look like a
large improvement and still leave the machine stuck in `SEARCHING`.

The collaborator could afford `LOCK_FRAMES = 3` because their detector ran on a
tight crop around a MediaPipe hand-pinch point and fired on nearly every frame.
That reliability is the assumption the constant carries, and it did not come
across with the port.

---

## Addendum 10 — how many pills, actually? The literature, and what it says about this design

Asked by the project owner, on the suspicion that the data model and the intake
model disagree: the device collects a **pill count per dose**, while the intake
machine appeared to confirm only a single pill.

### First, the premise was half wrong

`GuardedIntakeStateMachine` **does** handle multiple pills.
`intake_fsm_reset(pills_required)` takes the count straight from
`PatientRecord.pill_count`; the machine counts each completed intake, holds for
`REARM_FRAMES` (~2.5 s at 30 fps), re-arms, and reports `ALL_CONSUMED` only
when the count is reached. Multi-pill is implemented and ported faithfully.

**What is untested is whether it works.** At the detection rates measured so
far the device cannot reliably confirm even one intake, so a 3-pill dose has
never been exercised end to end. That is a gap in evidence, not in the code,
and it is listed with the other open items.

### The figures

| | |
|---|---|
| Median regular medications per care-home resident per day | **8** |
| Residents meeting polypharmacy (>=5 medications) | **78-86%** |
| Medication administration rounds per day | **~4** (3.66 where consolidated) |
| Pills (tablets) per sitting | **not reported** — see the caveat |

### What this validates

**`MAX_DOSE_TIMES = 4`.** `ai_vision.h` justified four slots as "what real
prescriptions use... a decision, not a limit that was reached for". Care-home
practice reports four administration rounds a day as the norm. The comment now
carries the citations instead of asserting.

**`MECHANICAL_DESIGN.md`'s 6-8 hoppers.** That range was chosen long before
anyone looked up a number, and it brackets the measured median of 8 almost
exactly. Worth recording as a case where the design intuition was right.

### What this exposes, and it is the more useful half

The mismatch the owner sensed is real, but it is not in the intake model — it
is in the **hopper**. This prototype holds ONE medication, and `pill_count`
means "how many tablets of that one drug". A resident taking a median of eight
different medications is served, by this device, for one of them.

That is already documented as a deliberate prototype cut
(`MASTER_PROJECT_PLAN.md` §6), but it had never been quantified, and the
adherence chain in `PROGRAM_PLAN_RECONCILIATION.md` §1 could be read as
covering a whole regimen. §1 now states the ratio explicitly. A judge should
not have to infer it.

### The caveat that must travel with every one of these numbers

**Every study counts MEDICATIONS, not TABLETS.** One medication can be two
tablets (2 x 500 mg), so pills-per-sitting is >= medications-per-sitting, and
none of these sources reports the tablet figure directly. The defensible
sentence is "a median of 8 regular *medications* a day". **Do not write "8
pills a day"** in submission material — it is a different quantity, larger,
and unsupported by the citation it would appear to rest on.

For the same reason no number was added for "pills per dose". Reasoning from 8
medications over 4 rounds would give ~2, but chronic medications cluster in the
morning round, so the true distribution is skewed and none of these papers
measures it. `pill_count`'s 1-10 range is left as it is: comfortably above any
plausible single-drug dose, and not pretending to a precision the evidence
does not support.

### Sources

- *Regular Medications Administered to Older Adults in Aged Care Facilities: A
  Retrospective Descriptive Study* — median 8; 78% polypharmacy.
  https://www.ncbi.nlm.nih.gov/pmc/articles/PMC12666757/
- *Daily Medication Use in Nursing Home Residents with Advanced Dementia* —
  slightly over 8 oral medications/day, nationally representative US sample.
  https://pmc.ncbi.nlm.nih.gov/articles/PMC2910133/
- *Medication burden attributable to chronic co-morbid conditions in the very
  old and vulnerable*. https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5912775/
- *Comparing nursing medication rounds before and after implementation of
  automated dispensing cabinets: a time and motion study* — ~4 rounds/day.
  https://pmc.ncbi.nlm.nih.gov/articles/PMC11416501/
- *Prescribing in the Nursing Facility* (AAFP FPM, 2024).
  https://www.aafp.org/pubs/fpm/issues/2024/0300/nursing-home-prescribing.html

---

## Addendum 11 — the small-object retrain: better, short of its bar, shipped anyway

Addendum 8 set a target: the detector must work at **6.4 px**, where the
shipped model managed 33%, with an acceptance bar of **>=80% offline**. A model
was retrained for exactly that — `imgsz=160` to match deployment, `scale=0.9`
and `mosaic=1.0` so it sees pills at a few pixels rather than the 17-49 px the
raw dataset contains. 90 epochs, best mAP50 **0.971**, which matches the
shipped model's 0.972 on a considerably harder task.

### The result

| px | what it is | shipped | retrained |
|---|---|---|---|
| 5.3 | 10 mm tablet @ ROI 2.0x | 14% | **38%** |
| 6.4 | 12 mm GEM @ ROI 2.0x — the target | 33% | **56%** — short of 80% |
| 8.5 | 12 mm GEM @ ROI 1.5x | 57% | **72%** |
| 12.8 | 12 mm GEM @ ROI 1.0x | 92% | 94% |
| 14.9 | 28 mm cap @ ROI 2.0x | 98% | 96% |
| 29.9 | 28 mm cap @ ROI 1.0x | 98% | 98% |

**It missed the bar and that is reported rather than quietly rounded up.**
Strictly better where it was weak, unchanged where it was already fine, and no
regression anywhere that matters.

### A correction to Addendum 9's arithmetic

Addendum 9 argued that `LOCK_FRAMES = 3` cubes the detection rate, so a 50%
detector locks only 12% of the time. **That is the per-TRIPLE probability, and
a watch contains dozens of triples.** The cumulative figure is what matters:

| per-frame rate | lock within 30 frames | within 100 frames |
|---|---|---|
| 15% | 8% | 25% |
| 28% | 37% | **81%** |
| 36% | 62% | **96%** |
| 50% | 90% | 100% |

The hardware said so before the arithmetic did: the bottle-cap watches reached
`RETREATING_CHECK`, which is only reachable *through* a lock. Locking was never
the blocker. Addendum 9's framing overstated it, and the conclusion it fed —
that the retrain had to clear 80% for the machine to be usable — was wrong.
**The bar was too strict**, which is the direction to err but is still an error.

### The decision, with both variables measured this time

**Ship the retrained model, and set `ROI_FACE_W_PCT` to 150.**

| | field of view | 12 mm object | expected on hardware |
|---|---|---|---|
| 200 | ±1.0 face widths | 6.4 px, 56% | FOV proven good; detection marginal |
| **150** | **±0.75** | **8.5 px, 72%** | **~36% -> ~96% chance of locking per watch** |
| 100 | ±0.5 | 12.8 px, 94% | FOV proven BAD — 1-3 detections per watch |

150 is the first value chosen with evidence on **both** halves of what this
constant controls, which is the whole lesson of Addendum 8. Field of view keeps
double the margin Stage 3's `RETREAT` transition needs, and detection is good
enough that locking is near-certain over a normal watch.

### The limit that no amount of retraining removes

A true 10-12 mm pill renders at 5-9 px at conversational distance, and that is
**geometry**: the ratio is `pill / (mult x face_width)`, so it is independent of
how close the patient stands. The retrain roughly doubled the detection rate at
those sizes and could not change the sizes themselves.

For a dependable demonstration, a ~20 mm swallowable object sits at ~14 px and
~95%. This should be said plainly in submission material rather than implied
away: **the pipeline is demonstrated, and the smallest real tablets are at the
edge of what a 160 px input resolves.** Raising that would mean a larger
detector input, which does not fit `AI_ARENA` (192 px needs 267,264 bytes
against 225,280) and would have to go to PSRAM at a latency cost.

---

## Addendum 12 — the AI overlay: showing the models instead of describing them

Requested by the project owner: draw the models' output on the LCD, the way a
pose-estimation demo does, so the AI is visible rather than merely logged.

Everything this project claims about its AI is, on the device itself,
invisible. A face is matched, a pill is tracked, a verdict is written — and a
person watching sees a screen change. The log knows; the viewer does not. For a
contest submission judged partly on demonstrating that the NPU is doing real
work, that is a gap worth closing.

`FSBL/Src/ui/ai_overlay.c` + `Inc/ui/ai_overlay.h`, behind `MEDSIGHT_AI_OVERLAY`
(default 1).

### Two views, with genuinely different constraints

**1. "What the face detector saw"** — on the DISPENSING screen, immediately
after a match. It renders the **frozen crop the NPU actually ran on** rather
than a fresh camera frame, so the box and the landmarks align with the image by
construction rather than by luck. Drawn on it:

- the detector's chosen face box, green
- all five CenterFace landmarks — eyes and nose amber, **the two mouth corners
  in rose**, because that pair is the half this project consumes
- the confidence

**2. The live intake view** — on the confirm screen, refreshing at ~8 fps:

- the live camera frame out of PSRAM
- **the ROI the detector actually searched**, amber
- the mouth position from the landmarks, rose
- the pill box when the detector has one, green
- the state machine's live state and the frame counts

### Why view 2 is possible at all, and why it is a consequence rather than luck

The device has never been able to draw anything over a camera image.
`session_09_notes.md`'s "Bug 2" is the record of the last attempt: with one
framebuffer shared by the DCMIPP and the UI, the camera erased every drawn
pixel at ~30 fps, and every screen after a capture has been static UI ever
since.

Two Session 16 decisions removed that constraint, neither taken with an overlay
in mind:

- **the camera streams to PSRAM** (`0x90400000`), not to `BUFFER_ADDRESS`, so
  the DCMIPP no longer competes with the UI for the framebuffer;
- **the pill detector's activations live in `AI_ARENA`** (`0x34388000`), chosen
  because it overlaps nothing the display uses — unlike the two face networks,
  whose activation scratch ST's codegen hardcodes on top of `BUFFER_ADDRESS`.

So during an intake watch the UI owns the framebuffer outright and can render
while the NPU is mid-inference. **The face view still has to wait** for the
capture handshake, because those two networks really do clobber the display.
That asymmetry is the only subtle thing in the module and it is stated at the
top of the header.

### Cost, and what was deliberately not used

A plain CPU nearest-neighbour scale into a 300x180 window — about 54,000 pixel
writes per refresh, throttled to ~8 fps, comfortably inside the UI task's 10 ms
tick. Measured build cost: **+0.4% ROM, no measurable RAM**, because it borrows
`gui_draw`'s primitives instead of allocating a buffer.

**DMA2D was deliberately not used**, though it would be faster. It is a bus
master, and adding one to a path that runs while the CPU may sleep drags in the
whole `LPEN` question (`session_12_notes.md` Addendum 9) for what is a cosmetic
feature. A CPU loop costs nothing but cycles this device has spare.

### The bug it walked into, which was already written down

`ai_overlay.c` went into `FSBL/Src/ui/`, and the build failed at link with
three undefined references. `Src/ai/` is a **linked folder** in `.project`, so
new files there are compiled automatically; `Src/ui/` is mapped by **per-file
`<link>` entries**, so a new file there is invisible to the IDE until one is
added. This is Session 09's Bug 1 exactly — and Addendum 4 of this very
document states the distinction between those two directories. Writing a
lesson down is not the same as remembering it at the moment it applies.

### What it is really for

The amber ROI rectangle is the one to watch. **Seeing where the detector was
actually looking would have made Addendum 8's field-of-view mistake obvious in
one glance**, instead of costing two hardware rounds and an offline shrink
curve to diagnose. The overlay was asked for as a demonstration feature; it is
at least as valuable as an instrument.

---

## Addendum 13 — the overlay drew nothing, and the reason was six missing lines

The face overlay was written, wired, called from both capture states, and drew
zero pixels on hardware. Not intermittently — never.

`ai_vision_get_capture_view()` opens with

```c
if ((v == NULL) || !s_last_box_valid || !s_last_landmarks.valid) return false;
```

and `s_last_box_valid` was **declared and never assigned anywhere in the file**.
So the getter took its early-out on every call, `ai_overlay_draw_capture()`
returned `false`, and the caller carried on exactly as designed — because
"nothing to show" is a legitimate outcome that must not log about itself.

Two properties combined to make this invisible:

* A variable that is only ever *read* is not a compiler warning. It has an
  initialiser — zero — and zero is a valid value of its type.
* A feature whose failure mode is "returns false politely" produces no
  evidence at all. Every other subsystem in this project announces its
  failures; this one was specified not to.

This is the same shape as the intake publish block that went missing earlier in
the session: the consumer was written carefully and the producer was never
written at all. **When a feature has a "there is nothing to show" path, make
the once-per-use summary count how often it took it.** `ai_overlay_stats()`
now reports `drawn / skipped_inactive / skipped_invalid`, which turns "no
overlay appeared" from an impression into a number.

---

## Addendum 14 — three camera flashes, three different causes

Between the composed preview and the result screen the panel showed a frame of
garbage. It was fixed three times, because it was three faults that happened to
look identical.

**1. The NPU's tensors, shown directly.** Both face networks' activation
scratch is hardcoded by ST's codegen to overlap `BUFFER_ADDRESS`. At the
instant a capture completes, the framebuffer literally *contains* tensors.
The first result screen unblanked the panel and then drew a 336×252 pane into
one corner of it, so the user saw their own face in the pane and raw tensor
data everywhere else. Fixed by making the result screen own the whole screen:
full repaint, product chrome, then draw.

> After a capture, the only safe thing to do with the framebuffer is write
> **all** of it.

**2. Unblanking before drawing.** Both success paths called
`display_blank(false)` and *then* drew. One frame of tensors is still one
frame. The panel now stays dark until `STATE_CAPTURE_RESULT` has finished
drawing. Draw first, light second.

**3. The safety unblank, firing during the settle window.** This one was
subtle. `state_machine_update()` ends with

```c
if (s_display_blanked && !s_cap_in_flight && state_init_done)
    display_blank(false);
```

and during `CAP_SETTLE` — the 150 ms in which the camera is repointed at
`BUFFER_ADDRESS` so the NPU gets a fresh frame — *every one of those
conditions is true*. The capture has not started, so `s_cap_in_flight` is
false. The camera state was entered long ago, so `state_init_done` is true.
And the DCMIPP is writing all 800×480. The guard dutifully turned the panel on
and showed it.

The general lesson is about what that condition was *trying* to say. It means
"is anything other than the UI writing the framebuffer right now?", and
`s_cap_in_flight` only ever covered the NPU half of that. The camera half
arrived with the live preview and had to be added by hand. **A predicate that
stands for a concept rather than a fact goes stale silently when the concept
grows.**

---

## Addendum 15 — the pill detector was never going to work, and the geometry said so

Five consecutive doses with the retrained detector running:

| dose | frames | with a pill | best conf | verdict |
|---|---|---|---|---|
| 1 | 54 | 2 | 58% | pill reached mouth |
| 2 | 420 | 53 | 74% | uncertain |
| 3 | 243 | 6 | 50% | uncertain |
| 4 | 121 | 0 | 28% | monitoring |
| 5 | 210 | 5 | 64% | uncertain |

with the box frequently on wall texture rather than on the object. The
hard-negative retrain improved every offline number — mAP50 0.958 → 0.974, and
detection at the deployment scale 57% → 79% — and changed none of this.

The reason is geometry, and it was knowable before any of the training:

```
ROI = 1.5 × face width ≈ 225 mm across a 160 px input = 0.71 px/mm

    a 12 mm tablet   →   8.5 px
    a hand           →  ~64 px
```

We were asking a detector to find an **eight pixel** object in a frame whose
mean luminance measured 63..78, while the hand carrying it was seven times
larger in each dimension and about fifty times larger in area. At 8 px a
tablet and a patch of wall grain are not reliably different things.

**What the field actually does.** Medication-adherence systems do not detect
the pill. They decompose the act into mini-activities — hand-to-mouth,
pill-into-mouth, hand-off-mouth — and detect the *hand* and the *mouth*
(AiCure, US10402982 and family). The smartwatch literature detects the same
gesture from wrist accelerometry and never sees a pill at all (JMIR Hum
Factors 2023;10:e42714). Nobody resolves a 10 mm tablet at conversational
distance, because it cannot be done.

The correction is not "train harder". It is **detect a different object**. Two
sessions of effort went into improving a signal whose ceiling was set by pixel
count, and the arithmetic that showed the ceiling is three lines long. Do that
arithmetic first.

The detector was **demoted, not deleted** — see Addendum 17.

---

## Addendum 16 — Stage 1C, and the three ways it failed before it worked

Stage 1C finds the hand: no model, no NPU, a 40×40 decimation of the crop
Stage 1A already has in hand. It took four hardware rounds, and each failure
was instructive in a different way.

**Attempt 1 — the noise centroid.** Frame-difference the ROI, mask to skin
tone, take the centroid. On hardware it **confirmed a dose on the second frame
of the watch**, from sensor noise. The reason is worth stating as a rule:

> The centroid of scattered noise sits at the centre of the ROI, and the ROI
> is centred on the mouth.

So noise did not merely produce a false detection — it produced one *at
exactly the position that means "the pill went in"*. A stage whose failure
mode is indistinguishable from its success case is not a detector. No
threshold on **count** can fix it, because noise and a hand can cover the same
number of cells.

**Attempt 2 — the density gate that rejected the signal.** Adding a
shape test (lit cells as a fraction of their bounding box) killed the noise
and also found a hand in **0 of 316 frames**, while reporting healthy masks:
mean skin 21%, mean motion 7%. The cause is a property of frame differencing
that is easy to forget:

> A smooth object in motion only lights up its own edges.

A moving hand's interior is hand-in-frame-N and hand-in-frame-N+1, so it
differences to zero. What survives is a thin arc — large box, few cells —
which is, on the two measurements the gate looks at, the same shape as
scattered noise. The gate was not wrong; the signal was.

Fixed by subtracting a **baseline** instead of the previous frame. Against a
still reference the whole *area* of the hand differs. It also disposes of the
face for free — the face is *in* the baseline, along with the wall and the
room — so what remains is what **arrived**, which is the question being asked.

**Attempt 3 — the exposure flood.** `flood = 418 of 429 frames`, with
`roi_mean` swinging **85..127 inside a single watch**. The ISP hunts hard
during a watch, partly *because* a hand entering changes what it is metering,
and a uniform brightness change makes every skin cell differ from the baseline
at once. Two fixes:

* **Illumination-invariant differencing.** A uniform change adds the same
  offset to every cell, so subtracting the difference of the two frame means
  removes it exactly and leaves what changed locally. Two sums over 1600 bytes.
* **Largest connected region, not the bounding box of everything lit.** This
  was the deeper error: a hand plus a few stray cells in the far corner
  produces a box spanning the whole grid, and the hand is then judged — and
  rejected — on the stray cells' geometry rather than its own. A flood fill
  asks the question actually meant: *is there **one** region big and solid
  enough to be a hand?* Scattered noise makes many tiny components and no
  large one. `MAX_SPAN` was deleted; it had been a proxy for "is this one
  object?", and the component test answers that directly.

**The result, on hardware:**

| | before | after |
|---|---|---|
| `flood` rejections | 418 / 429 | 0, 0, 0 |
| `sparse` rejections | — | 0, 0, 0 |
| hand detected | 0 / 316 | 232/376, 69/258, 30/88 |
| best blob | 460 cells @ 28% | 427 @ 46%, 226 @ 45%, 324 @ 41% |

with a genuine `pill reached mouth (all prescribed pills consumed)` verdict.

**What actually shortened this.** Each round the diagnostic line was made
*more specific*, and each time it named the next fault directly. The version
that reported only "hand seen in 0 of 316 frames (mean skin 21%, mean motion
7%)" cost a round, because it could not distinguish "the blob was too small"
from "the blob was too sparse". The version that reported
`rejected small=4 sparse=0 flood=418` answered the question in one line.

> A diagnostic that reports *whether* something failed buys one round. A
> diagnostic that reports *which gate* rejected buys the fix.

---

## Addendum 17 — the pill detector, demoted rather than deleted

Stage 1A still runs, on **one frame in four**. It is corroboration now: when
it fires alongside the hand it lifts confidence by at most a quarter, and it
can strengthen a detection but never create one.

Three reasons to keep it rather than rip it out:

1. Its box still draws on the overlay, so "a YOLOv8n runs on the Neural-ART
   NPU" stays true and demonstrable rather than becoming a claim about a
   deleted file.
2. The three-stage pipeline and the state machine came from the project
   owner's collaborator and are the valuable part of that contribution. Stages
   2 and 3 were written against *one tracked object* and do not care what it
   is — feeding them the hand instead of the pill changed the call site by
   five lines and left every threshold and transition exactly as ported.
3. It cost real work — the RF100 CC BY retrain, the DFL head cut, the memory
   pool, the flash procedure — and none of that is invalidated by being a
   second opinion instead of the verdict.

**The power figure came from the NPU, not the overlay.** Idle measured 42%
during a watch against 88% either side, and it stayed at 42% on a run where
the overlay drew *once*. Two rounds were spent shrinking and throttling the
overlay on the assumption that drawing was the cost. It never was. Sampling
Stage 1A at 4 Hz instead of 30 Hz took idle to 59–78%.

> Before optimising a suspected cost, find a run where the suspect did not
> happen. If the number is unchanged, the suspect is innocent.

---

## Addendum 18 — one camera panel, three moments

The device now shows the camera on three screens: the live face preview, the
capture result, and the intake watch. They began as three unrelated
rectangles, and the complaint that surfaced it was *"not sure if it's the
border, or the shape, or the resolution"* — which was the correct diagnosis,
because it was all three.

The intake pane mapped the whole 800×480 frame into a 300×300 window. Against
a square that is an **anamorphic squash** — 800 across compressed into the same
300 as 480 down — from a wider field of view, at half the vertical resolution
because a `PV_ROW_STEP` of 2 was skipping every other row to save PSRAM
bandwidth. Three differences at once, none of them individually nameable.

All three now share `CAPRES_IMG_*`: the same 300×300 rectangle at the same
position, the same centred-square crop, the same anti-aliased rounded border,
the same caption slot and font. `CONFIRM_PV_*` is *defined as* `CAPRES_IMG_*`
rather than repeating the numbers, so they cannot drift apart later.

Three smaller things worth recording:

**The live preview exists because the camera can write somewhere else.** The
seconds before a capture used to be a raw camera dump — the DCMIPP wrote all
800×480 continuously, so no chrome, no title and no text could survive a
single frame. Pointing the camera at PSRAM instead (the trick the intake watch
already proved) lets the UI own `BUFFER_ADDRESS` and compose a real screen.
The cost is a handover: the capture reads `BUFFER_ADDRESS`, so `preview_end()`
must repoint the pipe and the caller must let a whole frame land before
requesting a capture. That is `PREVIEW_SETTLE_MS`, and it is not optional.

**`gui_stroke_round_rect()` is right for buttons and wrong next to a
photograph.** It picks one integer inset per row and fills a hard-edged span.
Beside a camera image the eye has a smooth reference to compare against and
the staircase is the first thing it sees. `aa_border()` draws the same ring
with 4×4 supersampled coverage, blending against the *destination* so one
function sits correctly on the page colour outside and on the image inside.
Only the four corner boxes need it, so it is ~43k samples once per screen.

**Name entry moved before the face scan.** The small reason is that the camera
screen can then greet the patient by name. The larger one is that being
photographed by a machine that has not yet asked who you are is the wrong way
round; asking first makes the scan something done *with* the patient rather
than *to* them. `registration_ui_reset()` moved to the start of the flow —
left where it was, it would have wiped the name that had just been typed.

---

## Addendum 19 — what is still open

Recorded honestly rather than left to be rediscovered.

* **Multi-pill is NOT an open item, and listing it as one was wrong.** A hand
  carries the whole dose in one motion — that is how people take medication —
  so one hand-to-mouth settles the dose whatever the prescription says
  (Addendum 21). There is no per-pill count left to test and no code path that
  varies with `pills_required`. Doses of 3, 4 and 5 have all been run on
  hardware and behave identically, which is the point rather than a
  coincidence.
* **Idle during a watch is not a caveat on the power figure.** It was listed
  as one, and the arithmetic says otherwise: a watch is at most 30 s on one
  screen, so four doses a day is 120 s out of 86,400 — 0.14 % — which moves a
  day-average idle of 88.0 % to 87.9 %. `AI_PIPELINE.md` §9 now states both
  numbers with their duty cycle instead of an apology. The thing that would
  make it a real cost is a watch that runs when no one is being dispensed to,
  which `intake_end()` and the 30 s timeout exist to prevent.
* **The gesture concluded on 1 dose in 3.** Rounds that saw the hand in 232
  and 69 frames still ended `uncertain`, because they never got three
  consecutive frames inside the mouth zone. That may be honest — the gesture
  genuinely was not completed — or `MOUTH_ZONE_NORM_DIST` (0.15 face widths)
  may be tight. It should not be loosened without evidence: that constant is
  exactly the one whose relaxation would start confirming things that did not
  happen.
* **Idle is 59–78% during a watch**, against 88% either side. The remaining
  cost is the per-frame ROI grab from PSRAM, not the NPU.
* **A cup may be an easier target than either.** Recorded as a hypothesis in
  `AI_PIPELINE.md` §8, not as a plan: a cup is 50-64 px at the deployment
  ROI (same regime as a hand, 6-7x a pill), it is a rigid manufactured shape
  rather than an articulated one, `cup` is already a COCO class so no dataset
  or licensing work is needed, and it would need no change to Stages 2 or 3.
  It would also remove Stage 1C's skin-tone dependency, which is the item
  immediately below. The case against is there too.
* **~~Stage 1C's constants are set from one person~~ — CLOSED.** This was
  written when Stage 1C was a skin-tone heuristic, whose thresholds really
  were tuned to one person's skin under one light. That stage is gone: the
  hand model (Addendum 23) has no skin ratios and no per-subject constants at
  all — it was trained on MediaPipe's own dataset, whose model card documents
  a fairness evaluation across skin tones. The project owner has since
  confirmed correct behaviour on a second subject.

  What remains subject-independent by construction: `PRESENCE_T` is a
  threshold on the model's own confidence, not on any property of the person,
  and the geometry is expressed in FACE WIDTHS, which normalise across face
  size and standing distance. The per-dose diagnostic stays useful, but it is
  no longer guarding a known weakness.

---

## Addendum 20 — the mouth zone, recalibrated for a different object

Addendum 19 listed "concluded on 1 dose in 3" as open and counselled against
loosening `MOUTH_ZONE_NORM_DIST` without evidence. The project owner called
for loosening it, and on inspection they were right for a better reason than
either of us gave at the time.

0.15 face widths is the collaborator's number and it was correct **for their
signal**. They tracked a PILL, and a pill's centroid *is* the thing entering
the mouth: 0.15 of a 150 mm face is about 22 mm, roughly a mouth's half-width,
so "the pill is at the lips" and "the centroid is within 0.15" are the same
statement.

Stage 1C tracks a HAND. A hand's centroid is not at the lips when the pill is
— it is the middle of a ~90 mm blob whose *fingertips* reach the mouth, so at
the moment of delivery it sits 40-60 mm away, two to three times the old
threshold. Holding 0.15 does not make the machine stricter about a real
gesture; it makes a correctly completed gesture **unreachable**. That is
exactly what the hardware showed: the hand seen in 232 and 69 frames, verdict
still `uncertain`, because the zone was never entered.

So this is a **recalibration for a different tracked object, not a relaxation
of a safety margin**. The guards against false confirmation are elsewhere and
are untouched: `SIMPLE_HOLD` (three consecutive frames in the zone) and Stage
1C's own size and density gates. Set to 0.30 (~45 mm, about a hand's
half-width). If this is ever pointed back at a pill-sized object, put it back.

`RETREAT_NORM_DIST` moved 0.350 → 0.550 with it. The two are a hysteresis
pair, and their gap is the margin that stops a jittering centroid oscillating
between states. Widening the entry to 0.30 without moving the exit would have
cut the gap from 0.20 to 0.05, so a hand wobbling by 7 mm would have produced
a CONSUMED/RETREATED flutter. Simple mode has no retreat check and would never
have read it — but wrong is wrong whether or not today's build looks.

**The general point.** A constant inherited with an algorithm carries the
assumptions of the signal it was tuned on. When the signal changes, every
constant expressed in units of that signal has to be re-derived, not kept out
of respect for its source. Stages 2 and 3 were reused wholesale and correctly;
this was the one number in them that could not survive the substitution.

### Also in this pass

The build-time default carer passcode changed from `1379` to `1234`
(`Inc/ui/carer_ui.h`, and both references in `README.md`). It matters because
the SD card is wiped before shipping, which deletes `carer.cfg` and returns
the device to the build-time default — so the default is what a fresh unit
actually ships with, not a fallback nobody meets.

---

## Addendum 21 — one gesture is the whole dose

The FSM counted pills: each entry into the mouth zone incremented
`pills_consumed`, and `ALL_CONSUMED` waited for it to reach `pills_required`.
A three-pill prescription needed three separate hand-to-mouth trips.

The project owner pointed out that this is not how people take medication —
they tip the whole dose into a palm and take it in one motion. That is right,
and it exposes a second problem underneath it that is worse than the
inconvenience.

**The behaviour was wrong.** Requiring three trips makes the *normal* way of
taking medication unreachable. The verdict would sit at `uncertain` forever
for a patient doing exactly the right thing — the same shape of failure the
mouth zone had before Addendum 20, where a correctly completed gesture could
not reach the threshold that recognised it.

**The count was never measured.** Stage 1C tracks a HAND. A hand carrying
three tablets and a hand carrying one are the same object to it. Incrementing
a pill counter per trip counts TRIPS and reports them as PILLS — inventing
precision the device does not have and writing it into a medication record.
The counter existed because the collaborator's design tracked individual
pills, and like `MOUTH_ZONE_NORM_DIST` it did not survive the change of
tracked object. Addendum 20's closing point applies again, and this is the
second instance of it in two days:

> A constant — or a rule — inherited with an algorithm carries the
> assumptions of the signal it was tuned on.

`pills_consumed` is now *set* to `pills_required` rather than incremented,
because the honest statement is "the dose was observed", not "n pills were
counted".

### The audit line now names what was actually seen

Related, and found while making the change above: the simple-mode verdict
string was `"pill reached mouth"`, written when Stage 1A was the tracked
object. Since Stage 1C it usually is not — the pill detector only
corroborates, and often does not fire at all. Writing "pill reached mouth"
from a hand observation is an unearned word in a medication record: the device
did not see a pill, it saw a hand.

`pill_obs_t` and `intake_result_t` gained a `corroborated` flag, set by the
service when Stage 1A saw a pill in the SAME frame Stage 1C saw the hand. The
suffix is now:

| observation | audit suffix |
|---|---|
| hand at mouth, pill detector agreed in that frame | `pill reached mouth` |
| hand at mouth, no pill seen | `hand reached mouth` |

`R_ALL_CONSUMED`'s reason text changed from "all prescribed pills consumed" to
"dose observed in one hand-to-mouth" for the same reason — the old wording
claimed a count that is no longer made.

This is what keeping Stage 1A as corroboration actually buys: the record can
distinguish the stronger observation from the weaker one, instead of asserting
the stronger one always.

---

## Addendum 22 — the ear, and why no single radius could have worked

Addendum 20 widened `MOUTH_ZONE_NORM_DIST` from 0.15 to 0.30, reasoning that a
hand's centroid sits 40-60 mm from the lips at the moment of delivery while a
pill's centroid *is* at the lips. The reasoning was right and the fix was
wrong: on hardware, 0.30 began confirming on a hand raised to an **ear**.

Working out why is the useful part. Put the two distances side by side:

```
mouth centre -> ear         ~0.45-0.55 face widths
hand centroid at delivery   ~0.27-0.40 face widths
```

Those ranges are close enough that head pose closes the gap.

> **A radial test on a hand centroid cannot separate these two cases.** Not at
> 0.30, and not at any other single radius.

0.15 excludes the ear and also excludes a correct delivery. 0.30 admits a
correct delivery and the ear with it. There is no value that does both, and
choosing a third number would only have moved which of the two mistakes the
device makes. Two hardware rounds were spent moving one number back and forth
across a gap it cannot bridge.

### What works instead: containment, not proximity

When a person puts a pill in their mouth, their fingers are at their lips — so
the mouth point falls **inside the hand's bounding box**, even though the
centroid, being the middle of the palm, sits well below it. When the same hand
is at an ear, the box is beside the head and the mouth is outside it, at a
similar centroid distance.

So `in_mouth_zone` now asks whether the tracked object *contains* the mouth,
with a margin of one eighth of the object's own width and height. That margin
is a fraction of the OBJECT rather than of the face, so it scales with the hand
and with standing distance without another constant to tune.

`MOUTH_ZONE_NORM_DIST` went back to **0.18**, near the collaborator's original,
and is now only a fallback for frames where the mouth landmarks are momentarily
off and containment cannot be evaluated. A zero-extent box (the pill detector's
tiny boxes, or an unset observation) falls through to the radial tests, which
is right: containment is meaningless for an 8 px object.

`RETREAT_NORM_DIST` stays at 0.55 rather than following the entry threshold
back down, because entry no longer happens at a fixed radius — a large hand can
satisfy containment with its centroid ~0.4 face widths out, and a 0.35 exit
would then be satisfied at the same time as the entry condition.

### The general lesson

The first three attempts at this — 0.15, 0.30, and the impulse to try 0.22 —
were all attempts to find a better value for a measurement that does not
contain the answer. The distance from a hand's centre to a mouth is genuinely
ambiguous between "delivering" and "scratching an ear"; no threshold recovers
information the measurement never had.

> When a constant has to be moved in both directions to fix two different
> failures, the constant is not the problem. The quantity being measured is.

Containment is a different quantity, computed from data already in hand — the
tracked object's box, which Stage 1C has produced since the connected-component
rewrite in Addendum 16 — and it separates the two cases geometrically rather
than metrically.

---

## Addendum 23 — a real model, and the limit of using half a pipeline

Stage 1C's skin-tone heuristic was replaced by MediaPipe hand landmarks from
the ST model zoo (`pose_estimation/handlandmarks`, Apache-2.0, 224x224 INT8,
21 keypoints).

**Why the heuristic could not be saved.** Addendum 16 records four rounds of
fixes, each trading one failure for another, and the reason none of them could
work is one sentence:

> A moving ear is a moving skin region.

At the level that stage described the world — skin-coloured, solid, differs
from a baseline — an ear revealed by a turn of the head and a hand are the same
object. Size, density and colour all agree. Every fix was therefore geometric:
how far from the mouth, is the mouth inside the box, has the blob moved. A
model that knows what a hand *is* settles it in one step, and on hardware the
ear stopped firing immediately.

**What was measured before integrating**, which is the discipline Stage 1A's
history argues for:

| input | hand presence |
|---|---|
| this project's own compiled-in face image | 0.0078 |
| the same face in a wider frame | 0.0000 |
| 20 wall/background crops | median 0.000, max 0.012 |
| 400 random images | never above 0.035 |

The README claims one output; the model actually has **four**, and two of them
are `[1,1]` scalars. One is MediaPipe's hand-presence score. Which one was
determined by RUNNING the generated graph on known inputs, not inferred from
the names — they are `Dequantize_<n>_out_0` and carry no meaning.

**THE LIMIT, which is a property of the model and not of the integration.**
MediaPipe's landmark net is not a detector. In its own pipeline a *palm
detector* runs first and hands it a tight crop with the hand filling the frame.
We do not ship that palm detector. So presence is reliable when the hand is
centred and large in the crop, and unreliable elsewhere — and because the ROI
is centred on the mouth, "centred and large" and "at the mouth" are the same
condition.

That was found the direct way: waving a hand beside the face produces no box.
Two attempts to fix it by tuning — widening the ROI to 250% of face width, and
dropping the threshold to 0.15 — were both aimed at the wrong cause and were
reverted. Widening the crop makes the hand *smaller* in the model's input,
i.e. further out of distribution: the opposite of the intent.

**The delivered claim is therefore narrower than "action recognition", and the
documentation says so in those words: it looks for a hand near the mouth.**
That is what the device does, it does it reliably, and it is what the adherence
question needs. Calling it hand tracking would be an overclaim.

---

## Addendum 24 — the fingertip, and a constant that came home

MediaPipe keypoints 4 and 8 are the thumb tip and the index tip. Stage 1C
reports their midpoint, and Stage 2 measures the distance from THAT to the
mouth.

This retroactively settles the argument this session had twice.
`MOUTH_ZONE_NORM_DIST` was the collaborator's 0.15, was widened to 0.30 because
a hand's *centroid* sits 40-60 mm below the lips at delivery, locked onto an
ear because mouth-to-ear is 0.45-0.55 and the ranges overlap, and was pulled
back to 0.18 with a containment test bolted alongside.

Every one of those moves was trying to make a **palm** stand in for the thing
that enters a mouth. A fingertip *is* that thing — which is the property a
pill's own centroid had, and the property that made 0.15 correct in the first
place. The constant went back to 0.15, unchanged, for its original reason.

> A constant inherited with an algorithm carries the assumptions of the signal
> it was tuned on. Change the signal and it must be re-derived — and sometimes
> the re-derivation lands exactly where the original author put it.

Measured on hardware, the fingertip over three detections: mean (97,88), span
(18,20), in a 224 space whose centre is (112,112). An earlier hypothesis that
localisation had collapsed to the centre was **wrong** — a degenerate output
would have read mean (112,112) span (1,1). The box appearing on the mouth is
the gesture succeeding, not the model failing.

---

## Addendum 25 — two networks that stopped colliding

The hand model needs the whole 220 KB arena and the whole of `0x73000000`, so
the first integration switched the pill detector off. Both are now live, and
nothing is shared:

| network | activations | weights |
|---|---|---|
| hand landmarks | 220 KB `AI_ARENA` + ~978 KB PSRAM `0x90500000` | `0x73000000` |
| pill detector | 208 KB, entirely PSRAM `0x90A00000` | `0x73400000` |

Three things made that possible, and each is worth knowing on its own:

1. **PSRAM is NPU-reachable and declared `READ_WRITE`.** The hand model does
   not fit in the arena at all — the compiler refuses with `total bytes left
   unallocated=3515456` and single buffers of 451,584 bytes. It fits only by
   spilling to `0x90500000`.
2. **A network can be regenerated against a different pool.** The pill detector
   was rebuilt with AXISRAM6 excluded outright and xSPI2 based at `0x73400000`.
   No source change; the addresses are entirely a generation-time decision.
3. **`--Ocache-opt` has to be dropped when every activation is in a cacheable
   external pool.** The Neural Art compiler aborts on an internal assertion
   (`check_npu_caching_of_output_live_buffers`). That is a tool limitation, not
   a misconfiguration, and it costs one optimisation pass.

**The cost is speed, charged where it can be afforded.** A hand inference
measures 285-302 ms against ST's published 20.75 ms — theirs is all-internal.
Idle falls to 14-50% during a watch. The pill detector runs one frame in four
for the same reason, and it is the stage that can afford to be slow because it
only corroborates.

**Why the pill detector is still here at all.** The Program Plan commits to CNN
classification of pills. The honest position is that the HOPPER classifies —
one medicine per hopper, mechanically, foolproof in a way vision is not. The
detector's job is to corroborate that a pill was in the frame where the hand
reached the mouth, which is a real contribution without pretending to identify
the medicine. Face recognition + action recognition + pill corroboration, with
mechanical classification, is a stronger system than vision-based pill ID would
have been. `PROGRAM_PLAN_RECONCILIATION.md` now says this in full.

---

## Addendum 26 — two process lessons, both paid for twice

**A threshold set against a measured negative and an unmeasured positive is a
guess wearing a number.** `PRESENCE_T` was set to 0.50 with the negative side
characterised (a face 0.0078) and the positive side never measured. It then
moved 0.50 -> 0.30 -> 0.15 -> 0.30 across four hardware rounds. The per-watch
diagnostic that finally made those moves informed — the band histogram, the
peak to four decimals, the fingertip spread — should have existed before the
first value was chosen, not after the fourth.

The general form, which Addendum 16 also arrived at from the other direction:

> A diagnostic that reports *whether* something failed buys one round. A
> diagnostic that reports *which gate* rejected buys the fix.

**Power-cycle after external-loader operations — including READS.**
`AI_LESSONS.md` §3 documents a boot hang after back-to-back loader operations,
seen once in Session 08B. It happened again in Session 16, and the trigger was
two `-r32` verification reads on a live board. The board hung with the log
stopping right after `ai_vision_init: DEBUG build (-O0)` — before
`HAL_CACHEAXI_Enable`, i.e. where XSPI is first touched — and the LCD held a
stale frame.

The rule as written said to power-cycle after flashing. It should say: after
**any** external-loader operation. Reads confuse the flash chip's live bus
state exactly as writes do, and verifying a write is precisely when one is most
tempted to leave the board running.

---

## Addendum 27 — three faults found by watching the device, not the code

All three came out of the last hardware round, and none would have been found
by reading the source.

### The overlay painted over the home screen

After a dose timed out, the camera pane appeared on top of HOME. The log shows
why in its ordering: `STATE_HOME` is printed *before* the intake summary.

`intake_end()` only REQUESTS a stop. The AI task notices when it next comes
round its loop, and a hand inference takes ~305 ms — so for up to a third of a
second after the UI has moved on, `intake_is_active()` is still true and the
overlay is still publishing. The tick was gated on that flag.

The guard was wrong in kind rather than in value. The pane belongs to ONE
SCREEN, so it is now gated on `current_state == STATE_CONFIRM_TAKEN`.

> A subsystem's liveness is not a licence to draw.

### The mascot cried indefinitely

`MASCOT_ERROR` was set when a dose window closed unserved, and nothing cleared
it. The sad face persisted until the next window opened, or USER1 was pressed,
or an unrelated flow happened to reset it — about twelve minutes in DEMO mode,
hours on a real clock.

Bounded to 60 seconds of real time (`MISSED_SAD_HOLD_MS`), because an
indefinite signal stops carrying information: a device that has been crying for
an hour cannot tell a carer whether the miss was a minute ago or this morning.
The permanent record is the dose log; the mascot is an ambient cue.

Real seconds rather than demo-clock minutes, deliberately — this is feedback
for a person standing in the room, not a scheduled event. The expiry is checked
on the ordinary UI tick rather than inside `schedule_service()`, which only
runs on a schedule EVENT; a missed dose leaves the device in exactly the state
where no further event is coming.

### The log contradicted itself about a dose

The worst of the three, because it is in the medication record:

```
03:30:49  MISSED: DOUSIK did not take the 03:00 dose
03:32:50  CONFIRMED: DOUSIK took pills (hand reached mouth)
```

The patient was dispensed at 03:12 for the 03:00 window, the window closed at
03:30 before they tapped, and they confirmed at 03:32. Both lines are
individually true and together they are a contradiction. Worse, the wording
degrades: the in-window form names the dose (`took the 02:00 dose (on time…)`)
while the out-of-window form falls back to `took pills` and drops the
connection, so the two entries do not even obviously refer to the same dose.

**The MISSED line is not suppressed.** It was true when written, and an
append-only audit log must not rewrite history. Instead the device remembers
which window it declared missed, and a later confirmation for that same patient
names the same dose and says what it supersedes:

```
CONFIRMED LATE: DOUSIK took the 03:00 dose (supersedes MISSED; hand reached mouth)
```

109 characters worst case against `SD_LOG_MSG_MAX_LEN` of 128 with a maximal
31-character name — checked, because the first draft of the line was 142 and
would have silently truncated the verdict, which is the part a carer most needs.

A late confirmation also retires the sad face, since the question it was asking
has now been answered.

**The general point.** Session 15 built missed-dose recording so the device
could record what did NOT happen. A record that contradicts itself is worse
than one that says less — and the contradiction only appears when a real person
taps a real button two seconds after a real window closes.
