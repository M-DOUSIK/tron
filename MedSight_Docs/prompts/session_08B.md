# Session 08B — Face Recognition AI Pipeline (Clean Do-Over)

## How to Start This Session

Hello! We are doing a clean do-over of Session 08B for the MedSight project.

**Before writing any code or taking any action**, you must acquire full context:

1. **READ ALL DOCUMENTATION**: Read every markdown file in `MedSight_Docs/` —
   especially `MASTER_PROJECT_PLAN.md`, `SOFTWARE_ARCHITECTURE.md`,
   `HARDWARE_ARCHITECTURE.md`, `ENGINEERING_LESSONS.md`, and `AI_LESSONS.md`.

2. **READ PAST SESSION PROMPTS**: Read session prompts `session_01.md` through
   `session_08A.md` in `MedSight_Docs/prompts/` to understand what is already built.

3. **READ THE REFERENCE IMPLEMENTATION** (do this before touching any code):
   A working STM32N6 face recognition project lives at:
   `C:\Users\Dousik\Workspace\TRON\scratch\PeleAB_repo\`

   Read these files IN FULL before writing anything:
   - `Src/svc/nn_service.c` — the complete NPU pipeline: init, input prep, run, output parse
   - `Src/svc/face_gallery.c` — embedding storage and cosine-similarity matching
   - `Src/sysobj/src/embedding_store.c` — how embeddings are persisted
   - `Model/stai_fd.c` + `Model/stai_fd.h` — CenterFace face detector model wrapper
   - `Model/stai_faceid.c` + `Model/stai_faceid.h` — FaceID embedder model wrapper
   - `Model/faceid.c` — post-processing and embedding extraction logic
   - `Inc/svc/face_detect.h` — face detection output structure definitions

   These files run on this exact board. They are your ground truth.

---

## Project Rules (non-negotiable, from prior sessions)

| Rule | Detail |
|---|---|
| No `.ioc` files | Manual HAL only. Never use STM32CubeMX. |
| NPU weights in OSPI flash | Every large const weight array (>100KB) **must** have `__attribute__((section(".xspi2")))`. The linker maps this to OSPI NOR at `0x71000000`. Weights in `.rodata` overflow the 1023KB RAM. |
| `aiPreInitialize()` first | Call this before any `stai_*_init()`. It enables OSPI memory-mapped mode and NPU clocks. |
| DCache flush on inputs | `SCB_CleanDCache_by_Addr(input_ptr, size)` before every NPU run. |
| DCache invalidate on outputs | `SCB_InvalidateDCache_by_Addr(output_ptr, size)` after every NPU run before reading results. |
| `static` on model internals | Generated `forward_lite_*` functions must be `static` to avoid linker "multiple definition" errors when two models are compiled together. |
| No UART logging of embeddings | Face embeddings are biometric data. Never `printf` embedding bytes. |
| OSAL-safe | Use `ms_osal.h` only. No direct FreeRTOS API calls anywhere. |
| New session = new folder | Work was done in a folder originally named `session_09B` (the old `session_08B` had been deleted at the time this session started, forcing the collision-avoiding name below). That folder has since been renamed to `session_08B` — its correct, final name — once the deleted-folder collision was no longer a concern; see `ENGINEERING_LESSONS.md`'s note on this rename. All later docs (`session_08B_notes.md`, `session_09.md`, `session_09_notes.md`) refer to it as `session_08B`. |

---

## Your First Action — Create the Working Folder

**Editorial note (post-session):** this section is kept as a historical record of what
was actually run. At the time, `session_08B` had just been deleted, so this step
created the working folder under the collision-avoiding name `session_09B` instead.
That folder has since been renamed to `session_08B` — its correct, final name — so a
reader following this repo today will find the work at `sessions/session_08B/`, not
`sessions/session_09B/`.

Copy `session_08A` (the clean, proven NPU base) to a new folder called `session_09B`:

```powershell
Copy-Item -Path "C:\Users\Dousik\Workspace\TRON\sessions\session_08A" `
          -Destination "C:\Users\Dousik\Workspace\TRON\sessions\session_09B" -Recurse

# Delete stale .d files so make doesn't fail
Get-ChildItem -Path "C:\Users\Dousik\Workspace\TRON\sessions\session_09B" `
              -Recurse -Filter "*.d" | Remove-Item -Force
