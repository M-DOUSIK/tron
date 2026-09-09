# Session 08A — AI Toolchain Proof (Trivial Model on NPU)

*Session 08 is split into 08A/08B/08C because it is the highest-risk session in the
project (see MASTER_PROJECT_PLAN.md risk register). Do not skip ahead to 08B/08C
before 08A succeeds.*

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Prove the STM32Cube.AI → Neural-ART NPU toolchain path works end-to-end using a
trivial, off-the-shelf pretrained model, before touching the real pill/face models.
This isolates toolchain risk from model risk.

BACKGROUND AND CONTEXT
Building on tron/session_07/ (OSAL + FreeRTOS in place, touch GUI functioning). Per
AI_PIPELINE.md §3, the de-risking plan requires validating the inference runtime path
with a throwaway model first.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ai_vision.c/.h (new)
- AI_PIPELINE.md §3 (the de-risking sequence this session implements step 1 of)
- tron/session_07/ as the base

REQUIRED INPUTS
- A small pretrained INT8 classification model sourced from the ST Model Zoo
  (stm32ai-modelzoo-services on GitHub) — pick the simplest available image
  classification model, not anything face specific.

EXPECTED OUTPUTS
- The chosen model converted via STM32Cube.AI into C code targeting the Neural-ART
  Accelerator, integrated as a new RTOS task (via the OSAL from Session 07), running
  inference on live or static camera frames and printing the output classification
  and inference latency over the UART debug log.

CONSTRAINTS
- Model must execute on the NPU, not the Cortex-M55 CPU — confirm this via
  STM32Cube.AI's reported operator mapping, not just "it works."
- This is throwaway validation code — do not wire it into the interactive GUI's
  mascot states or SD logger yet. Keep it isolated so it's trivial to replace in
  Session 08B.
- All task creation must go through the OSAL, per Session 07's rule.

CODING STANDARDS
- Keep this in ai_vision.c/.h from the start, even though the model will be replaced —
  this file's structure carries forward into 08B/08C.

FILES TO CREATE
- FSBL/Src/ai_vision.c
- FSBL/Inc/ai_vision.h

FILES TO MODIFY
- FSBL/Src/main.c (create the AI inference task via OSAL)

DOCUMENTATION TO UPDATE
- docs/milestones/session_08A_notes.md: which Model Zoo model was used, STM32Cube.AI
  conversion steps taken, confirmed NPU vs. CPU execution, measured inference latency.
- Fill in AI_PIPELINE.md §5's performance budget with the actual measured latency.

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly, model conversion completes without STM32Cube.AI errors.
- (Manual) UART log shows classification output and latency for each inference,
  confirmed running on the NPU.

COMPLETION CHECKLIST
- [ ] Pretrained model selected and converted via STM32Cube.AI
- [ ] NPU execution confirmed (not CPU fallback)
- [ ] Inference task created via OSAL, output logged over UART
- [ ] session_08A_notes.md written with measured latency

COMMON PITFALLS
- STM32Cube.AI silently falling back to CPU execution for unsupported operators —
  check the conversion report carefully, don't assume NPU execution.
- Memory allocation failures if the model's activation buffers don't fit in the
  budget already committed to camera/LCD framebuffers in PSRAM/SRAM.

DEFINITION OF DONE
A trivial model runs on the NPU, confirmed via STM32Cube.AI's own reporting, with
output and timing visible over UART. This validates the toolchain before Session 08B
introduces real models.

SELF-REVIEW BEFORE DECLARING COMPLETE
Re-check the STM32Cube.AI conversion report specifically for "unsupported operator"
or CPU-fallback warnings — don't declare success on a model that's secretly running
on the CPU.
```

## Expected Deliverables
Working NPU inference pipeline with a throwaway model, latency measurement.

## Manual Verification Steps
1. Flash and watch UART output for classification results and latency numbers.
2. Cross-check STM32Cube.AI's conversion report for confirmed NPU mapping.

## Acceptance Criteria
Confirmed NPU (not CPU) execution, stable repeated inference, latency recorded.

## Next Prompt
Copy to `tron/session_08A/`, proceed to `session_08B.md`.

Reference:
https://www.st.com/en/development-tools/stm32n6-ai.html