# MedSight — Smart Pill Dispenser

**TRON Programming Contest 2026 · RTOS Application category · Student Division**

MedSight is an on-device, face-recognition medication adherence device for
elderly and cognitively-impaired patients. A carer enrols a patient once —
face, name, dose size and the times of day they take it — and from then on
the device recognises the patient by face, reminds them when a dose is due,
dispenses it, and records what actually happened. No network connection, no
phone app, no cloud, and no biometric data ever leaves the board.

The problem: missed or duplicated doses are a leading cause of preventable
harm in elderly and memory-impaired patients, and most pill organisers are
passive — a labelled box does not know who opened it or whether anything was
taken. MedSight puts identity verification, timing and dose confirmation on
the device itself.

---

## The numbers

Every figure below is measured on the real board and traceable to a session's
notes. None of them is an estimate.

| | Measured | Where |
|---|---|---|
| **CPU idle** | **~89.6%** of wall-clock time asleep in `WFI`, waking ~980×/s | `session_12_notes.md` Part B |
| **NPU inference latency** | **209 ms** end-to-end — detector + embedder + the RTOS IPC between them — identical to the millisecond across four captures with different faces and confidences | `AI_PIPELINE.md` §5, `session_13_notes.md` Addendum 9 |
| **Failed capture** | 1111 ms (3 detector passes + two 500 ms waits) | same |
| **Memory footprint, Debug** | `.text` 933,328 · `.data` 4,036 · `.bss` 663,876 — 47.3% of the code region, 77.6% of the data region | `MEMORY_MAP.md` §4 |
| **Memory footprint, Release** | `.text` 778,688 · `.data` 4,032 · `.bss` 663,868 — 34.0% / 76.1% | same |
| **Free NPU-reachable SRAM** | **220 KB**, claimed as a named linker region and pattern-tested from a cold boot | `MEMORY_MAP.md` §3 |
| **µT-Kernel modification surface** | **6 modified files out of ~230**, and **every file implementing a system call is byte-identical to upstream** — verified by recursive diff against pristine mtk3_bsp2 | `THIRD_PARTY_SOFTWARE.md` §4 |
| **Hand-written code** | ~11,800 lines across 28 files, excluding all vendored code and generated assets | `PROGRAM_PLAN_RECONCILIATION.md` §7 |

The 209 ms figure is worth one extra sentence, because the *invariance* is the
interesting part: four captures, four different images, four different
detector confidences, and the same 209 ms every time. The Neural-ART runtime
executes a fixed epoch schedule for a fixed input shape, so cost does not
depend on image content — which is what makes it defensible to hold the
display still for the capture window instead of showing an indeterminate
spinner.

---

## What makes this a µT-Kernel application, specifically

Rule 1.4 asks for "a high degree of relevance to µT-Kernel 3.0" for the
TRON × AI theme. This project's answer is not "we used an RTOS":

- **Inference is dispatched through a µT-Kernel event flag.** The NPU runs in
  its own task at a deliberately *lower* priority than the UI, and the
  request/response handshake is `tk_cre_flg` / `tk_set_flg` / `tk_wai_flg`
  with `TWF_ORW | TWF_BITCLR`. The UI's wait has three outcomes to
  distinguish in one blocking call — face found, no face, the AI task never
  answered — which is exactly what an event flag expresses and what a queue,
  a semaphore or a mutex cannot. The behavioural payoff is real: touch and
  the physical USER1 button stay alive during a capture, where before the
  whole UI task blocked inside the NPU for seconds.
- **Dose scheduling is a µT-Kernel alarm handler.** `tk_cre_alm` /
  `tk_sta_alm`, one-shot, re-armed for the window's closing edge and then for
  the next dose. A task polling the clock would wake 8,640 times a day to act
  four times, and would spend the power-saving figure above to do it. The
  handler runs in handler context, so it sets one bit in an event flag and
  returns — every line of actual work is done by a task.
- **Task priorities are derived, not inherited.** Five tasks, rate-monotonic:
  shortest period gets the highest priority, each number justified in writing
  (`SOFTWARE_ARCHITECTURE.md` §9). The AI sits below the UI *on purpose*.
- **`low_pow()` is a real `WFI` with a measured effect.** The vendored BSP
  ships it empty. It now forwards to `ms_osal_low_power_idle()`, and the
  masking around that instruction is load-bearing in a way the first flash
  proved (see the debugging stories below).