```

All further work happened in `session_09B` (now `session_08B`). The AI files replaced
during this session are in:
`C:\Users\Dousik\Workspace\TRON\sessions\session_08B\FSBL\Src\ai\`

---

## What session_08A Gives You (the clean base)

`session_08A` has:
- Working camera (IMX335 → DCMIPP → LTDC on RK050 display)
- Working touch UI with state machine
- Working FreeRTOS tasks via `ms_osal.h`
- Working FatFS SD card logger
- **Working NPU runtime** — `aiPreInitialize()` + `stai_runtime_init()` + a throwaway
  test model (`network.c`) confirmed running on NPU, not CPU
- `ai_vision.c` currently just runs the throwaway network in a loop

You will **replace** the throwaway model with the real face recognition pipeline.

---

## Objective

Implement a complete face recognition pipeline in `session_08B`:

1. **Face Detector**: runs on NPU, detects a face in the camera frame
2. **Face Embedder**: runs on NPU, extracts a 128-D embedding from the detected face
3. **Gallery matching**: compare embedding against stored patient embeddings using
   cosine similarity; return best match above threshold or "no match"
4. **SD card persistence**: save/load patient gallery from `patients.dat`

---

## Step-by-Step Implementation Plan

### Step 1 — Copy the PeleAB model files into session_08B

The PeleAB repo's models are already generated for STM32N6 and proven to work.
**Use them directly** — do not regenerate.

Copy these files from `C:\Users\Dousik\Workspace\TRON\scratch\PeleAB_repo\Model\`
into `session_08B\FSBL\Src\ai\`:

```
stai_fd.c          → face detector C wrapper
stai_fd.h          → face detector header
stai_faceid.c      → face embedder C wrapper
stai_faceid.h      → face embedder header
faceid.c           → post-processing / output parsing
faceid_ecblobs.h   → weight blob declarations
```

Also check if there are additional data files (e.g. `stai_fd_data.c`,
`stai_faceid_data.c` or similar) — copy those too. Read the headers to find
all includes.

### Step 2 — Fix multiple-definition symbols

After copying, open `stai_fd.c` and `stai_faceid.c` and add `static` to every
internal helper function. Specifically: any function defined in those files that is
NOT declared in the corresponding `.h` file. A quick way:

```powershell
# Preview which forward_lite_ functions exist (adjust pattern as needed)
Select-String -Path "session_08B\FSBL\Src\ai\stai_fd.c" -Pattern "^void forward_lite_"
```

Add `static` before each such function definition.

### Step 3 — Place weights in OSPI flash

Find every large `const` array in `stai_fd.c`, `stai_faceid.c`, `faceid.c` and any
`*_data.c` files. Any array that is more than a few KB must get:
```c
__attribute__((section(".xspi2")))
```
placed immediately before the `const` keyword. Example:
```c
// Before:
const uint64_t g_fd_weights_array[80000] = { ... };

// After:
__attribute__((section(".xspi2")))
const uint64_t g_fd_weights_array[80000] = { ... };
```

### Step 4 — Update `subdir.mk` to compile the new files

Open `session_08B\STM32CubeIDE\FSBL\Debug\AI\subdir.mk`.

Add entries for `stai_fd.c`, `stai_faceid.c`, `faceid.c`, and any `*_data.c` files.
Remove entries for the old throwaway `network.c` and `network_weights.c`
(you can keep the rest of the `ll_aton_*` files — they are the NPU runtime and are
still needed).

### Step 5 — Rewrite `ai_vision.c` and `ai_vision.h`

Replace the contents completely. The public API **must** be exactly:

```c
/* ai_vision.h — public API (do not change signatures; Session 09 depends on them) */

#pragma once
#include <stdbool.h>
#include <stdint.h>

#define EMBEDDING_SIZE    128
#define MAX_PATIENTS      10
#define PATIENT_NAME_MAX  32

typedef struct {
    uint8_t   valid;
    char      name[PATIENT_NAME_MAX];
    int8_t    embedding[EMBEDDING_SIZE];
    uint8_t   pill_count;
    uint8_t   pills_remaining;
} PatientRecord;

extern PatientRecord patient_gallery[MAX_PATIENTS];

/* Init — call once after aiPreInitialize(), before any pipeline call */
void  ai_vision_init(void);

/* Run the full pipeline on the frozen camera frame at 0x34200000.
   Writes a 128-byte embedding into out_embedding.
   Returns true if a face was detected and embedding extracted. */
bool  ai_vision_run_pipeline(int8_t *out_embedding);

/* Cosine similarity between two 128-D int8 embeddings. Returns [-1.0, 1.0]. */
float ai_vision_match_face(const int8_t *emb1, const int8_t *emb2);

