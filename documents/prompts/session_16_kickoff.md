# Session 16 — kickoff message

Paste everything below the line into a fresh Claude Code chat in this repo.

---

Hello! We are starting **Session 16** for the MedSight project.

**IMPORTANT — session ordering and numbering, read this first.** The session
numbering changed after Session 15 and the folder names will look odd if you
do not know why:

- Sessions 01–13 and **15** are done and hardware-verified.
- **Session 14 is a retired number.** Its prompt (physical dispensing
  hardware) was renumbered to **Session 17**, because the project owner
  decided to do all hardware interfacing last. There is no `session_14.md`
  and there never will be a `session_14_notes.md`.
- **This session is 16: action recognition.** Session 17 (hardware + the
  carer buzzer) comes after it and has not run.

Before anything else, run `ls sessions/` and `ls documents/milestones/`, pick
as your base the highest-numbered `sessions/session_NN` that has a matching
`session_NN_notes.md` recording a completed hardware-verified run (it should
be `session_15`), copy it to `sessions/session_16/`, and tell me which base
you chose and why.

Then, before you write any code or take any action, do these in order:

1. Read every markdown file in `documents/` — especially
   `MASTER_PROJECT_PLAN.md` (the **v13** changelog entry first, it explains
   the renumbering), `MEMORY_MAP.md`, `SOFTWARE_ARCHITECTURE.md`,
   `PROGRAM_PLAN_RECONCILIATION.md`, `AI_PIPELINE.md`, `AI_LESSONS.md`,
   `ENGINEERING_LESSONS.md` and `COMPLIANCE_PRIVACY_POSTURE.md`.
2. Read all session prompts in `documents/prompts/` from `session_01.md`
   through `session_15.md`, plus `session_17.md` so you know what comes next
   and do not collide with it.
3. Read every milestone notes file in `documents/milestones/` in order.
   **`session_15_notes.md` is the most important**: Addendum 2 is the
   corrected action-recognition memory analysis and the decision to
   corroborate rather than replace, and Addendum 1b records the hardware
   round. **`session_12_notes.md` Addendum 9 is a hard constraint** — `WFI`
   stops the clock of any memory bank whose `LPEN` bit is clear, so any new
   DMA destination needs its bit added to `ms_configure_sleep_clocks()` in
   the same change and verified from a cold boot. Part B of this session adds
   the first new DMA destination since that bug.
4. Read **`tools/action_recogntion/summary.md` in full** — this is the
   collaborator's own architecture document for the model they built and
   trained. Also look at `tools/action_recogntion/main/main.py` (the
   `GuardedIntakeStateMachine` you will be porting to C),
   `main/hand_pill_tracker.py`, `main/mouth_tracker.py`, and
   `models/pill_detector/best.onnx`.
5. Read `tools/Program Plan 54916.pdf` — the contest program plan the judges
   have already read.
6. Read the working code in `sessions/session_15/FSBL/`, especially
   `Src/ai/ai_vision.c` (the two existing NPU models and the AI task),
   `Src/ui/state_machine.c` (`STATE_CONFIRM_TAKEN` is where this lands),
   `Src/main.c` (`ms_configure_sleep_clocks()` and the camera bring-up), and
   the linker script `STM32CubeIDE/FSBL/STM32N657X0HXQ_AXISRAM2_fsbl.ld`
   (the `AI_ARENA` region).
7. Read `documents/prompts/session_16.md` — this is your complete briefing.
   Follow it precisely.

This session adds action recognition as **corroboration** of the "✓ I Took
It" button, never as a replacement. Part 0 of the briefing contains an
analysis of the collaborator's work that is already done — verify it, do not
redo it. Two things from it worth knowing before you start reading:

- Only **one** of the three stages needs the NPU (the YOLOv8n pill
  detector). Stage 3 is a rule-based state machine, not a neural network.
- **Mouth tracking is free.** The CenterFace detector that has run since
  Session 08B emits five landmarks including both mouth corners, on a tensor
  `ai_vision.c` already `#define`s (`FD_OUT_LANDMARKS`) and has never read.
  The collaborator's MediaPipe dependency does not need replacing — it needs
  decoding.

The hard part is **not** the model. It is the camera: the DCMIPP currently
DMAs into the display framebuffer and the camera is stopped for the whole
dispense flow. Read Part B before you plan your time.
