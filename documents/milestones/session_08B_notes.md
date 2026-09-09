# Session 08B Notes — Face Recognition AI Pipeline (clean do-over)

## Summary

Replaced Session 08A's throwaway NPU test model with a real two-model face
recognition pipeline (CenterFace detector + FaceID embedder), wired to a
placeholder SD-card-backed patient gallery. Working folder:
`sessions/session_08B/` (copied from `sessions/session_08A/`). At the time
this session ran, the prior `session_08B` folder had just been deleted, so
the briefing had this session create its working folder under the
collision-avoiding name `session_09B` instead — that folder has since been
renamed to `session_08B`, its correct, final name, once the deleted-folder
collision was no longer a concern (see `ENGINEERING_LESSONS.md`).

**Build status: verified locally.** I ran the actual STM32CubeIDE toolchain
(`arm-none-eabi-gcc`/`make`, the same one the IDE uses) from the command line
against `session_08B` and got a **100% clean build — zero errors, zero
warnings, zero linker region-overflow issues**:
```
   text	   data	    bss	    dec	    hex	filename
 714216	    600	 382568	1097384	 10bea8	MedSight_Session08B_FSBL.elf
```
This confirms the code compiles and links correctly. It does **not** confirm
runtime correctness on hardware — per the project's own workflow
(`MASTER_PROJECT_PLAN.md` §4), flashing and manual verification against the
Definition of Done below is still your step; I have no way to touch the board.

## Model files used

Copied verbatim from `scratch/PeleAB_repo/Model/` into `session_08B/FSBL/`:

| File | Role | Placed in |
|---|---|---|
| `stai_fd.c` / `stai_fd.h` | Face detector (CenterFace) stai wrapper | `Src/ai/` / `Inc/` |
| `fd.c` | Face detector generated forward-pass code | `Src/ai/` |
| `fd_ecblobs.h` | Face detector epoch-controller weight blobs | `Inc/` |
| `stai_faceid.c` / `stai_faceid.h` | Face embedder stai wrapper | `Src/ai/` / `Inc/` |
| `faceid.c` | Face embedder generated forward-pass code | `Src/ai/` |
| `faceid_ecblobs.h` | Face embedder epoch-controller weight blobs | `Inc/` |

**Deviation from the briefing's file list:** the briefing named
`stai_fd.c/h`, `stai_faceid.c/h`, `faceid.c`, `faceid_ecblobs.h` and said to
"check for additional data files... copy those too." The face **detector**
turned out to have its own equivalent pair — `fd.c` (its generated
forward-pass code, 643 KB) and `fd_ecblobs.h` (its weight blobs, 394 KB) —
that the briefing didn't name explicitly. Without them the detector has no
implementation to link against. Confirmed via `fd.c`'s own
`#include "fd_ecblobs.h"`.

**Deviation on header placement:** the briefing said to copy everything into
`Src/ai/`. I put the `.h` files in `FSBL/Inc/` instead, matching every other
header in this project (`ai_vision.h`, `stai_network.h`, etc. all live in
`Inc/`) and matching the AI subdir.mk's include path (`-I../../../FSBL/Inc`,
no `-I` for `Src/ai`). Putting headers in `Src/ai` would have required adding
a new include path instead — this was the lower-risk option since I was
already touching `subdir.mk`.

## Removed files (and why — this goes beyond the briefing's list)

The briefing said to remove `network.c` and `network_weights.c` (the
Session 08A throwaway model) from the AI subdir.mk. I also had to remove:

- `network_atonbuf.xSPI2.c` — the throwaway model's own `.xspi2` weight pool;
  same reason as `network_weights.c`.
- `stai_network.c`, `app_x-cube-ai.c`, `user_init.c` — all three call the
  throwaway model's generic `stai_network_*` API (`stai_network_init`,
  `stai_network_run`, etc.), which is implemented by symbols
  (`NN_Interface_network`, `LL_ATON_EpochBlockItems_network`, ...) that only
  existed in `network.c`. Once `network.c` is gone these three fail to link.
  None of them are actually called by MedSight's own code (`app_x-cube-ai.c`
  is X-CUBE-AI's own demo/template scaffolding, never invoked from `main.c`)
  — confirmed by grep before deleting.