/* Gallery operations */
void  gallery_init(void);
bool  gallery_save(void);
int   gallery_add_patient(const char *name, const int8_t *embedding, int pill_count);
int   gallery_find_best_match(const int8_t *embedding, float *out_confidence);
```

In `ai_vision.c`, implement `ai_vision_init()` using the PeleAB `nn_service.c` as your
reference. Key points:
- Initialize detector first: `stai_fd_init(...)`, then embedder: `stai_faceid_init(...)`
- Get input/output pointers via `stai_fd_get_inputs()` / `stai_fd_get_outputs()`
- Provide input buffers via `stai_fd_set_inputs()` if the model flags indicate inputs
  are not preallocated (check `STAI_FD_FLAGS` — if `STAI_FLAG_INPUTS` is set, inputs
  ARE preallocated; if not, you must provide your own buffer)

In `ai_vision_run_pipeline()`:
1. Read camera frame from `(uint16_t*)0x34200000` (800×480 RGB565)
2. Center-crop and scale to detector input size (check `STAI_FD_IN_1_HEIGHT` and
   `STAI_FD_IN_1_WIDTH` in `stai_fd.h`)
3. Convert RGB565 → the quantized format the model expects (check
   `STAI_FD_IN_1_SCALE` and `STAI_FD_IN_1_ZERO_POINT` in `stai_fd.h`)
4. `SCB_CleanDCache_by_Addr(input_buf, size)`
5. `stai_fd_run(fd_net, STAI_MODE_SYNC)`
6. `SCB_InvalidateDCache_by_Addr(output_ptr, size)` for each output
7. Parse CenterFace outputs using `faceid.c` logic from PeleAB as reference
8. If face found: scale/crop to embedder input size, run embedder, copy output
9. Return embedding

### Step 6 — Update state_machine.c AI call sites

The state machine in `FSBL/Src/ui/state_machine.c` calls `ai_vision_run_pipeline()`.
Ensure:
- `g_isp_suspend = true` is set BEFORE the call (freezes camera DMA)
- `g_isp_suspend = false` is set AFTER the call (resumes live feed)
- 3-retry loop with 500ms delay between attempts before giving up

### Step 7 — Build

```powershell
$env:PATH += ";C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin"
C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.100.202601091506\tools\bin\make.exe -j12 -C C:\Users\Dousik\Workspace\TRON\sessions\session_08B\STM32CubeIDE\FSBL\Debug all 2>&1
```

Fix ALL errors before moving on. Common errors and fixes:

| Error | Fix |
|---|---|
| `multiple definition of 'forward_lite_*'` | Add `static` to those functions in the generated `.c` files |
| `.rodata will not fit in region 'RAM'` | Add `__attribute__((section(".xspi2")))` to the large weight arrays |
| `undefined reference to 'stai_fd_get_info'` | Remove any `stai_fd_get_info()` call from your code — it's only compiled when `HAVE_FD_INFO` is defined in the model's own TU |
| `implicit declaration of 'SCB_CleanDCache_by_Addr'` | Add `#include "main.h"` to `ai_vision.c` |

---

## Memory Map (never forget)

| Region | Address | Size | Used For |
|---|---|---|---|
| AXISRAM1 (ROM) | 0x34180400 | 511 KB | Code, `.rodata` |
| AXISRAM2 (RAM) | 0x34000400 | 1023 KB | Stack, heap, BSS, buffers |
| AXISRAM3 | 0x34200000 | — | Camera framebuffer |
| OSPI NOR | 0x71000000 | 64 MB | **NPU model weights** (`.xspi2`) |

---

## Definition of Done

- [ ] `session_08B` folder created from `session_08A`
- [ ] PeleAB model files copied and integrated
- [ ] Build is 100% clean — zero errors, zero warnings
- [ ] UART shows `det_run done rc=0` and `emb_run done rc=0` (not hanging)
- [ ] With face: `"Detector: Face detected!"` prints
- [ ] Without face (cover lens): `"Detector: No face found."` prints  
- [ ] `MedSight_Docs/milestones/session_08B_notes.md` written with: which model files
  were used, confidence threshold and reasoning, NPU latency for detector + embedder

---

## What This Session Does NOT Do

- No physical motors, servos, or IR hardware (software-only prototype)
- No action recognition / gesture detection — deferred; "OK" button is the final UX
- No full UI integration beyond the existing AI call sites in `state_machine.c`
- No patient registration keyboard flow — that is Session 09
