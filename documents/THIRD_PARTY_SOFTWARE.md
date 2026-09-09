# THIRD_PARTY_SOFTWARE.md — MedSight

**Purpose.** TRON Programming Contest 2026 rule 1.3 requires, for every piece of
existing software by others that a submitted program uses:

1. its **name, rights holder, acquisition method and function**, stated in the
   documentation;
2. **provision of that software** to the organizer, free of charge and in usable
   form, until roughly one week after the awards ceremony, so the program can be
   evaluated; and
3. a **written guarantee** that copyrights and other rights have been handled in
   accordance with the Application Rules.

Rule 1.3 also permits modifying µT-Kernel itself **provided the OS API
specification is not changed**. §4 below lists every modification this project
makes to the vendored µT-Kernel 3.0 BSP2 tree, with the reason for each, and
§5 is the explicit statement that no `tk_*` API signature or semantic was
altered.

This inventory was compiled in Session 12 **from the build, not from memory** —
walking `STM32CubeIDE/FSBL/Debug/objects.list` (321 objects), the `-l` and `-L`
flags on the actual link line, and the licence headers in the source tree — and
is accurate for the Session 12 firmware. Anything added later must be added
here too.

---

## 1. Summary — what this program is built from

| Layer | Component | Rights holder | Licence |
|---|---|---|---|
| RTOS | µT-Kernel 3.0 BSP2 | TRON Forum / Ken Sakamura | T-License 2.1 / 2.2 |
| CPU support | CMSIS (Core, Cortex-M55) | Arm Limited | Apache-2.0 |
| Device support | STM32N6xx CMSIS Device | Arm Limited, STMicroelectronics | Apache-2.0 |
| Peripheral drivers | STM32N6xx HAL/LL Drivers | STMicroelectronics | BSD-3-Clause |
| Board support | STM32N6570-DK BSP + components | STMicroelectronics | BSD-3-Clause |
| Camera ISP | STM32 ISP Library (`isp`) | LACROIX - Impulse, STMicroelectronics | ST SLA0044 |
| Camera ISP | eVision AE / AWB libraries (binary) | STMicroelectronics | ST SLA0044 |
| NPU runtime | ST Edge AI / X-CUBE-AI runtime (`ll_aton_*`, `NetworkRuntime1200_CM55_GCC.a`) | STMicroelectronics | ST SLA0044 |
| AI models | CenterFace detector, MobileFaceNet embedder (ST Edge AI generated) | STMicroelectronics | ST SLA0044 |
| Filesystem | FatFs R0.15 | ChaN | FatFs licence (BSD-style, 1-clause) |
| UI font | DejaVu Sans Bold Oblique (rasterised into `ui_assets.c`) | DejaVu authors; Bitstream Inc. | Bitstream Vera / DejaVu licence (permissive, embedding allowed) |
| C library | newlib-nano (via GNU Tools for STM32 14.3.rel1) | Red Hat, Inc. and contributors | BSD-style / GPL-compatible |

**Everything else in the firmware is original MedSight code** written across
Sessions 01-12: `main.c`, `ms_osal.c/.h`, `ai_vision.c/.h`, `sd_logger.c/.h`,
`sd_diskio.c`, `ui/state_machine.c`, `ui/gui_draw.c`, `ui/anime_ui.c`,
`ui/touch_driver.c`, `ui/registration_ui.c`, and the documentation set in
`documents/`.

---

## 2. Detailed inventory

### 2.1 µT-Kernel 3.0 BSP2 — the RTOS this entry runs on

| Field | Value |
|---|---|
| **Name** | µT-Kernel 3.0 BSP2 (micro T-Kernel 3.00.00 - 3.00.07 across the tree) |
| **Rights holder** | TRON Forum; Copyright (C) 2006-2024 by Ken Sakamura |
| **Licence** | T-License 2.1 and T-License 2.2 (both appear in the tree; per-file headers state which) |
| **Acquisition** | Cloned from the TRON Forum's public GitHub repository <https://github.com/tron-forum/mtk3bsp2_samples>, specifically `Examples/prj_stm32n6_cam` — an official sample that already targets this exact board (STM32N6570-DK / STM32N657X0, Cortex-M55). Its whole `mtk3_bsp2/` source tree was vendored unmodified into `FSBL/mtk3_bsp2/` in Session 11, together with that project's own proven `Debug/mtk3_bsp2/**/subdir.mk` build configuration. |
| **Function** | The real-time kernel. Provides tasks, message buffers, mutexes, event flags, cyclic handlers, delays and the dispatcher — everything `FSBL/Src/ms_osal.c` maps this application's OSAL onto. This is the component TRON Contest rule 1.1 requires. |
| **In the build** | ~234 of the 321 linked objects. Note the whole tree is compiled, including RX231/RA/NXP/XMC board variants this board will never use: every non-STM32N6 file is guarded end to end by `#if defined(MTKBSP_...)` (via `<sys/machine.h>`, selected by `-D_STM32CUBE_DISCOVERY_N657_`) and compiles to a genuinely empty translation unit. That is what the reference project's own verified build does, and reusing its file list was judged lower risk than re-deriving the subset by inspection. |
| **Modified?** | **Yes — six files. See §4.** |

