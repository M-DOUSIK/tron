# Session 09 — Patient Registration Flow

## How to Start This Session

Hello! We are starting Session 09 for the MedSight project — the real
patient registration flow, building on Session 08B's now-working AI vision
pipeline.

**Before writing any code or taking any action**, acquire full context:

1. **READ ALL DOCUMENTATION**: every markdown file in `documents/` —
   especially `MASTER_PROJECT_PLAN.md`, `SOFTWARE_ARCHITECTURE.md`,
   `HARDWARE_ARCHITECTURE.md`, `ENGINEERING_LESSONS.md`, `AI_LESSONS.md`,
   `AI_PIPELINE.md`, and `COMPLIANCE_PRIVACY_POSTURE.md`.

2. **READ PAST SESSION PROMPTS**: `session_01.md` through `session_08A.md`
   in `documents/prompts/`, to understand what's already built.

3. **READ `documents/milestones/session_08B_notes.md` IN FULL.** This is
   not optional background — it documents five hard-won hardware bugs found
   and fixed during Session 08B's actual bring-up (a stale-flash boot
   assertion, a *second* undiscovered NPU weight pool at a separate flash
   address, a self-test methodology bug, and — the big one — a whole
   external PSRAM chip that had to be brought up because the FaceID
   embedder silently hung/crashed without it). None of the docs written
   *before* 08B knew about these; the notes are the ground truth for what
   the AI pipeline actually needs to keep working. In particular:
   - `ai_vision.c` has a documented **memory hazard**: both NPU networks'
     activation scratch overlaps `BUFFER_ADDRESS` (the live camera
     framebuffer) and part of `GUI_BUFFER_ADDRESS`. `ai_vision_run_pipeline()`
     already handles this correctly (snapshots what it needs before
     touching the NPU) — do not "simplify" that code without re-reading why
     it's structured that way.
   - Never `printf` face embeddings or other biometric bytes — this was a
     hard rule in 08B and stays one here (`COMPLIANCE_PRIVACY_POSTURE.md`).
   - There's a known, deferred cosmetic issue: the LCD briefly (a few ms)
     shows corrupted pixels during NPU inference before self-healing when
     the camera resumes. Documented as intentionally deferred to Session 11
     — don't attempt to fix it in this session unless it's trivial; it
     isn't blocking.

4. **READ THE ACTUAL WORKING CODE** in `sessions/session_08B/FSBL/`
   (not just the docs) before writing anything — specifically:
   - `Inc/ai_vision.h` — the real, current API: `ai_vision_run_pipeline()`,
     `gallery_add_patient()`, `gallery_find_best_match()`, the
     `PatientRecord` struct (`name`, `embedding[128]`, `pill_count`,
     `pills_remaining`), `MAX_PATIENTS` (10), `PATIENT_NAME_MAX` (32).
   - `Src/ui/state_machine.c` — the real current state machine
     (`STATE_HOME`, `STATE_INSTRUCT_REGISTER`, `STATE_CAMERA_REGISTER`,
     `STATE_INSTRUCT_DISPENSE`, `STATE_CAMERA_DISPENSE`), including how
     `STATE_CAMERA_DISPENSE` already calls `ai_vision_run_pipeline()` with
     a 3-retry loop and `camera_stop()`/`camera_start()` bracketing —
     `STATE_CAMERA_REGISTER` is **still session_08A's untouched 5-second
     mock timer**. This session replaces that mock with the real flow,
     following the same camera-freeze pattern already proven working in
     the dispense state.
   - `Src/ui/gui_draw.c` / `Inc/ui/gui_draw.h` — existing screen-drawing
     primitives (`gui_draw_rect`, `gui_draw_text`, `gui_draw_ready_screen`,
     the elderly-friendly color palette) to build new screens from,
     consistent with the existing visual style.
   - `Src/sd_logger.c` / `Inc/sd_logger.h` — note `SD_Write_File`/
     `SD_Read_File` (generic named-file helpers added in 08B) already exist
     if you need SD access beyond what `gallery_add_patient()` covers.

---

## Project Rules (non-negotiable, carried forward)

