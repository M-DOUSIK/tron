# Session 08B — One-Shot Face Recognition + Multi-Patient Gallery Matching

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Implement one-shot face recognition running on the NPU, matching a captured face
against a small gallery of enrolled patients (not a single hardcoded reference face) —
this is a shared, multi-person device, so identification must return "which patient,"
not just "match/no-match" against one person.

BACKGROUND AND CONTEXT
Building on tron/session_08A/ — the NPU inference pipeline is proven to work. MedSight
serves multiple people sharing one device, so face recognition here is small-gallery
identification: given a captured face, compare its embedding against every currently
enrolled patient's stored embedding and return either the best-matching patient ID (if
confidence clears a threshold) or "no match" (treated as an unrecognized/intruder case
by later sessions).

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ai_vision.c/.h (from 08A, extend)
- AI_PIPELINE.md §1-2 (updated to reflect gallery matching, not single-reference match)
- tron/session_08A/ as the base

REQUIRED INPUTS
- A one-shot face recognition/embedding model (INT8, NPU-targeted) — the same class of
  model referenced in AI_PIPELINE.md.
- For this session's standalone testing (registration/enrollment doesn't exist yet
  until Session 09), use 2-3 hardcoded test embeddings loaded from a test data file as
  a stand-in gallery — note this explicitly as a placeholder in the session notes;
  Session 09 replaces this with real enrolled-patient embeddings from the SD card.

EXPECTED OUTPUTS
- ai_vision.c updated with a clean, reusable one-shot embedding-extraction function
  (this exact function is what Session 09's registration flow will call to enroll a
  new patient, so design it as a standalone reusable API now, not something wired only
  to a "compare against test gallery" test harness).
- A gallery-matching function: given a captured embedding and a list of enrolled
  embeddings, return the best match (patient ID) if above a defined confidence
  threshold, or a clear "no match" result otherwise.
- Both running on the NPU, confirmed the same way as 08A (check STM32Cube.AI's
  conversion report for CPU fallback warnings).

CONSTRAINTS
- Must still execute on the NPU, confirmed the same way as 08A.
- Face embeddings are sensitive biometric data — this session's test embeddings are
  placeholders, but still route them through the same "never log to UART" discipline
  that will apply to real data later (build the habit now, per
  COMPLIANCE_PRIVACY_POSTURE.md).
- Do not implement action/consumption recognition yet — that's Session 08C.
- Do not wire this into the mascot UI, dispenser, or state machine yet — those come in
  Sessions 09-11. This session only proves embedding extraction + gallery matching.

FILES TO MODIFY
- FSBL/Src/ai_vision.c/.h

DOCUMENTATION TO UPDATE
- docs/milestones/session_08B_notes.md: model architecture/size, confidence threshold
  chosen and why, measured NPU latency, explicit note that the test gallery is a
  placeholder pending Session 09's real enrollment data.
- Update AI_PIPELINE.md if the model choice diverged from the originally referenced
  one-shot face recognition class of model.

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly.
- (Manual) Present a face matching one of the 2-3 test gallery entries; confirm the
  correct patient ID is returned. Present an unrecognized face; confirm "no match" is
  returned, not a false positive against the nearest test entry.

COMPLETION CHECKLIST
- [ ] One-shot embedding-extraction function implemented as a standalone reusable API
- [ ] Gallery-matching function implemented against a placeholder multi-entry gallery
- [ ] Confirmed running on NPU (not CPU fallback)
- [ ] Correct match and correct no-match both demonstrated
- [ ] session_08B_notes.md written, placeholder-gallery status noted explicitly

COMMON PITFALLS
- Designing the matching function only for a single reference face (easy to do if you
  test with just one gallery entry) and discovering later it doesn't generalize to
  real multi-patient galleries — test with at least 2-3 gallery entries from the start.
- Setting the confidence threshold too loose (false-positive matches between different
  people) or too tight (real patients rejected) — tune deliberately and record the
  chosen value and reasoning, don't leave it as an arbitrary default.
- Building the embedding-extraction function in a way that's awkward for Session 09 to
  call standalone (e.g. tightly coupled to this session's test harness) — keep it
  genuinely reusable now, since Session 09 depends on it directly.

DEFINITION OF DONE
A captured face correctly matches the right patient ID against a multi-entry test
gallery, and correctly returns "no match" for an unenrolled face — both confirmed
running on the NPU.

SELF-REVIEW BEFORE DECLARING COMPLETE
Confirm the embedding-extraction function has a clean enough signature that Session 09
can call it directly for enrollment without modification. Re-check the NPU execution
confirmation, same as 08A.
```

## Expected Deliverables
Reusable one-shot embedding-extraction API, multi-entry gallery-matching logic,
confirmed NPU execution, session notes.

## Manual Verification Steps
1. Present a face from the test gallery; confirm correct patient ID returned.
2. Present a different, unenrolled face; confirm "no match," not a false positive.
3. Test with all 2-3 gallery entries individually to confirm none are being
   mismatched against each other.

## Acceptance Criteria
Correct identification across multiple distinct gallery entries, correct rejection of
unenrolled faces, confirmed NPU execution.

## Next Prompt
Copy to `tron/session_08B/`, proceed to `session_08C.md`.
