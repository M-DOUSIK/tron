# AI_PIPELINE.md — MedSight

## 1. Two Models, One NPU

1. **One-shot face recognition + small-gallery matching** — verifies which enrolled
   patient (of potentially several sharing the device) is present, before dispensing
   or logging anything. "One-shot" means enrollment from a single reference photo per
   patient (Session 09's registration flow), not a large training set per person.
   Matching is against a gallery of all currently enrolled patients, not a single
   hardcoded reference face — this is a shared, multi-person device (see
   `MASTER_PROJECT_PLAN.md` §1).
2. **Action/consumption recognition** — a lightweight model over a short frame
   sequence, detecting the hand-to-mouth pill-taking gesture to confirm the dispensed
   pill was actually consumed.

**Pill/packet classification was considered and deliberately dropped** (see
`MASTER_PROJECT_PLAN.md` §8): each hopper already dispenses a single, known medicine
by construction (`MECHANICAL_DESIGN.md`), so classifying the dispensed pill's type
mostly re-confirms what the mechanism already guarantees, at real NPU/development cost.
Action recognition is a better use of that budget — it's genuine new information
(consumption, not just correct dispensing) that nothing else in the system provides.

Both models run as INT8 models on the Neural-ART Accelerator via STM32Cube.AI-generated
C code, not on the Cortex-M55 CPU — this is a hard constraint (see
`SOFTWARE_ARCHITECTURE.md` module rules): CPU cycles stay free for UI/camera pipeline
work.

## 2. Toolchain

- **STM32Cube.AI** (X-CUBE-AI) — converts a trained/quantized model into C code mapped
  to the Neural-ART Accelerator.
- **ST Model Zoo** (`stm32ai-modelzoo-services` on GitHub) — good source of pretrained,
  already-STM32-optimized starting models; check here before training from scratch.
- Reference implementations worth reviewing for the face-recognition approach (not
  necessarily reused verbatim — check licenses before importing code):
  one-shot/few-shot face recognition projects targeting STM32N6-class NPUs.
- For action recognition specifically: keep the model genuinely lightweight — a
  purpose-trained short-sequence classifier over a small frame window, not a full
  pose-estimation model, which would be significant overkill for confirming one
  specific gesture.

## 3. De-Risking Plan (per risk register in MASTER_PROJECT_PLAN.md)

Do not start Session 08A by trying to get your real face+action models working end to
end. Sequence across Sessions 08A-C:
1. **Session 08A:** Get *any* trivial pretrained INT8 model (e.g. a stock
   classification model from the Model Zoo) running through STM32Cube.AI and
   executing on the NPU, with output visible over UART. This proves the
   toolchain/runtime path works before you add model-specific risk.
2. **Session 08B:** Swap in the real one-shot face recognition model, wired to a
   multi-entry gallery-matching function — verify correct identification and correct
   rejection of unenrolled faces against a placeholder test gallery.
3. **Session 08C:** Add the action/consumption-recognition model as a second inference
   pass, verify correct gesture detection and correct rejection of non-gestures.
4. **Session 09:** Wire real enrollment (replacing 08B's placeholder test gallery with
   real registered-patient embeddings from the SD card).
5. **Session 11:** Wire both models' outputs into the actual dispense flow and mascot
   UI states.

## 4. Data Handling

Face enrollment images/embeddings, and now also names and phone numbers collected
during registration (Session 09), are sensitive personal data — see
`COMPLIANCE_PRIVACY_POSTURE.md`. All of it is written only to the local SD card, never
transmitted, and never leave the device. None of it is ever logged over the UART debug
channel, in any session.

## 5. Performance Budget

Target: inference (both models combined — face recognition + action recognition, run
at different points in the dispense flow rather than simultaneously in most cases, but
budget for the worst case) fast enough not to visibly stall the mascot animation or
camera preview (Session 04 already establishes non-blocking rendering via DMA2D —
inference must not violate that). Profile actual NPU inference time in Session 08A and
record it here once measured; don't guess a number in advance.