- **Deferred object creation bridges a genuine µT-Kernel constraint.**
  `tk_cre_tsk`/`tk_cre_mbf`/`tk_cre_mtx`/`tk_cre_flg`/`tk_cre_alm` can only be
  called once the kernel is running, but every `osal_*_create()` in this
  codebase happens in `main()` before the scheduler starts. `ms_osal.c`
  defers real creation to its own `usermain()`. Callers never see it.
- **Two idioms were evaluated and deliberately NOT adopted**, with their
  evidence written down: a fixed-size memory pool (`tk_cre_mpf` — this
  firmware has no fixed-size runtime allocation site at all, so adopting one
  would have meant inventing an allocation in order to have something to
  pool) and an event flag for `STATE_CONFIRM_TAKEN` (all three of its
  conditions are produced by the task that would wait on them). A contrived
  use reads worse to an expert than an absent one.

**No µT-Kernel API was changed.** Six vendored files differ from upstream, and
none of them implements a system call.

---

## The debugging stories

These are the strongest evidence in the project that the platform is
understood rather than assembled from examples. Each was found on real
hardware, root-caused, and written down as it happened.

**The D-cache ate the vector table.** (`session_11_notes.md` Addendum 7)
µT-Kernel builds its exception vector table at runtime, in write-back
cacheable RAM, and the vendored BSP does no cache maintenance of its own. On
a Cortex-M55 with caches enabled — which the BSP's own reference project does
not do — the NVIC fetched stale vectors. The fix is a single
`ms_osal_clean_dcache()` call placed on this project's side of the vendored
code boundary.

**A `printf` on the dispatcher path cost the whole system.**
(`session_11_notes.md` Addendum 8) Instrumentation added to diagnose a
scheduling problem made SysTick starve PendSV, which is to say the
measurement created the fault it was measuring. It is why nothing in this
firmware prints from handler context, and why the idle accounting is read out
from a task on a 10-second period.

**`WFI` hung the board, and BASEPRI was why.** (`session_12_notes.md`
Addendum 1) The idle path runs with `BASEPRI = 0x10`, and SysTick's priority
is *also* `0x10`. A wake-up event must be an exception that would preempt the
current execution priority — the Arm ARM excludes PRIMASK from that judgement
but **not** BASEPRI — so SysTick could not wake the core and a plain `WFI`
froze the system the instant it first had nothing to run.

**Putting a CPU to sleep is a system-wide change, not a power tweak.**
(`session_12_notes.md` Addendum 9 — six rounds, five wrong diagnoses) `WFI`
on this part enters CSleep, which stops the clock of every peripheral, bus
and memory whose `LPEN` bit is clear. The framebuffer is in AXISRAM3–6, so
every idle tick cost the LTDC either its own clock, the AXI bus clock, or the
RAM it was reading. The panel starved and greyed out while every register the
CPU could read said the display was healthy — because the CPU only reads when
it is awake. The measurement that isolated it removed the sleep instruction
and changed nothing else.

**A guard that could never pass, for six sessions.** (`session_12_notes.md`
Addendum 2) `disk_ioctl()` checked `HAL_SD_GetState() != HAL_SD_STATE_TRANSFER`
— but the HAL driver never assigns `HAL_SD_STATE_TRANSFER` to that field. The
condition was unconditionally true, so `disk_ioctl()` returned `RES_NOTRDY`
for every command it was ever given, from Session 06 to Session 12. It hid
because FatFs calls it at the very end of `f_close()`, after everything has
already been written, and because two call sites above it threw the result
away.

**Debug and Release laid the NPU weights out in opposite order.**
(`session_12_notes.md` Addendum 5) The epoch-controller blobs are `const`
arrays in one literal section, so their internal order is GCC's emission
order — which is not the same at `-O0` and `-Os`. Every blob address the
Release binary computed pointed at a different blob's bytes. Fixed with
`-fno-toplevel-reorder`; one flashed image now serves both configurations.

**A cold boot is not the reset you have been testing.**
(`session_12_notes.md` Addendum 6) The PWR "supply valid" bits live in the
always-on domain and survive a system reset, so nine sessions of
flash-and-run inherited I/O domains that a genuine power cycle does not
provide. The display half-worked in a way that only appeared when the
development habit changed.

**The screen said "Registered!" when nothing had been saved.**
(`session_13_notes.md` Addendum 9) `gallery_add_patient()` returned the slot
number on a failed card write, so the UI reported success for an enrolment
that would vanish at the next power cycle. The fix is not just the return
value: the device now distinguishes "saved" from "saved for now — no SD card"
and says which.

---

## Hardware

- **STM32N6570-DK** — STM32N657X0H3Q (Cortex-M55 @ 800 MHz + Neural-ART NPU
  @ 1 GHz, ~600 GOPS), ~3.75 MB of AXI SRAM in six banks
  (see [`documents/MEMORY_MAP.md`](documents/MEMORY_MAP.md)).