| Rule | Detail |
|---|---|
| No `.ioc` files | Manual HAL only. Never use STM32CubeMX. |
| No new hardware | No physical motors, servos, or IR LEDs. The dispenser is 100% software-simulated for this prototype — a design model/render only, no actuator interfacing, ever. |
| No embeddings over UART | Never `printf`/log raw face embedding bytes. Names, indices, and confidence scores are fine to log. |
| OSAL-safe | Use `ms_osal.h` only. No direct FreeRTOS API calls anywhere. |
| DCache discipline | Any new code touching camera/NPU buffers follows the same clean-before-input / invalidate-after-output pattern already used in `ai_vision.c` — read that file's comments before adding anything similar. |
| New session = new folder | Copy `sessions/session_09B` (at the time this session started; that folder has since been renamed to `sessions/session_08B` — see `ENGINEERING_LESSONS.md`) → `sessions/session_09` (drops the stale "B" suffix from the do-over naming — Session 09 in `MASTER_PROJECT_PLAN.md`'s numbering is genuinely this Registration session, not a variant of 08B). Work happens in `session_09/`; the source folder (now `session_08B`) stays as the last-known-good rollback point. |

---

## Your First Action — Create the Working Folder

**Editorial note (post-session):** this section is kept as a historical record of what
was actually run — the source folder was named `session_09B` at the time and has since
been renamed to `session_08B` (see `ENGINEERING_LESSONS.md`); a reader following this
repo today should substitute `session_08B` for `session_09B` below.

```powershell
Copy-Item -Path "C:\Users\Dousik\Workspace\TRON\sessions\session_09B" `
          -Destination "C:\Users\Dousik\Workspace\TRON\sessions\session_09" -Recurse

# Delete stale .d files so make doesn't fail
Get-ChildItem -Path "C:\Users\Dousik\Workspace\TRON\sessions\session_09" `
              -Recurse -Filter "*.d" | Remove-Item -Force
```

**Then fix stale absolute paths** — every generated `subdir.mk`/`makefile`
under `STM32CubeIDE/` has `session_09B` baked into absolute source paths
from the copy. This bit Session 08B hard (silently compiling the *old*
folder's code). Before building anything:

```powershell
Get-ChildItem -Path "C:\Users\Dousik\Workspace\TRON\sessions\session_09\STM32CubeIDE" `
              -Recurse -Include "subdir.mk","makefile" | ForEach-Object {
  (Get-Content $_.FullName) -replace 'session_09B', 'session_09' | Set-Content $_.FullName
}
```

Also update the `.project` file's `<name>` tag and, optionally, rename the
build artifact (`MedSight_Session08B_FSBL` → `MedSight_Session09_FSBL`) in
`Debug/makefile` and `Release/makefile` for a clean identity — not required
for the build to work, just for clarity when you have multiple session
folders' `.elf` files around.

**No external NPU flash re-flashing is needed for this session** — Session
09 doesn't add or change any AI model files, so the OSPI weight data
flashed during 08B (documented in `session_08B_notes.md`) carries over
unchanged. A normal Debug/Run from STM32CubeIDE (internal-RAM-only reflash)
is all that's needed to test each change.

---

## What `session_08B` (the working base, named `session_09B` at the time) Gives You

- Working camera + LTDC + touch UI + FreeRTOS tasks (all prior sessions).
- **Working NPU face pipeline** — `ai_vision_run_pipeline()` reliably
  detects a face, extracts a 128-D int8 embedding, and
  `gallery_find_best_match()` correctly matches or rejects it. Proven on
  real hardware (see `session_08B_notes.md` Addendum 5).
- `gallery_add_patient(name, embedding, pill_count)` already exists and
  already persists to `patients.dat` on the SD card via `sd_logger.c` — the
  storage half of registration is *done*. This session is about building
  the **UI flow** that collects a name and pill count from the user and
  calls it, replacing `STATE_CAMERA_REGISTER`'s mock timer.
- `STATE_CAMERA_DISPENSE` is a working reference implementation of "freeze
  camera → run AI → resume camera" to model the new `STATE_CAMERA_REGISTER`
  after.

---

## Objective

Replace the mock `STATE_INSTRUCT_REGISTER` → `STATE_CAMERA_REGISTER` flow
with a complete, real registration flow: camera capture → face embedding →
name entry (on-screen keyboard) → pill count entry → confirm → save to the
gallery.

## Step-by-Step Implementation Plan

### Step 1 — Real face capture in `STATE_CAMERA_REGISTER`

Mirror `STATE_CAMERA_DISPENSE`'s pattern exactly: `camera_stop()`, 3-retry
`ai_vision_run_pipeline()` loop with a short delay between attempts,
`camera_start()`. On success, **hold the embedding in a session-scoped
static buffer** (not yet saved — the user hasn't entered a name or pill
count yet). On failure after 3 attempts, show a clear error and return to
`STATE_HOME` (don't silently fail).

### Step 2 — New state: `STATE_KEYBOARD_REGISTER`

An on-screen QWERTY keyboard (new file:
`ui/registration_ui.c`/`registration_ui.h`, per `SOFTWARE_ARCHITECTURE.md`
§3's already-documented exception — this module is allowed to call
`ai_vision`'s embedding-capture and `sd_logger`'s profile-write functions
directly, unlike other UI modules which only go through `state_machine.c`).
Cap input at `PATIENT_NAME_MAX - 1` characters. A "Done"/checkmark button
advances to pill count; a backspace/clear key is required — elderly users
mistype.

### Step 3 — New state: `STATE_PILLCOUNT_REGISTER`

Large +/- buttons (matching the existing elderly-friendly, high-contrast,
large-touch-target style in `gui_draw.h`) to set a daily pill count, 1–10.
Big legible number in the center.

### Step 4 — New state: `STATE_CONFIRM_REGISTER`

Summary screen: name + pill count. "Confirm" calls
`gallery_add_patient(name, embedding, pill_count)`, shows a clear
"Registered!" success message, then returns to `STATE_HOME`. "Retry"
discards the captured embedding and returns to `STATE_CAMERA_REGISTER`.

### Step 5 — Handle a full gallery

`gallery_add_patient()` returns `-1` if `MAX_PATIENTS` (10) is already
full. Show a clear "gallery full" message on that path instead of silently
failing or crashing.

### Step 6 — Build and verify

```powershell
$env:PATH += ";C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin"
C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.100.202601091506\tools\bin\make.exe -j12 -C C:\Users\Dousik\Workspace\TRON\sessions\session_09\STM32CubeIDE\FSBL\Debug all 2>&1
```
Zero errors, zero warnings before declaring anything done.

---

## Definition of Done

- [ ] `session_09` folder created from `session_08B` (named `session_09B` at the time), stale paths fixed
- [ ] Build is 100% clean — zero errors, zero warnings
- [ ] `registration_ui.c/.h` created per the documented architecture exception
- [ ] Full flow works on hardware: tap Register → face capture (reusing the
      proven 08B pipeline) → type a name on-screen → set pill count →
      confirm → "Registered!" → home
- [ ] The newly registered patient is then correctly **recognized** by
      Dispense (tests the whole loop: register someone, then dispense and
      confirm it's no longer "intruder" but shows their name)
- [ ] Gallery-full case shows a clear message, doesn't crash
- [ ] No face embedding bytes ever appear in UART output
- [ ] `documents/milestones/session_09_notes.md` written: what states
      were added, any deviations from this plan, and the actual hardware
      test result of the register-then-dispense end-to-end check above

---

## What This Session Does NOT Do

- No physical motors, servos, or IR hardware (still fully software-only)
- No changes to the AI models themselves (`ai_vision.c`'s pipeline,
  `fd.c`/`faceid.c`, and the flashed OSPI weight data all carry over
  unchanged from Session 08B — don't touch them unless something is
  actually broken)
- No fix for the known transient LCD glitch during inference — deferred to
  Session 11 per `session_08B_notes.md`
- No delete-patient / edit-patient UI (not asked for; `gallery` already
  has no delete function — add one only if this session's scope needs it,
  and note it explicitly in the session notes if you do)