### 2.2 CMSIS and STM32N6xx CMSIS Device

| Field | Value |
|---|---|
| **Name** | CMSIS Core (`core_cm55.h` and companions); STM32N6xx CMSIS Device (`stm32n657xx.h`, `system_stm32n6xx_fsbl.c`, `startup_stm32n657x0hxq_fsbl.s`) |
| **Rights holder** | Arm Limited (CMSIS Core); Arm Limited and STMicroelectronics (device layer) |
| **Licence** | Apache-2.0 (SPDX identifier present in the headers) |
| **Acquisition** | Shipped inside the STM32Cube FW_N6 firmware package, obtained as part of the `DCMIPP_ContinuousMode` ST example project this repository was founded on in Session 03. |
| **Function** | Cortex-M55 core definitions and intrinsics, the reset/startup code and interrupt vector table, and the SystemInit clock scaffolding. Session 12's power-saving and idle-accounting code uses CMSIS's `__WFI`, `__DSB`, `__ISB` and the DWT cycle-counter definitions directly. |
| **Modified?** | No. |

### 2.3 STM32N6xx HAL/LL drivers

| Field | Value |
|---|---|
| **Name** | STM32N6xx HAL Driver (85 source modules; DCMIPP, LTDC, DMA2D, SDMMC, I2C, UART, RCC, PWR, XSPI and the rest) |
| **Rights holder** | STMicroelectronics |
| **Licence** | BSD-3-Clause |
| **Acquisition** | STM32Cube FW_N6 package, via the `DCMIPP_ContinuousMode` example (Session 03). |
| **Function** | All peripheral access: the camera pipeline (DCMIPP + CSI), the display controller (LTDC), the 2D blitter (DMA2D) behind the mascot renderer, the SD card (SDMMC2), the touch controller bus (I2C2), the debug console (USART1), clocks and power domains. |
| **Modified?** | No. |

### 2.4 STM32N6570-DK BSP and BSP components