- **IMX335** camera module (DCMIPP pipeline) for face capture.
- **RK050HR18** 5" LCD with integrated **GT911** capacitive touch (I2C2,
  PD14/PD4).
- **microSD** (SDMMC2) — the append-only audit log, the patient gallery, and
  the carer passcode hash.
- **Internal RTC** (LSE, falling back to LSI) — the dose schedule, kept
  across resets in the backup domain.
- Session 17 (where it has run in this checkout) adds a single-hopper
  dispenser: a 28BYJ-48 stepper turntable and a 3-pin IR break-beam module
  that counts each pill as it physically drops. See
  [`documents/DESIGN_PROTOTYPE.md`](documents/DESIGN_PROTOTYPE.md) for what
  is built versus still design intent.
- No Wi-Fi, Ethernet or BLE — this device never connects to a network, by
  design.

## Software stack

- **µT-Kernel 3.0** (BSP2). All application code reaches it through one OSAL
  boundary (`ms_osal.h`); no `tk_*` call appears anywhere outside
  `ms_osal.c`.
- **FatFs** for the microSD filesystem.
- **ST Edge AI** on the Neural-ART NPU: a **CenterFace** detector and a
  **MobileFaceNet** embedder (ST's `stai_faceid` is the wrapper's name, not
  the architecture). One model pipeline; no pill-type classification and no
  action-recognition model was built — see
  [`documents/PROGRAM_PLAN_RECONCILIATION.md`](documents/PROGRAM_PLAN_RECONCILIATION.md)
  §1 for the substitution and the reasoning.
- A hand-rolled UI layer (`ui/gui_draw.c`, `ui/state_machine.c`,
  `ui/registration_ui.c`, `ui/carer_ui.c`, `ui/anime_ui.c`) drawing into an
  800×480 RGB565 framebuffer scanned out by the LTDC, with an original
  animated mascot. **One** framebuffer: a second one was attempted in Session
  13 and reverted when a canary proved the NPU's activation scratch runs
  straight through it — the display is blanked for the ~750 ms capture window
  instead.

## Build

- **STM32CubeIDE required, and there is no `.ioc` file.** The base project
  came from an ST example rather than STM32CubeMX, so every peripheral —
  LTDC, DCMIPP, SDMMC2, I2C2, DMA2D, RTC — is configured by hand in
  `stm32n6xx_hal_msp.c` / `main.c` / the driver modules. Do not try to
  regenerate it from an `.ioc`.
- Each session lives in its own folder under `sessions/`. **Build the
  highest-numbered one**; earlier folders are working snapshots and a
  rollback trail, not parallel branches.
- In the IDE, open `sessions/session_15/STM32CubeIDE/FSBL`. Headlessly:

  ```bash
  IDE="C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/stm32cubeidec.exe"
  WS="/some/throwaway/workspace"   # never point this at the repo

  "$IDE" --launcher.suppressErrors -nosplash \
    -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
    -data "$WS" \
    -import "<repo>/sessions/session_15/STM32CubeIDE/FSBL" \
    -cleanBuild "MedSight_Session15_FSBL"     # both Debug and Release
  ```

  Note that on Windows the `-import` path must use backslashes; a
  forward-slash path with a drive letter is parsed as a URI scheme and fails
  with `No file system is defined for scheme: C`.
- Both configurations build clean, 0 errors. The only warnings are four
  pre-existing ones in ST-generated code (`ll_aton_profiler.c`, `ATON.h`).
- **UI assets are pre-generated and committed.** A clone compiles and flashes
  as-is; the generator in `tools/` is only needed to change the artwork.
- **NPU weights live in external OSPI NOR and are flashed separately**, once.
  A normal build never touches them (the section is `(NOLOAD)`). See
  `documents/AI_LESSONS.md` for the commands, and check the layout with
  `arm-none-eabi-nm -n <elf> | grep '^71' | head` before blaming code for an
  epoch-controller error.

## Using it

- **Carer passcode.** Ships as a build-time default (`1379`,
  `MEDSIGHT_DEFAULT_CARER_CODE` in `ui/carer_ui.h`) so a fresh device is
  usable. Change it from carer mode; the new value's hash is written to the
  card. The device says on every boot if it is still on the default.
- **Carer mode** is reached by tapping the **home screen's title bar five
  times within three seconds**, then entering the passcode.
- **Registering a patient requires the passcode**, asked before the camera is
  ever used.
