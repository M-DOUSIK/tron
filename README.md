# MedSight — Smart Pill Dispenser

MedSight is an on-device, face-recognition pill dispenser built for elderly
and cognitively-impaired patients who need help remembering to take the
right dose. A patient registers once by facing the camera and entering their
name and daily pill count; from then on, the device recognizes them by face,
confirms their dose, and (as of Session 14, where built) physically dispenses
it — all without a network connection, a phone app, or a caretaker present
for every dose.

The problem it solves: missed or duplicated medication doses are a leading
cause of preventable harm in elderly and memory-impaired patients, and most
existing pill organizers are purely passive (a labelled box) — they don't
verify who is taking what, or whether a dose was actually taken. MedSight
puts identity verification and dose confirmation on the device itself, with
no cloud dependency and no biometric data ever leaving the board.

## Hardware

- **STM32N6570-DK** — STM32N657X0H3Q (Cortex-M55 @ 800 MHz + Neural-ART NPU
  @ 1 GHz, ~600 GOPS), 4.2 MB contiguous on-chip SRAM.
- **IMX335** camera module (DCMIPP pipeline) for face capture.
- **RK050HR18** 5" LCD panel with integrated **GT911** capacitive touch
  controller (I2C2, PD14/PD4).
- **microSD** card (SDMMC2) for the append-only audit log and patient
  gallery persistence.
- Session 14 (where it has run) adds a single-hopper pill dispenser: a
  28BYJ-48 stepper turntable + a 3-pin IR break-beam sensor module. See
  [`MedSight_Docs/DESIGN_PROTOTYPE.md`](MedSight_Docs/DESIGN_PROTOTYPE.md)
  for exactly what is physically built versus still design intent.
- No Wi-Fi/Ethernet/BLE — this device never connects to a network, by
  design (see Privacy & Security below).

## Software stack

- **µT-Kernel 3.0** (BSP2) real-time kernel — all application code talks to
  it through a single OSAL boundary (`ms_osal.h`); no `tk_*` kernel call
  appears anywhere outside `ms_osal.c`.
- **FatFs** for the microSD filesystem (patient gallery + audit log).
- **ST Edge AI** running on the Neural-ART NPU: a **CenterFace** face
  detector and a **MobileFaceNet** face embedder (ST's `stai_faceid`
  wrapper is the API surface, not a separate model — the two networks above
  are the actual architecture). One model pipeline; no pill-type
  classification or action recognition model was built (evaluated and
  dropped early — see `MedSight_Docs/AI_PIPELINE.md`).
- A small hand-rolled UI layer (`ui/gui_draw.c`, `ui/registration_ui.c`,
  `ui/anime_ui.c`, `ui/state_machine.c`) drawing directly into a 800×480
  RGB565 framebuffer, scanned out by the LTDC. As of Session 13 the UI owns
  two framebuffers — a camera-preview buffer and a separate GUI buffer the
  NPU never touches — so the screen no longer visibly corrupts during face
  capture.

## Build instructions

- **STM32CubeIDE required.** This project has **no `.ioc` file** — the base
  project came from an ST example, not STM32CubeMX, so every peripheral
  (LTDC, DCMIPP, SDMMC2, I2C2, DMA2D, ...) is configured by hand in
  `stm32n6xx_hal_msp.c`/`main.c`. Do not try to regenerate it from an `.ioc`.
- Each development session lives in its own folder under `sessions/` (e.g.
  `sessions/session_13/`) — always build the **latest** session folder;
  earlier ones are kept as working snapshots, not maintained in parallel.
- Open `sessions/session_13/STM32CubeIDE/FSBL` in STM32CubeIDE, or build
  headlessly:
  ```bash
  IDE="C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/stm32cubeidec.exe"
  WS="/some/throwaway/workspace"   # never point this at the repo

  "$IDE" --launcher.suppressErrors -nosplash \
    -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
    -data "$WS" \
    -import "<repo>/sessions/session_13/STM32CubeIDE/FSBL" \
    -cleanBuild "MedSight_Session13_FSBL"     # builds both Debug and Release
  ```
  See `MedSight_Docs/ENGINEERING_LESSONS.md` for the full rationale and the
  folder-copy/`.d`-file hygiene needed after copying a session folder.
- Both `Debug` and `Release` configurations build clean, 0 errors. Verified
  from a fresh clone into an empty workspace, not just in place.
- **UI assets are pre-generated and committed.** The sprites and fonts in
  `FSBL/Inc/ui/ui_assets.h` and `ui_assets_data.inc` are produced from the
  designer's PNGs by a script that lives in `scratch/`, which is not
  version-controlled. Nothing about the build depends on it - a clone
  compiles and flashes as-is. You only need the generator (and the original
  artwork) to change the artwork.

## Known limitations and future work

- **Single hopper only** (or none, if Session 14 hasn't run in this
  checkout) — the 6–8 independently-addressable hopper architecture in
  `MedSight_Docs/MECHANICAL_DESIGN.md` is documented design intent, not
  built. Adding hoppers is meant to be additive, not a firmware rewrite.
- **No on-device patient deletion** — removing a patient currently means
  physically pulling the SD card and editing/removing `patients.dat`
  offline. A proper delete flow is scoped for Session 15.
- **Enrollment is unauthenticated** — anyone standing at the device can
  register a new patient. Closing this is also Session 15's job.
- **Gallery capacity is fixed at 10 patients.**
- No scheduling/reminders yet (Session 15) — the device dispenses on
  request, not on a clock.

## Privacy & security posture

No network stack exists on this device — Wi-Fi/Ethernet/BLE are never
initialized, by design, so there is no path for biometric data to leave the
board. Face embeddings are never written to UART or any log; only names,
gallery slot indices, dose counts, and match confidence scores are
printed/logged. See `MedSight_Docs/COMPLIANCE_PRIVACY_POSTURE.md` for the
full posture and its known gaps.

**No µT-Kernel 3.0 API was changed.** The vendored `mtk3_bsp2/` tree and
kernel-facing internals in `ms_osal.c` are treated as fixed dependencies
across every session — see `MedSight_Docs/ENGINEERING_LESSONS.md` and
`MedSight_Docs/milestones/session_11_notes.md`/`session_12_notes.md` for the
integration history.

Third-party components and their licenses are inventoried in
[`MedSight_Docs/THIRD_PARTY_SOFTWARE.md`](MedSight_Docs/THIRD_PARTY_SOFTWARE.md).

## Documentation

Project history and design rationale live in `MedSight_Docs/` — start with
`MASTER_PROJECT_PLAN.md`, `SOFTWARE_ARCHITECTURE.md`, and
`HARDWARE_ARCHITECTURE.md`. Every development session has a prompt
(`MedSight_Docs/prompts/session_NN.md`) and, once complete, a notes file
(`MedSight_Docs/milestones/session_NN_notes.md`) recording what actually
happened on real hardware, including bugs found and fixed.
