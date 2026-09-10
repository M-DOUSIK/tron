# AI_PIPELINE.md — MedSight

## 1. One Model, One NPU

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

**This is the only model in the final pipeline.** Two other models were considered
and both cut, in sequence (full history in `MASTER_PROJECT_PLAN.md` §8):
- **Pill/packet classification** — dropped early: under the physical multi-hopper
  design that existed at the time, each hopper would have dispensed a single, known
  medicine by construction, so classifying the dispensed pill's type mostly
  re-confirmed what the mechanism already guaranteed, at real NPU/development cost.
- **Action/consumption recognition** (a lightweight model detecting the hand-to-mouth
  pill-taking gesture, proposed to replace pill classification) — planned as
  Session 08C, **never built**. Once the physical dispensing hardware itself was cut
  (see `MASTER_PROJECT_PLAN.md`'s Changelog and `prompts/session_10.md`), consumption
  confirmation became a simple UI problem — a manual "✓ I Took It" button (Session 10)
  — with no need for a second NPU model. `session_08C.md` does not exist.

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