- **Demo mode.** `MEDSIGHT_FAST_CLOCK=1` compresses a whole day into
  `MEDSIGHT_FAST_DAY_SECONDS` (1440 s), which makes **one simulated minute
  exactly one real second**: a 30-minute dose window lasts 30 real seconds —
  long enough to actually walk through a dispense inside it — and a missed
  dose takes half a minute to demonstrate instead of half an hour. It is
  **on by default in the Debug configuration** and off in Release; remove the
  `MEDSIGHT_FAST_CLOCK=1` entry from Debug in `.cproject` to get a
  real-clock Debug build. Both modes go through the same interface, and
  nothing else in the firmware knows which one is behind it.
- **`patients.dat` is format v3** as of Session 15. A v2 card from an earlier
  build is reported and rejected cleanly rather than silently loading empty —
  **carers re-register once** after upgrading.

---

## Honest limitations

Stated plainly, because a judge sees through inflation and
`COMPLIANCE_PRIVACY_POSTURE.md` §3 already argues that honesty is the
stronger position.

- **This is not a medically-certified device.** No PMDA approval, no SaMD
  classification, no clinical validation, and none is claimed. The
  data-locality choices are *inspired by* the requirements such a device
  would face; they are not a regulatory position.
- **The carer passcode protects against a curious patient, not an attacker.**
  There is no secure element on this board, the SD card is unencrypted, and
  the stored value is a 32-bit FNV-1a hash. Anyone holding the card can
  brute-force a four-digit code in seconds. The rate limiter is in RAM and a
  power cycle clears it.
- **Face matching is a similarity threshold, not an identity guarantee.** It
  can produce false rejections — one was observed and diagnosed in Session 12
  — and, in principle, false acceptances. The threshold (0.65 cosine) is
  documented and measurable rather than assumed; it was deliberately *not*
  moved blind, because loosening it trades a recoverable false rejection for
  a false acceptance.
- **The SD card is unencrypted.** Physical possession gives access to the
  names, the embeddings and the event log.
- **No tamper detection and no audit-log integrity protection.** The log is
  append-only by convention, not by enforcement.
- **One hopper, not six.** The 6–8 hopper architecture in
  `MECHANICAL_DESIGN.md` is documented design intent.
- **Gallery capacity is fixed at 10 patients**, and 4 dose times each.
- **In demo mode the schedule is driven from the millisecond tick**, which
  wraps after 49 days. Harmless in a demo build; the real build does not have
  this property at all.
- **The gallery-full path has never been exercised on hardware.** The check
  is in place and reviewed; filling ten slots takes deliberate effort and
  nobody has. Treat it as untested.

## Known future work

Deliberately kept here, in one place, rather than in a separate document — the
Program Plan's own "Extensibility" section was one of its strengths, and being
able to say precisely what comes next is a sign of a project that is
understood rather than merely finished. Roughly in the order it would be worth
doing.

