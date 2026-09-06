# AI_PIPELINE.md — MedSight

## 1. One Model, One NPU

**One-shot face recognition + small-gallery matching** — verifies which enrolled
patient (of potentially several sharing the device) is present, before dispensing
or logging anything. "One-shot" means enrollment from a single reference photo per
patient (Session 09's registration flow), not a large training set per person.
Matching is against a gallery of all currently enrolled patients, not a single
hardcoded reference face — this is a shared, multi-person device (see
`MASTER_PROJECT_PLAN.md` §1). Implemented Session 08B (CenterFace detector + FaceID
embedder), confirmed working end-to-end on real hardware.

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

Face enrollment images/embeddings, and now also the patient's name and daily pill
count collected during registration (Session 09, as actually implemented — see
`SOFTWARE_ARCHITECTURE.md` §6; an earlier draft of the data model also planned a
phone number field for a future notification feature, but that was never built),
are sensitive personal data — see `COMPLIANCE_PRIVACY_POSTURE.md`. All of it is
written only to the local SD card, never transmitted, and never leaves the device.
None of it is ever logged over the UART debug channel, in any session.

## 5. Performance Budget

Target: face-recognition inference (the only model in the pipeline — see §1) fast
enough not to visibly stall the mascot animation or camera preview (Session 04
already establishes non-blocking rendering via DMA2D — inference must not violate
that). Profile actual NPU inference time in Session 08A/08B and record it here once
measured; don't guess a number in advance.