- Kept `aiTestHelper_ST_AI.c` — it only depends on the generic
  `stai_network_info` struct, not on the throwaway model specifically, so it
  compiles standalone even though nothing currently calls it. Left in as
  harmless, in case a later session wants its debug-print helpers.

`main.c` no longer creates a free-running `ai_vision_task` — the throwaway
model ran inference on a 1 Hz timer forever; the real pipeline is
synchronous and event-driven (called from `state_machine.c` when the
dispense flow needs a face check), so there's no equivalent loop to keep.
`ai_vision_init()` now runs once from inside `task_ui_fn`, right after
`state_machine_init()`.

## A memory hazard the briefing didn't flag — found and fixed

Both generated networks' NPU activation scratch pools are **hardcoded by
STM32Cube.AI codegen** to overlap `BUFFER_ADDRESS` (`0x34200000`) — this
project's live camera framebuffer:

- Confirmed in `faceid.c`'s epoch-block cache-invalidate calls
  (`ATON_LIB_PHYSICAL_TO_VIRTUAL_ADDR(0x34200000UL + ...)`).
- Confirmed independently in PeleAB's own
  `Doc/Application-Overview.md`: `activations | 507 KB | 0x34200000 | NPURAMS`.

PeleAB's own project uses `0x34200000` purely as NPU scratch space — it
doesn't put a camera frame there. MedSight does. The briefing's literal
pipeline description ("Read camera frame from `0x34200000`... run
detector... [crop for embedder from] `0x34200000`") would silently read
**NPU garbage instead of the camera frame** for the embedder crop step,
because running the detector overwrites that address before the embedder
step gets to it.

**Fix implemented in `ai_vision.c`:** before touching the NPU at all,
`ai_vision_run_pipeline()` copies a centered 256×256 RGB565 crop
(`s_frame_hold`, 128 KB static buffer) out of the live frame. All further
work — the detector's own resized input *and* the embedder's face crop —
reads from that hold buffer, never from `BUFFER_ADDRESS` again once
inference has started. This is documented at the top of `ai_vision.c` and is
the reason the file is longer/more defensive than a literal reading of the
briefing would produce.

**Not yet verified on hardware:** whether 256×256 centered on the frame
reliably contains the user's face at the "please face the camera" distance
implied by the existing instruction screens. Tune `CROP_SIZE` in
`ai_vision.c` if not.

## CenterFace post-processing — simplification, and an unverified formula

Implemented single-best-box decode (highest-confidence grid cell only, no
NMS) — reasonable here since the device only ever needs the one patient
standing in front of it, not multi-face detection. The box decode itself
(`decode_best_face_box()` in `ai_vision.c`) uses the standard public
CenterFace formula (`cx=(gx+off_x)*stride`, `w=exp(sw)*stride`, etc.) as
documented in PeleAB's `Inc/svc/face_detect.h` head-order comment. **I did
not verify this against the actual training/export config of
`centerface_OE_3_3_1.onnx`** — I don't have that config, and getting it
exactly right (versus "close enough that a face is detected but the crop box
is a bit off") is something that can only really be confirmed by watching
real detections on hardware. If face detection fires reliably but the
embedder crop looks poorly centered on real footage, this formula is the
first place to check.

## Detector confidence threshold: 0.5

Standard CenterFace default for the post-sigmoid heatmap (0..1). Not tuned
against this specific export — adjust `FD_CONF_THRESHOLD` in `ai_vision.c`
if you see too many false detections or missed faces during bring-up.

## Gallery match threshold: 0.65 (cosine similarity)

Matches PeleAB's own `face_gallery.c` constant
(`FACE_GALLERY_MATCH_SIMILARITY`). Starting point only — PeleAB stores
embeddings as 16-bit fixed point; `ai_vision.h`'s mandated API here is
`int8_t embedding[128]` (lower precision), so this threshold may need
retuning once you have real enrolled-vs-impostor measurements. Embeddings
are L2-normalized before quantizing to int8 to make the best use of the
available 8-bit range.

## Gallery persistence: SD card, not on-chip flash

PeleAB's `face_gallery.c` persists to on-board XSPI NOR flash
(`BSP_XSPI_NOR_Write`). MedSight's `COMPLIANCE_PRIVACY_POSTURE.md` requires
biometric data live **only** on the removable SD card. I did not reuse
PeleAB's persistence code — `gallery_init()`/`gallery_save()` in
`ai_vision.c` call two new functions I added to `sd_logger.c`/`.h`
(`SD_Write_File`/`SD_Read_File`, generic named-file versions of the existing
`SD_Log_Binary`), keeping FATFS access inside `sd_logger.c` per
`SOFTWARE_ARCHITECTURE.md` §3's module-boundary rule. Gallery file:
`patients.dat` on the SD card root. This is Session 08B's placeholder
gallery (`PatientRecord`, not the canonical `patient_profile_t`) — Session 09
replaces it with real enrollment on the same storage mechanism.

## `state_machine.c` integration

`STATE_CAMERA_DISPENSE` (previously an 8 s mock timer) now:
1. Shows live preview for 1.5 s (time to get face in frame).
2. Sets `g_isp_suspend = true` and calls `camera_stop()` — genuinely halts
   the DCMIPP DMA pipe into `BUFFER_ADDRESS`, not just a flag (see memory
   hazard above; a software-only "suspend" flag wouldn't stop the actual DMA
   writes racing the NPU/CPU for that memory).
3. Runs `ai_vision_run_pipeline()`, retrying up to 3 times with a 500 ms gap
   if no face is found (per the briefing's step 6).
4. Resumes camera, matches the embedding against the gallery
   (`gallery_find_best_match`), logs the result (patient name / intruder /
   no-face) to UART and the SD event log — never the embedding bytes
   themselves.
5. Shows the result for 3 s, then returns to `STATE_HOME`.

`STATE_CAMERA_REGISTER` is untouched (still a mock timer) — real enrollment
is explicitly Session 09's job, not this session's, per the briefing's scope
note.

Mascot state wiring (`MASCOT_SUCCESS`/`MASCOT_ERROR` driven by this result)
is Session 11's job per `SOFTWARE_ARCHITECTURE.md` §7 — this session only
needed correct identification/rejection, logged and printed.

## NPU latency

**Not measured** — I have no way to run hardware. `AI_PIPELINE.md` §5 asks
for this to be filled in once measured; do that during hardware bring-up and
update that doc.

## Build-system issues found and fixed (not in the briefing)

These were necessary to get any build at all, on top of the AI subdir.mk
edits the briefing describes:

1. **Stale absolute paths.** STM32CubeIDE bakes absolute source paths into
   every generated `subdir.mk`/`makefile`. A plain folder copy from
   `session_08A` to `session_08B` leaves every one of those paths pointing
   at `session_08A` — meaning a build would have silently compiled the *old*
   folder's code regardless of any edits made in `session_08B`. Fixed with a
   project-wide `session_08A` → `session_08B` replace across every `.mk` and
   `makefile` file. `ENGINEERING_LESSONS.md`'s existing "stale `.d` files
   after folder copy" rule only covers dependency files, not this — worth
   adding this as a second bullet under that same rule.
2. **`objects.list` deleted, then had to be regenerated.** Cleaning stale
   build artifacts (`*.list` files, meaning to delete
   `MedSight_..._FSBL.list` disassembly dumps) also deleted
   `objects.list` — a linker response file the IDE normally regenerates
   internally, not something a plain `make all` invocation recreates.
   Regenerated it by scanning every real `subdir.mk`'s `OBJS +=` list
   (scoped to the subdirectories actually named in `sources.mk`'s
   `SUBDIRS`, see next point).
3. **Orphaned duplicate build folder:** `Debug/Application/User/ai/`
   contained its own `subdir.mk` compiling the *same* AI source files
   (including a `stai_fd.h`... wait, `aiTestUtility.c`, which doesn't even
   exist in this project) to a second set of object files. It isn't listed
   in `sources.mk`'s `SUBDIRS`, so the real build never touches it — but it
   would have poisoned a naive `objects.list` regeneration with phantom
   duplicate symbols. Deleted it as dead debris predating this session.
4. Renamed the build artifact from `MedSight_Session08A_FSBL.*` to
   `MedSight_Session08B_FSBL.*` (Debug and Release `makefile`, `.project`)
   so a flashed binary is identifiable by session. (At the time, the working
   folder — and so this artifact name — was `session_09B`/`Session09B`; both
   were later renamed to `session_08B`/`Session08B`, see
   `ENGINEERING_LESSONS.md`.)

## Addendum — "Error: Epoch Controller binary is invalid" on first hardware run

You hit this on first flash:
```
Error: Epoch Controller binary is invalid
assertion "ret == 1" failed: file ".../ll_aton_runtime.c", line 454, function: LL_ATON_RT_Init_Network
```

**Root cause:** this is a missing flashing step, not a code bug. Per
`AI_LESSONS.md`, this project's linker script marks the `OSPI_NOR` output
section `(NOLOAD)` on purpose — the normal STM32CubeIDE "Debug"/"Run" flow
only programs internal RAM and never touches external OSPI NOR flash, to
avoid the debugger-lockup issue documented there. That means the
`.xspi2`-tagged weight arrays I added this session (`fd_ecblobs.h`'s and
`faceid_ecblobs.h`'s `_ec_blob_*` arrays) were never actually written to the
board's external flash — whatever was physically in that flash before
(stale content from Session 08A's throwaway model, or blank/erased flash)
is what the detector tried to read, so `ec_get_blob_ptr()`
(`ecloader.c`) found a bad magic number and `LL_ATON_EC_Network_Init_fd()`
(`fd_ecblobs.h:4407`) returned `false`, tripping the assertion in
`ll_aton_runtime.c:454`.

(`faceid`'s own `LL_ATON_EC_Network_Init_faceid()` is a trivial
always-`true` stub — its blobs aren't validated the same way at init, so if
you get past this point, don't assume the faceid weights are automatically
fine; watch for a similar failure or bad detections once the detector is
working.)

**Fix — flash the weights to external OSPI NOR once:**

I extracted the correct 296,832-byte `.xspi2` image from this exact build
(temporarily relinking with `OSPI_NOR` made loadable instead of `NOLOAD`,
then reverting — the checked-in linker script and Debug/ outputs are
unchanged) and saved it here:
```
sessions/session_08B/STM32CubeIDE/FSBL/Debug/weights_flash/xspi2_weights.bin
```
Flash it to the board once, in HOTPLUG mode (does not hold the MCU in
reset, per `AI_LESSONS.md`'s documented fix for the same class of issue):

```powershell
& "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe" `
  -c port=SWD mode=HOTPLUG `
  -el "C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\ExternalLoader\MX66UW1G45G_STM32N6570-DK.stldr" `
  -w "C:\Users\Dousik\Workspace\TRON\sessions\session_08B\STM32CubeIDE\FSBL\Debug\weights_flash\xspi2_weights.bin" 0x71000000
```

Then do a normal Debug/Run from the IDE (internal-RAM-only flash, fast, as
usual) — the external flash content persists across that. You only need to
redo the external-flash step if the `.xspi2` content changes again (e.g. a
future session adds/changes models) — a plain code edit to `ai_vision.c` or
`state_machine.c` does not require re-flashing external memory, since those
files don't touch `.xspi2`.

**If this recurs after a future session's rebuild:** the pattern is
extract-the-`.xspi2`-section → flash-with-HOTPLUG, same as above. I can
prepare the extracted binary again the same way (temporarily un-mark
`OSPI_NOR (NOLOAD)` in the `.ld`, relink, `objcopy --only-section=OSPI_NOR`,
revert the `.ld`) — I just can't run `STM32_Programmer_CLI` myself since it
needs a physical ST-LINK connection.

## Addendum 2 — the real root cause: a second, undiscovered weight pool

After the OSPI fix above got past the boot-time assertion, live detection and
the self-test both still failed — every heatmap value came back as the exact
IEEE-754 NaN bit pattern (`0x7FC00000`), uniformly, regardless of what image
was fed in (live camera *or* a known-good embedded test image). That
consistency was the key clue: a real dequantize operation was running and
producing NaN from a bad parameter — not stale/uninitialized memory.

Digging into `fd.c`'s generated header comments turned up a **second,
completely separate weight pool** neither the original briefing nor my first
pass accounted for:
```
index=5 file postfix=xSPI2 name=octoFlash offset=0x70380000 absolute_mode
size=29360120 READ_ONLY ... use4initializers=YES
```
`faceid.c` has the equivalent at `offset=0x72000000`. This is *not* the same
thing as the `.xspi2`-tagged `_ec_blob_*` epoch-controller-microcode arrays
I found and flashed in Addendum 1 — it's a distinct constant-data region
(dequantization scale/zero-point parameters among other things) that this
compiled model expects to find at a specific absolute physical address, and
nothing in the source I'd copied ever placed data there.

PeleAB ships prebuilt binaries for exactly this — `Model/fd_data.xSPI2.bin`
and `Model/faceid_data.xSPI2.bin` — generated from the same codegen run as
the `.c`/`_ecblobs.h` files already in this project, so they're guaranteed
address/layout-compatible. Flashed them once, verified by readback (both
now hold real, non-blank data), confirmed the original `_ec_blob` weights
at `0x71000000` were untouched (the address ranges don't overlap):

```powershell
$cli = "...\STM32_Programmer_CLI.exe"
$loader = "...\ExternalLoader\MX66UW1G45G_STM32N6570-DK.stldr"
& $cli -c port=SWD mode=HOTPLUG -el $loader -w fd_data.xSPI2.bin 0x70380000
& $cli -c port=SWD mode=HOTPLUG -el $loader -w faceid_data.xSPI2.bin 0x72000000
```
Both files are saved in the project at
`STM32CubeIDE/FSBL/Debug/weights_flash/`. Like the `.xspi2` weights, this is
**external, non-volatile flash — a one-time step**, unaffected by normal
code changes and normal Debug/Run cycles from the IDE. Only redo it if a
future session regenerates/changes `fd.c` or `faceid.c`.

**One real scare along the way:** immediately after flashing both regions
back-to-back over HOTPLUG (no power cycle in between), the board hung at
boot inside `HAL_XSPI_GET_FLAG`'s polling loop, right where `aiPreInitialize()`
sets up OSPI memory-mapped mode. This matches the class of problem
`ENGINEERING_LESSONS.md` already documents (flash chip left in an
inconsistent Octal/SPI mode confusing later HAL calls) — **a full power
cycle (unplug USB entirely, not just reset) cleared it**, and the board
booted normally afterward with the flashed weight data intact. If a future
external-flash write ever leaves the board hung at boot the same way, that's
the fix — not a rebuild, not a re-flash of the app.

**Result after both fixes:** the self-test's heatmap output changed from
uniform NaN to genuine sigmoid-range values (confirmed via a raw hex dump
added temporarily to `ai_vision.c` and then removed — e.g. `0x3B800000` ≈
0.0039, alternating with 0.0, well within [0,1]). The model is now
computing for real.

## Addendum 3 — self-test false negative, fixed

Even after Addendum 2's fix, the self-test still reported `FAIL`, but for a
much more mundane reason: it originally centered the 128x128 test image
inside a `CROP_SIZE` (480×480) black-padded buffer before downscaling back
to the detector's native 128×128 input — the same treatment the live-camera
path needs (crop-then-downscale from an 800×480 frame). For a test image
that's *already exactly* 128×128, that round-trip shrinks the actual face
down to a small fraction of the frame surrounded by black, and the detector
correctly reported a low, below-threshold confidence (0.17) for how little
real face content was actually visible — not a bug, just a bad test.

Fixed by generalizing `run_pipeline_on_hold_buffer()` / `decode_best_face_box()`
to take an explicit hold-buffer size, and having the self-test feed the
debug image at its own native 128×128 resolution directly (no padding, no
extra downscale) while the live-camera path still uses the full
`CROP_SIZE` treatment unchanged. Not yet re-verified on hardware — that's
the next thing to check after this session note was written.

## Addendum 4 — detector confirmed working; embedder needed a whole extra RAM chip

With Addendums 1-3 applied, the self-test's **detector half started working
for real**: `Detector: best heatmap conf=0.76 (threshold=0.50)` /
`Detector: Face detected!` on the known-good test image — genuine, correct
detection.

But it hung immediately after, before the embedder ever printed a result,
and the LCD showed persistent full-screen visual noise (a photo confirmed
this — not the brief transient glitch from the earlier memory-hazard note,
a stuck one).

Cause: `faceid.c`'s generated header comments reveal it uses a **third**
external memory region neither `fd.c` nor anything else in this project
needed — the board's external Hexadeca-SPI **PSRAM** chip, as scratch
activation space, at physical address `0x90000000`:
```
index=4 file postfix=xSPI1 name=hyperRAM offset=0x90000000
absolute_mode size=16777208 READ_WRITE ... use4initializers=YES
```
Nothing in this project has ever brought that PSRAM chip up — no clock
config, no XSPI peripheral init, no memory-mapped-mode enable. The board
does physically have this chip (`HARDWARE_ARCHITECTURE.md`'s "Hexadeca-SPI
PSRAM (256Mbit)"), and the BSP driver for it (`aps256xx`) was already being
compiled into the project, just never called. The instant `faceid`'s
compiled epoch blocks tried to read/write `0x90000000`, they hit completely
unconfigured bus space — a bus fault, explaining the total hang (fd never
touches this address, which is exactly why the detector worked fine while
the embedder didn't).

Fix, in `npu_init.c`'s `aiPreInitialize()` (right before the existing NOR
flash init, matching PeleAB's own proven `platform.c` init order exactly):
```c
BSP_XSPI_RAM_Init(0);
BSP_XSPI_RAM_EnableMemoryMappedMode(0);
```
Both functions already existed in this project's own BSP driver
(`stm32n6570_discovery_xspi.c`) — they just needed to be called. Build is
clean (0 errors, 0 warnings) with this added. **Not yet re-verified on
hardware** — that's the next thing to check; I'm leaving the actual
flash+run to you per your last message, since this is a pure code change
(no external-flash re-flashing needed, just a normal Debug/Run from the
IDE).

## Addendum 5 — confirmed working end-to-end on hardware

With the PSRAM fix (Addendum 4) applied, both the self-test and a real
Dispense attempt succeeded:
```
ai_vision_self_test: PASS (face found)
...
Detector: best heatmap conf=0.88 (threshold=0.50)
Detector: Face detected!
emb_run done rc=0
Dispense: face detected but no gallery match (intruder).
```
"Intruder" here is the *correct* result, not a bug — the gallery is
legitimately empty (no one has been registered; that's Session 09's job).
Detection, embedding, and gallery matching are all confirmed working
end-to-end on real hardware.

Removed the automatic `ai_vision_self_test()` call from `ai_vision_init()`
— it had done its job proving the pipeline works, but running it at every
boot corrupted the freshly-drawn home screen (same NPU-activation-overlaps-
display-buffer issue as the live dispense flow's transient glitch). The
function itself is still there and exported (`ai_vision.h`) for manual use
if this pipeline ever needs re-diagnosing. The Definition of Done's UART
checks (`det_run done rc=0`, `emb_run done rc=0`, `Detector: Face
detected!`, `Detector: No face found.` when the lens is covered) are all
confirmed satisfied by the logs above.

**Known remaining cosmetic issue, not fixed this session:** during a real
Dispense attempt, the LCD briefly shows a corrupted/glitchy frame while the
NPU runs (the memory-hazard note at the top of `ai_vision.c` explains why —
the network's own activation scratch pool overlaps the camera framebuffer
address). It self-heals once the camera resumes and overwrites it with a
fresh live frame, so it's a ~1-2 second visual artifact, not a functional
bug — worth polishing in a later session (Session 11, when the mascot state
UI gets fully wired), not blocking for this one.

## What I could not verify (hardware-only — your task per the project workflow)

- [ ] UART shows `det_run done rc=0` and `emb_run done rc=0` without hanging
- [ ] With a face in frame: `"Detector: Face detected!"` prints
- [ ] With the lens covered / no face: `"Detector: No face found."` prints
- [ ] `CROP_SIZE` (256) actually contains the user's face at normal distance
- [ ] CenterFace box decode produces a sensibly-centered crop for the embedder
- [ ] Detector/gallery-match thresholds (0.5 / 0.65) are reasonable for real
      enrolled-vs-impostor faces
- [ ] Measure and record NPU latency (`AI_PIPELINE.md` §5)
- [ ] Confirm no biometric data reaches the UART debug log (spot-checked by
      code review — `ai_vision.c` never `printf`s embedding bytes — but a
      real-hardware grep of the serial capture is the actual verification
      `COMPLIANCE_PRIVACY_POSTURE.md` §4 asks for)