| Field | Value |
|---|---|
| **Name** | STM32N6570-DK board BSP; BSP components `imx335` (camera sensor), `gt911` (capacitive touch), `rk050hr18` (5" LCD panel), `mx66uw1g45g` (OSPI NOR flash), `aps256xx` (Hexadeca-SPI PSRAM), `Common` |
| **Rights holder** | STMicroelectronics |
| **Licence** | BSD-3-Clause |
| **Acquisition** | STM32Cube FW_N6 package (Session 03), plus the `aps256xx` PSRAM driver which was already present but only actually called from Session 08B onward (see `milestones/session_08B_notes.md` Addendum 4). |
| **Function** | Board-level init and device drivers: LEDs, user button, COM port retarget, touch-screen API, camera sensor configuration, external NOR flash (where the NPU weights live) and external PSRAM (which the MobileFaceNet embedder requires as activation scratch). |
| **Modified?** | No. |

### 2.5 STM32 ISP Library and eVision AE/AWB

| Field | Value |
|---|---|
| **Name** | `Middlewares/ST/STM32_ISP_Library/isp` (source: `isp_core.c`, `isp_algo.c`, `isp_services.c`) and `.../evision/Lib` (binary archives `libn6-evision-awb_iar.a`, `libn6-evision-st-ae_iar.a`) |
| **Rights holder** | LACROIX - Impulse and STMicroelectronics |
| **Licence** | ST SLA0044 ("Ultimate Liberty" software licence agreement) |
| **Acquisition** | STM32Cube FW_N6 package, via the `DCMIPP_ContinuousMode` example (Session 03). |
| **Function** | Runs the camera's auto-exposure and auto-white-balance loops against the DCMIPP's hardware statistics. `ISP_BackgroundProcess()` is the entire body of this firmware's highest-priority task. |
| **Modified?** | No. |
| **Note** | The link deliberately pulls the **IAR-toolchain** variants of the two eVision archives (`*_iar.a`) even though this is a GCC build; the `*_gcc.a` variants also ship in the same directory. This is inherited from the ST example's own project settings, has been the case since Session 03, and links and runs correctly — recorded here because it is surprising, not because it is wrong. |

### 2.6 ST Edge AI / X-CUBE-AI runtime (the NPU driver stack)

| Field | Value |
|---|---|
| **Name** | ST Edge AI runtime, version 1.1.3-262 (`LL_ATON_VERSION` in `ll_aton_version.h`): sources `ll_aton*.c`, `ll_sw_float.c`, `ll_sw_integer.c`, `ecloader.c`, `mcu_cache.c`, `npu_cache.c`, `lc_print.c`, `ai_device_adaptor.c`, plus the precompiled archive `NetworkRuntime1200_CM55_GCC.a` |
| **Rights holder** | STMicroelectronics |
| **Licence** | ST SLA0044 |
| **Acquisition** | X-CUBE-AI / ST Edge AI Core, obtained as part of ST's `x-cube-n6-ai-h264-usb-uvc` application package, via the reference project kept at `tools/PeleAB_repo/` (Session 08B; that folder was named `scratch/` at the time). |
| **Function** | Drives the Neural-ART accelerator: epoch-controller microcode loading, buffer/cache management and the synchronous `stai_*_run()` execution path. Configured `LL_ATON_OSAL_BARE_METAL` (`ll_aton_config.h`), which is why the three `ll_aton_osal_{freertos,threadx,zephyr}.c` files compile to genuinely empty translation units — they are in the build but contribute no code, and in particular the FreeRTOS-named one does **not** make this firmware depend on FreeRTOS. |
| **Modified?** | No. |

### 2.7 The two AI models

| Field | Value |
|---|---|
| **Name** | **CenterFace** face detector (`centerface_OE_3_3_1.onnx` → `fd.c`, `fd_ecblobs.h`, `stai_fd.c/.h`) and **MobileFaceNet** face embedder (`mobilefacenet_int8_faces_OE_3_3_1.onnx` → `faceid.c`, `faceid_ecblobs.h`, `stai_faceid.c/.h`), both INT8, both compiled to Neural-ART by ST's AtoNN compiler |
| **Rights holder** | STMicroelectronics (`Copyright (c) 2023-2024 STMicroelectronics` in every generated file) |
| **Licence** | ST SLA0044 — `tools/PeleAB_repo/Model/LICENSE.md` places that repository's whole `Model/` directory under SLA0044 |
| **Acquisition** | Copied verbatim in Session 08B from the `Model/` directory of ST's `x-cube-n6-ai-h264-usb-uvc` application (kept locally at `tools/PeleAB_repo/`; that folder was named `scratch/` at the time), together with its prebuilt constant-data blobs `fd_data.xSPI2.bin` and `faceid_data.xSPI2.bin`. The generated sources record their own provenance in their header comments (`--onnx-input`, `--network-name`, `--json-quant-file`), which is where the model identities above were confirmed. |
| **Function** | CenterFace locates a face in the camera frame; MobileFaceNet turns that face crop into a 128-dimensional embedding. MedSight's own `ai_vision.c` does everything else — the frame snapshot, the CenterFace box decode, L2 normalisation and int8 quantisation of the embedding, cosine-similarity gallery matching, and SD-card persistence. |
| **Modified?** | The generated sources were edited in two mechanical ways only, both documented in `milestones/session_08B_notes.md`: internal helper functions were made `static` to resolve multiple-definition errors when both models are linked together, and large weight arrays were tagged `__attribute__((section(".xspi2")))` so they land in external OSPI NOR, which the NPU's data masters can actually reach. No model weights or graph structure were altered. |

> **Documentation correction (settled in Session 12).** `AI_PIPELINE.md` described
> the pipeline as "CenterFace detector + FaceID embedder" and an earlier draft of
> `prompts/session_13.md` said "SCRFD + MobileFaceNet". Checked against the
> generated sources' own `--onnx-input` lines: the detector is **CenterFace**
> (so `session_13.md` was wrong about SCRFD) and the embedder is
> **MobileFaceNet** (so `AI_PIPELINE.md` was quoting the ST *wrapper* name,
> `stai_faceid`, not the architecture). The correct description is
> **CenterFace + MobileFaceNet**, and both documents have been corrected.

### 2.8 FatFs

| Field | Value |
|---|---|
| **Name** | FatFs — Generic FAT Filesystem module, R0.15 (revision ID 80286) |
| **Rights holder** | ChaN — `Copyright (C) 2022, ChaN, all right reserved` |
| **Licence** | The FatFs licence (a one-clause BSD-style permissive licence); full text at `Middlewares/Third_Party/FatFs/LICENSE.txt` |
| **Acquisition** | Downloaded from the official FatFs site and integrated manually in Session 06 (this project has no `.ioc`, so the usual CubeMX middleware wiring was not available — see `ENGINEERING_LESSONS.md`). |
| **Function** | FAT32 filesystem on the microSD card. Everything MedSight records — `events.log`, the patient gallery `patients.dat` — goes through it. Configured `FF_FS_REENTRANT 0`, so FatFs' own OS-mutex layer (`ffsystem.c`) is entirely compiled out; `sd_logger.c` serialises all SD access itself through a single logger task instead. |
| **Modified?** | No. The block-device glue underneath it (`FSBL/Src/sd_diskio.c`) is MedSight's own code, not part of FatFs. |

### 2.9 Toolchain and C library

| Field | Value |
|---|---|
| **Name** | GNU Tools for STM32, 14.3.rel1 (`arm-none-eabi-gcc`) with newlib-nano (`--specs=nano.specs`) |
| **Rights holder** | Free Software Foundation; Red Hat, Inc. and newlib contributors; STMicroelectronics (packaging) |
| **Licence** | GCC under GPLv3 with the GCC Runtime Library Exception; newlib under its collection of BSD-style licences. Neither imposes any licence condition on this firmware. |
| **Acquisition** | Bundled with STM32CubeIDE 2.1.1. |
| **Function** | Compiles and links the firmware. newlib-nano supplies `printf`, `memcpy`, `strncmp`, `sqrtf`, `lrintf` and `malloc` (the last used only by FatFs' `ff_memalloc`, and only when long filenames need a working buffer). |
| **Note for the organizer** | Reproducing the build needs STM32CubeIDE 2.1.1 or the equivalent standalone toolchain. Nothing in the firmware depends on the IDE at runtime; the project also builds from the command line — see `milestones/session_11_notes.md`. |

---

## 3. Provision to the organizer (rule 1.3, item 2)

Every component listed above is publicly and freely obtainable, and none of it
requires a paid licence to evaluate this program:

- **µT-Kernel 3.0 BSP2** — public GitHub repository of the TRON Forum,
  <https://github.com/tron-forum/mtk3bsp2_samples>. The exact tree used is
  vendored into this submission at `FSBL/mtk3_bsp2/`, so the organizer does not
  need to fetch it separately.
- **CMSIS, STM32N6xx HAL/LL, STM32N6570-DK BSP, STM32 ISP Library, eVision
  archives** — free of charge from STMicroelectronics as part of the STM32Cube
  FW_N6 package and STM32CubeIDE. The specific files used are included in this
  submission's source tree.
- **ST Edge AI runtime and the two generated models** — free of charge from
  STMicroelectronics (X-CUBE-AI / ST Edge AI Core, and the
  `x-cube-n6-ai-h264-usb-uvc` application package). The generated sources and
  the prebuilt weight blobs are included in this submission at `FSBL/Src/ai/`
  and `STM32CubeIDE/FSBL/Debug/weights_flash/`.
- **FatFs** — free of charge from <http://elm-chan.org/fsw/ff/>. Included at
  `Middlewares/Third_Party/FatFs/`.
- **GNU Tools for STM32 / newlib** — free of charge with STM32CubeIDE.

The submitted archive therefore contains, in usable form, every piece of
third-party software the program needs in order to be built and evaluated, and
these will remain available to the organizer until at least one week after the
awards ceremony.

---

## 4. Modifications to µT-Kernel (rule 1.3's modification allowance)

Rule 1.3 permits modifying µT-Kernel provided the OS API specification is not
changed. Six files in the vendored `FSBL/mtk3_bsp2/` tree differ from the
upstream `tron-forum/mtk3bsp2_samples` `Examples/prj_stm32n6_cam` copy. The
list below is complete and was produced by a full recursive diff of the two
trees, not from memory. Every other file in the tree — roughly 230 of them,
including the entire `mtkernel/kernel/` directory — is **byte-identical to
upstream**.

Five of the six are integration fixes found the hard way during Session 11's
migration; `milestones/session_11_notes.md` carries the full investigation for
each. Every one is a case where the vendored BSP's defaults were correct for
*its own* reference project and wrong for *this* one, most often because that
reference project runs with the Cortex-M55 caches disabled and this one cannot.

| # | File | Session | Change | Why |
|---|---|---|---|---|
| 1 | `config/config.h` | 11 | `CNF_SYSTEMAREA_END` 0 → `0x34100000` | `0` means "use the system default", which resolves to `INTERNAL_RAM_END` (`0x341FFF00`) — the end of the entire physical AXI SRAM0 bank, not this project's linker-script RAM region (which ends at `0x34100000`). Left alone, the kernel's `imalloc` heap could hand `tk_cre_tsk` a task stack sitting on top of this project's own `.text`. Symptom was a silent hang into `Default_Handler` with no UART output at all. |
| 2 | `config/config.h` | 11 | `CNF_TIMER_PERIOD` 10 → `1` | At 10 ms, `tk_dly_tsk()` granularity coarsened every task's period, **and** `ms_osal.c`'s cyclic HAL-tick bridge made `HAL_GetTick()` run 10× slow rather than 10× coarse — so every `HAL_GetTick()`-based UI deadline stretched tenfold (250 ms mascot frame gate → 2.5 s; 500 ms touch dwell guard → 5 s). 1 ms is inside this port's own declared `MIN_TIMER_PERIOD..MAX_TIMER_PERIOD` of 1..50 and matches the tick the pre-migration FreeRTOS build used. |
| 3 | `config/config.h` | 11 | `USE_TMONITOR` 1 → `0` | The BSP's T-Monitor debug console (`tm_com.c`) reprograms USART1's `CR1`/`CR2`/`CR3`/`BRR` by hardcoded physical address, with a baud divisor computed for a different clock tree — silently taking over the HAL-managed UART this project has used for `printf` since Session 02. This project has its own console and never uses T-Monitor's, so disabling it removes a redundant second UART driver rather than a feature. |
| 4 | `include/sys/sysdepend/stm32_cube/cpu/stm32n6/sysdef.h` | 11 | `N_INTVEC` 196 → `195` | This project's ST-generated startup file has exactly 195 IRQ vectors (verified as `.isr_vector` = 844 bytes = 16 system + 195 IRQ entries via `size -A` on the linked `.elf`). At 196, `knl_start_mtkernel()`'s ROM→RAM vector-table copy read one word past the end of the real table. |
| 5 | `sysdepend/stm32_cube/cpu/core/armv8m/sys_start.c` | 11 | Two additions: an 8 KB `MS_NEWLIB_HEAP_RESERVE` gap before `knl_lowmem_top`, and an `ms_osal_clean_dcache()` call on `knl_exctbl` after the `VTOR` relocation | (a) newlib's `_sbrk()` and µT-Kernel's `knl_init_Imalloc()` both started allocating at the linker symbol `_end`, with no coordination — every `printf()` that grew stdio's buffer overwrote kernel allocator state. The matching constant lives in `STM32CubeIDE/FSBL/Application/User/sysmem.c`; both files comment-reference each other. (b) The kernel builds its exception vector table at runtime in write-back cacheable AXI SRAM with no cache maintenance anywhere in the BSP, because its own reference project runs with both CPU caches **disabled** — this project has run with them enabled since Session 03. Without the clean, the CPU's exception-entry vector fetch read stale physical memory and no exception was ever delivered after the relocation. |
| 6 | `sysdepend/stm32_cube/cpu/core/armv8m/interrupt.c` | 11 | `ms_osal_clean_dcache()` call at the end of `knl_init_interrupt()`; `knl_default_handler()` prints through this project's `printf()` instead of the (now disabled) `tm_printf()` | Same cache-coherency fix as above, applied after the fourteen handler-slot writes. The `printf` change is a consequence of modification #3: with `USE_TMONITOR 0`, `tm_printf()` is a silent no-op, so the strong `knl_default_handler()` definition in this file printed nothing at all — an unhandled exception was completely silent. |
| 7 | `sysdepend/stm32_cube/cpu/core/armv8m/exc_hdr.c` | 11 | All CPU fault handlers print unconditionally, with `HFSR`/`CFSR`/`MMFAR`/`BFAR` register dumps | `EXCEPTION_DBG_MSG()` is gated on `USE_EXCEPTION_DBG_MSG && USE_TMONITOR`, so modification #3 turned every fault handler — HardFault, MemManage, BusFault, UsageFault, NMI, SVCall, DebugMon — into a bare silent `while(1)`. These handlers only ever run after the CPU has already faulted fatally, so they cost nothing at runtime and are the difference between a diagnosable crash and an indistinguishable hang. |
| 8 | `sysdepend/stm32_cube/power_save.c` | **12** | `low_pow()` forwards to `ms_osal_low_power_idle()` (this project's own `ms_osal.c`), which executes a race-free `WFI` (PRIMASK-guarded, BASEPRI cleared across the
sleep — a plain WFI cannot be woken there, because BASEPRI masks the very
SysTick that would wake it) and accumulates the cycles spent asleep | Upstream ships `low_pow()` **empty**, so µT-Kernel's dispatcher idle path (`dispatch.S`, `l_dispatch_110`) spun the Cortex-M55 at full clock whenever no task was runnable — which in this application is most of the time. Rule 1.4 names power saving explicitly as an evaluation criterion, and the hook was already wired; it simply did nothing. The body lives in `ms_osal.c` rather than in the vendored tree, following the precedent set by `ms_osal_clean_dcache()`, so the CMSIS dependency stays outside third-party code and this diff stays a single forwarding call. |

**Eight changes, six files.** Rows 1-3 are all in `config.h` (three separate
constants); rows 4-8 are one file each. The six files are `config/config.h`,
`include/sys/sysdepend/stm32_cube/cpu/stm32n6/sysdef.h`,
`sysdepend/stm32_cube/cpu/core/armv8m/sys_start.c`,
`.../armv8m/interrupt.c`, `.../armv8m/exc_hdr.c`, and
`sysdepend/stm32_cube/power_save.c` — which is exactly what the recursive diff
in §7 reports.

---

## 5. Statement on the µT-Kernel API specification (rule 1.3)

**No µT-Kernel 3.0 API specification was changed by this project.**

Concretely:

- Not one `tk_*` function's **signature** was altered — not its name, its
  parameter list, its return type, or its error codes. The entire
  `mtkernel/kernel/tkernel/` directory (`task_manage.c`, `messagebuf.c`,
  `mutex.c`, `eventflag.c`, `timer.c`, `memory.c` and the rest — every file
  that implements a system call) is **byte-identical to upstream**, verified by
  recursive diff.
- Not one `tk_*` call's **semantics** were altered. The eight modifications in
  §4 are confined to build-time configuration constants (`config.h`,
  `sysdef.h`), the boot/interrupt-plumbing layer (`sys_start.c`,
  `interrupt.c`), fault reporting (`exc_hdr.c`), and the BSP's own
  power-management hook (`power_save.c`). None of those files defines a system
  call.
- The application reaches the kernel **only** through `FSBL/Src/ms_osal.c`,
  which is MedSight's own OS abstraction layer. Every session since 07 has
  verified this by grep, Session 12 included: no `tk_*` call and no
  `<tk/tkernel.h>` include exists anywhere in `FSBL/Src/` or `FSBL/Inc/`
  outside that one file.

## 6. Rights guarantee (rule 1.3, item 3)

The author warrants that the copyrights and other rights in all software used
in this submission have been handled in accordance with the TRON Programming
Contest Application Rules. Specifically:

- Every third-party component listed in §2 is used under the licence its own
  rights holder grants, as identified in that section, and each is used within
  the scope those licence terms permit.
- No component was obtained by circumventing a licence, and none requires a
  fee, registration or NDA to obtain or to evaluate.
- The µT-Kernel modifications in §4 are made under the modification allowance
  of rule 1.3, are individually documented above with their reasons, and leave
  the OS API specification unchanged as stated in §5.
- All original MedSight code (§1) is the author's own work.
- No copyrighted or trademarked character, artwork or other third-party
  creative asset appears anywhere in the firmware, the UI, or the
  documentation. The on-screen mascot and every UI element are original
  designs — see `MASCOT_UI_DESIGN.md`, which has enforced this from Session 04
  onward.

---

## 7. Maintenance note

This document was written in Session 12, against that session's build. If a
later session adds, removes or upgrades any third-party component, or touches
another file in `FSBL/mtk3_bsp2/`, update §2 and §4 in the same session — the
inventory is only useful to the organizer if it is true. The §4 table can be
regenerated at any time with a recursive diff against the pristine upstream
tree:

```bash
diff -rq tools/mtk3bsp2_samples/Examples/prj_stm32n6_cam/extracted/prj_stm32n6_cam/FSBL/mtk3_bsp2 \
         sessions/session_NN/FSBL/mtk3_bsp2
```