| Item | Why it was deferred | What it needs first | Rough cost |
|---|---|---|---|
| **Audio alert — scheduled for Session 17, and it is a CARER alert, not a patient one** | The buzzer claim was dropped from the documents in Session 12 rather than half-built. Session 15 gave the device the one event that genuinely needs sound: a dose window closing unserved. **Decided:** the buzzer sounds on the *missed* edge, to bring a carer to the device, who then opens carer mode → DOSE HISTORY to see who missed. It never sounds at the patient — a device that beeps at someone who has already not responded is nagging, not helping. | Nothing. The hardware is on the board and untouched (`HAL_SAI_MODULE_ENABLED` is still commented out), and the firmware hook already exists: `schedule_service()`'s `SCHED_FLAG_CLOSE` branch in `state_machine.c` fires exactly once per missed window and is where the `MISSED:` line is written. | Small. One call at that branch, plus SAI bring-up, plus the rule that nothing ever plays from handler context. This also satisfies the Program Plan's "audio and/or visual alert feedback", which `PROGRAM_PLAN_RECONCILIATION.md` §3 currently records as dropped. |
| **Multi-hopper** — the 6–8 hopper architecture | Session 17 builds one. The data model and `dispenser_dispense()` were kept extensible on purpose. | The mechanical build, and a `hopper_id` field in `PatientRecord` (which forces `patients.dat` to v4). The UI already draws four hopper slots with three greyed out. | Mostly mechanical. The firmware change is genuinely additive. |
| **Encrypted SD storage** | Prototype scope. It is the largest real privacy gap in the build. | An answer to key storage on a part with no secure element — which is a design question, not an implementation one. Without it, encryption moves the problem rather than solving it. | Small to write, hard to justify until the key question is answered. |
| **Pill classification** — the Program Plan's original core function | Substituted by face recognition; see `PROGRAM_PLAN_RECONCILIATION.md` §1. **No longer blocked on memory** — Session 15 freed 539 KB of ROM and proved a 220 KB NPU-reachable arena. Blocked on *time*, and on this project's history of losing most of three sessions to NPU toolchain problems. **Note that Session 16 does NOT close this**: a single-class detector finds *a* pill; it does not identify *which* medication, which is what the plan promised. It narrows the gap honestly and no further. | The arena (done), a labelled multi-medication dataset that does not exist, and a session's calendar for the flashing and layout problems that will recur. `MEMORY_MAP.md` §5 is the runbook. | One session if everything goes right; three if it goes the way Sessions 08A/08B did. |
| **Action recognition — now scheduled as Session 16** | Ruled out twice, and both reasons turned out to be wrong. It was not the temporal model's frame ring (that never had to be in SRAM — there is 16 MB of NPU-reachable PSRAM at `0x90000000`), and it was not training cost: **a collaborator has built and trained the vision half** (`tools/action_recogntion/`). It is not the temporal CNN this project always assumed — it is a YOLOv8n single-class **pill detector**, geometric features, and a **rule-based state machine**, of which only the detector needs the NPU. Mouth tracking is free: the CenterFace detector already emits both mouth corners on a tensor this firmware defines and has never read. **Decided: it corroborates the "I Took It" button, never replaces it** — the button stays the confirming action and the model's verdict becomes evidence in the log, behind `MEDSIGHT_ACTION_RECOGNITION` so it stays cuttable. | The **camera**, not the model. The DCMIPP DMAs into the display framebuffer and the camera is stopped for the whole dispense flow, because Session 09 found that resuming it overwrites the UI. Watching a patient during the confirm screen means the camera writing to PSRAM while the UI keeps drawing — plus re-deriving the `LPEN` question for that destination, which is the fault that cost six rounds in Session 12. | See `prompts/session_16.md`. `MEMORY_MAP.md` §5 is the runbook for the model; the camera path is the unknown. |
| **Caregiver notifications off-device** | Zero-network is a permanent design principle of this project, not an omission. | A deliberate connectivity and privacy decision — which would change what this project *is*, not just what it does. | Out of scope by choice, not by capacity. |
| **Persisting the passcode lockout across a power cycle** | Deliberate: it would turn a wrong tap into an SD write, and give anyone a way to wear the card out or lock a device out permanently by pulling power at the right moment. | A place to keep a small counter that is neither the SD card nor volatile — the RTC backup registers are the obvious candidate and are already in use for the clock-set marker. | An hour, once someone decides the trade is worth it. |

## Privacy and security posture

No network stack exists on this device. Wi-Fi, Ethernet and BLE are never
initialised, so there is no code path capable of transmitting anything
off-device — a stronger guarantee than a policy, because it is not that the
device chooses not to transmit, it is that it cannot. Face embeddings are
never written to UART or to any log; only names, gallery slot indices, dose
counts, schedule times and match confidence scores are. That rule is checked
against a **real captured UART log** from the noisiest build that exists, not
just by reading the source.

The patient record is five fields: a validity flag, a name, a 128-byte
embedding, a dose size, and up to four dose times. No phone number, no
address, no date of birth, no medical history. What is stored for face
recognition is a quantised embedding, not the enrolment photograph — the
camera frame that produced it is never written to the card.

Full posture, including the gaps, in
[`documents/COMPLIANCE_PRIVACY_POSTURE.md`](documents/COMPLIANCE_PRIVACY_POSTURE.md).
Third-party components and their licences are inventoried in
[`documents/THIRD_PARTY_SOFTWARE.md`](documents/THIRD_PARTY_SOFTWARE.md).

## Documentation

Start with
[`MASTER_PROJECT_PLAN.md`](documents/MASTER_PROJECT_PLAN.md),
[`PROGRAM_PLAN_RECONCILIATION.md`](documents/PROGRAM_PLAN_RECONCILIATION.md)
(what we promised against what we built),
[`SOFTWARE_ARCHITECTURE.md`](documents/SOFTWARE_ARCHITECTURE.md) and
[`MEMORY_MAP.md`](documents/MEMORY_MAP.md).

Every development session has a prompt (`documents/prompts/session_NN.md`)
and, once complete, a notes file
(`documents/milestones/session_NN_notes.md`) recording what actually happened
on real hardware — including the bugs, the wrong theories, and the order they
were disproved in. Those notes are the honest record, not a summary written
afterwards.
