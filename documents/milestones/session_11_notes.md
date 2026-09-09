# Session 11 Notes — µT-Kernel 3.0 OSAL Migration

## Addendum 9 — full system works under µT-Kernel; then the "everything feels slower than FreeRTOS" regression: HAL's millisecond clock was running 10x slow

With Addendum 8's cleanup in place the board boots all the way through and
the entire application runs on µT-Kernel 3.0. Confirmed on hardware from the
UART capture: checkpoints 11→13, `usermain()` creating the log queue, the
HAL tick bridge and all four tasks (`cam_isp` tskid 2, `ui` 3, `logger` 4,
`heartbeat` 5), then `task_camera_isp: started.` / `task_ui: started.` /
`task_logger: started.` / `task_heartbeat: started.`, touch init, SD mount,
NPU init, and a complete exercise of the Session 09/10 flows —
register (face captured → keyboard → pill count → confirm →
`registration_ui: patient 'DOU' saved to slot 0` → `SD_Write_File(patients.dat): 1630 bytes`)
and dispense (`Detector: Face detected!` → `emb_run done rc=0` →
intruder rejection → logged). **The Session 10 dispense flow survived the
OSAL backend swap with zero application-code changes, which was the whole
point of the OSAL.**

The one problem left was that everything — mascot animation, button
response, general feel — was noticeably slower than the FreeRTOS build.

### Root cause: two compounding effects of `CNF_TIMER_PERIOD = 10`

`mtk3_bsp2`'s stock `CNF_TIMER_PERIOD` is 10 ms. FreeRTOS's tick in this
project was 1 ms. Every piece of application timing was written against the
1 ms assumption, and the 10 ms tick broke it twice over:

**1. `HAL_GetTick()` ran 10x SLOW — not 10x coarse.** This is the big one,
and the original "HAL tick bridge" section of these notes got it wrong.
`HAL_IncTick()` adds **one** to HAL's `uwTick` counter per call. The bridge
fired a `tk_cre_cyc` cyclic handler every `CNF_TIMER_PERIOD` = 10 ms and
called `HAL_IncTick()` **once** — so HAL's clock advanced 1 ms per 10 ms of
real time. The earlier note claimed this only meant "`HAL_GetTick()` now
advances in 10 ms steps," which would have been a genuinely harmless
coarsening; it was actually a 10x rate error, and every `HAL_GetTick()`-based
deadline in the UI stretched by 10x:

| Timing | Written as | Actually behaved as |
|---|---|---|
| Mascot frame gate (`anime_ui.h` `LUMIO_FRAME_MS`, 1000/4) | 250 ms → 4 FPS | 2.5 s → **0.4 FPS** |
| Touch dwell guard (`state_machine.c`, the Session 05 "touch bleed" fix) | 500 ms | **5 s** before a tap is accepted |
| Registration capture-failure dialog | 2500 ms | 25 s |
| Confirm-register / thank-you screens | 2000 / 1500 ms | 20 / 15 s |

The 5-second dwell guard is precisely what "buttons feel unresponsive" was,
and the 0.4 FPS frame gate is precisely what "the mascot animation is slow"
was. Neither is a scheduler-performance problem — the scheduler was fine;
the clock the UI reads was wrong.

**2. `tk_dly_tsk()` granularity.** `osal_delay_ms()` maps to `tk_dly_tsk()`,
which resolves to whole kernel ticks. `main.c`'s camera/ISP task sleeps
`osal_delay_ms(1)` per iteration — at a 10 ms tick that became ~10 ms, so
`ISP_BackgroundProcess()` ran at a tenth of its intended rate. The UI task's
`osal_delay_ms(10)` rounds up to the next tick boundary, roughly halving
touch poll responsiveness on top of the dwell-guard problem above.

### The fix

`CNF_TIMER_PERIOD` 10 → **1** (`mtk3_bsp2/config/config.h`). 1 ms is what
FreeRTOS's tick was, is inside this port's own declared
`MIN_TIMER_PERIOD`..`MAX_TIMER_PERIOD` range of 1..50
(`include/sys/sysdepend/stm32_cube/cpu/stm32n6/sysdef.h`), and costs exactly
the same 1 kHz timer ISR the FreeRTOS build already paid for. That single
constant fixes both effects at once — HAL's clock now advances 1 ms per real
millisecond, and `tk_dly_tsk()` regains 1 ms granularity.

`ms_osal.c`'s bridge was also hardened so the two can never silently
disagree again: `OSAL_HAL_TICK_PERIOD_MS` is now **derived** from
`CNF_TIMER_PERIOD` rather than hardcoded to a matching literal, and the
handler adds one HAL millisecond per millisecond of cyclic period. Retuning
the kernel tick later can now only make `HAL_GetTick()` coarser, never
wrong-rate.

Verified in the linked `.elf` rather than trusting the build (per the rule
added to `ENGINEERING_LESSONS.md` after Addendum 8):

- The `T_CCYC` static initializer reads
  `exinf=0, cycatr=3 (TA_HLNG|TA_STA), cychdr=0x3419800d, cyctim=0x00000001, cycphs=0`
  — the cyclic period really is 1 ms.
- `knl_start_hw_timer` computes its SysTick reload as
  `SystemCoreClock * 0x10624DD3 >> 6` — the standard divide-by-1000 magic,
  i.e. a 1 ms kernel tick. Under the old setting this would have been a
  divide by 100. So the constant propagated into the kernel itself, not just
  into the OSAL.

### Known cosmetic issue, unrelated to the tick (not fixed)

The UART capture contains one visibly garbled line
(`disk_write: wD_REGISTER` / `or 4225, count 1`) — two tasks' `printf()`
output interleaving mid-line. `printf` is not reentrant and this build links
`--specs=nano.specs` with no retargetable locking, so concurrent `printf`
from the UI and logger tasks can interleave. This is not a µT-Kernel
regression (FreeRTOS would do exactly the same) and it affects only the
debug console, never SD-card data. Worth noting because the `disk_read:` /
`disk_write:` tracing from Sessions 06/08B prints four lines per sector
operation — roughly 13 ms of blocking UART each at 115200 — which is pure
debug overhead in the logger task's hot path. Trimming it is an easy, real
win for Session 12's hardening pass, but it is Session 06-era code and out of
this session's scope.

---

## Addendum 8 — the last "hang" was the diagnostic instrumentation itself: a SysTick ISR slower than its own tick period, starving PendSV forever

The Addendum 7 build's hardware run settled everything at once. Two separate
results came out of one boot.

### Result 1 — exception delivery was genuinely broken, and the D-cache clean fixed it

Before Addendum 7's cache maintenance, probes planted as the literal first
instruction of both `knl_dispatch_entry` (PendSV) and `knl_systim_inthdr`
(SysTick) never fired — no exception of any kind was ever delivered after
the vector table was relocated. With the `ms_osal_clean_dcache()` calls in
place and **no other functional change**, the very first `set_basepri(0)` in
`knl_force_dispatch()` immediately delivered SysTick:

```
[MS_DIAG] force_dispatch: PENDSVSET written, ICSR=1440f000 (PENDSVSET=1 PENDSTSET=1 VECTPENDING=15), now set_basepri(0)
[MS_DIAG] knl_systim_inthdr (SysTick ISR) ENTERED
[MS_DIAG] knl_timer_handler: entered, ...
...
[MS_DIAG] knl_systim_inthdr (SysTick ISR) returning
```

The handler runs start to finish and returns cleanly. **Addendum 7's
diagnosis is confirmed on hardware**: a vector table built with ordinary
stores into write-back cacheable AXI SRAM is not visible to the CPU's
exception-entry vector fetch until it is cleaned to the point of coherency,
and this BSP does no cache maintenance anywhere because its own reference
project runs with both CPU caches disabled.

The state dump also cleared every other hypothesis outright, so none of them
need chasing again:

| Reading | Value | What it rules out |
|---|---|---|
| `IPSR` | `00000000` | Thread mode. Not stuck inside some higher-priority handler. |
| `DHCSR` | `05110000`, `C_MASKINTS=0` | No debugger interrupt mask. `C_DEBUGEN=0` — not even a debug session. |
| `CONTROL` | `0000000c` | Privileged, MSP-based, FPU active. Normal. |
| `PRIMASK`/`FAULTMASK` | `0`/`0` | Nothing masked architecturally. |
| `VTOR` / `vec[14]` / `vec[15]` | `34022c00` / `34180791` / `341c2c0d` | Read back **through the live VTOR pointer** — table content correct, PendSV Thumb bit set. |
| `SHPR3` | `10f00000` | SysTick `0x10`, PendSV `0xF0` — exactly as intended. |
| `MSP`/`MSPLIM` | `340ffec0`/`34000400` | Stack healthy, nowhere near its limit. |

### Result 2 — the remaining infinite loop is caused by the instrumentation, not by the port

After that first SysTick, the log shows `knl_systim_inthdr` entering,
completing, returning, and **immediately entering again**, forever. PendSV
never runs.

That is correct hardware behaviour, not a bug in the kernel. The
instrumented SysTick path printed **eight** diagnostic lines per tick
(`knl_systim_inthdr` entry/exit plus six `knl_timer_handler` step probes).
At 115200 baud with a blocking, polled UART, roughly 480 characters costs on
the order of **40 ms** — while `CNF_TIMER_PERIOD` is **10 ms**. So SysTick
re-pends several times over before its own handler can finish. On exception
return the core tail-chains to the highest-priority pending exception, and
SysTick (`0x10`) always outranks PendSV (`0xF0`). PendSV is the lowest
priority in the system by design — it is *meant* to run only when nothing
else wants the CPU — so a system timer that never yields starves it
permanently.

Every earlier symptom in this investigation is consistent with this once
Result 1 is accounted for: `knl_force_dispatch()` sets `PENDSVSET`, the
correct higher-priority exception wins the race, and the dispatcher simply
never gets a turn.

**There is nothing to fix in the port for this.** The fix is to remove the
instrumentation, which had already served its purpose.

### What was stripped, and what deliberately stayed

Restored byte-for-byte from the vendored reference tree
(`mtk3bsp2_samples/Examples/prj_stm32n6_cam`), so they now match upstream
exactly:

- `mtkernel/kernel/tkernel/timer.c` — the six per-tick probes. **The single
  worst offender.**
- `sysdepend/stm32_cube/cpu/core/armv8m/dispatch.S` — the
  `bl ms_diag_dispatch_entered` probe. Equally dangerous for a different
  reason: it sat in `knl_dispatch_entry`, so it would have `printf`-ed on
  **every context switch** for the life of the system.
- `sysdepend/stm32_cube/cpu/core/armv8m/cpu_cntl.c` — `knl_force_dispatch`'s
  state dump, `ms_diag_dispatch_entered`, and the hit counters.
- `mtkernel/kernel/tkernel/memory.c` — the `knl_Imalloc`/`knl_searchFreeArea`
  probes (which caught Addendum 5a).
- `mtkernel/kernel/tkernel/task_manage.c` — the `tk_cre_tsk` probes.

`sysdepend/stm32_cube/cpu/core/armv8m/interrupt.c` was restored to pristine
and then had **only the two real fixes** re-applied — the
`ms_osal_clean_dcache()` call at the end of `knl_init_interrupt()`, and
`knl_default_handler()`'s `printf()` (see Addendum 6). Its SysTick probes and
counter are gone.

Kept on purpose:

- **`exc_hdr.c`'s unconditional fault printing** (Addendum 5c) — this is not
  instrumentation, it is a genuine fix. Those handlers only ever print when
  the CPU has already faulted fatally, so they cost nothing at runtime, and
  without them `USE_TMONITOR 0` leaves every fault silent. Same for
  `interrupt.c`'s `knl_default_handler()`.
- **`ms_osal_clean_dcache()` and its two call sites** — the Addendum 7 fix.
- **One-shot boot checkpoints** in `sys_start.c` (A–G), `sysinit.c` (1–9),
  and `inittask.c` (11–13). These run exactly once each during startup,
  before any task exists, so they cannot starve anything. They stay for this
  verification run and come out in the final cleanup pass.

### A build-system trap worth recording

The first attempt at this cleanup produced a binary that still contained
every probe. `Copy-Item` (like `cp -p`) preserves the **source** file's
modification time, and the vendored reference files are older than the
`.o` files built from the instrumented copies — so `make` saw them as up to
date and skipped them entirely. The `.elf` shrank by 208 bytes and looked
plausible. Caught by checking the binary rather than trusting the build:
`arm-none-eabi-nm` still listed `ms_diag_dispatch_entered`,
`ms_diag_pendsv_hits`, `ms_diag_systick_hits`, and `strings` still found the
per-tick format strings. `touch`-ing the restored files and rebuilding fixed
it; re-verified afterwards that all three symbols and all four hot-path
format strings are gone, both `ms_osal_clean_dcache` call sites are still
linked, and the PendSV vector is still written as `0x34180791`.

This generalises: **after restoring a file from an older source, verify the
compiled artifact, not the build log.** It is the same failure family as
`ENGINEERING_LESSONS.md`'s stale-`.d`-file rule — make's timestamp model
quietly doing the wrong thing after a file operation that came from outside
the build.

---

## Addendum 7 — the reference project this BSP came from runs with **both CPU caches disabled**; this project does not (CONFIRMED ON HARDWARE)

Found while diffing this project's `main.c` against the vendored BSP's own
reference project, `mtk3bsp2_samples/Examples/prj_stm32n6_cam`
(`Core/Src/main.c`, lines 87–90):

```c
  /* Enable I-Cache---------------------------------------------------------*/
  // SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  // SCB_EnableDCache();
```

Both are **commented out** in the reference. This project enables both
(`FSBL/Src/main.c:183-184`) and has since Session 03 — the camera/NPU
pipeline needs them. Apart from that (and the obvious application
differences), the two `main()` functions are structurally the same: HAL
init → clocks → UART → DCMIPP/IMX335/LCD/ISP brought up and *started* →
`knl_start_mtkernel()`.

**Why that difference is not cosmetic.** `knl_start_mtkernel()`
(`sysdepend/stm32_cube/cpu/core/armv8m/sys_start.c`) builds µT-Kernel's
exception vector table at runtime: it copies the ROM `.isr_vector` into
`knl_exctbl[]` — an ordinary array in normal, **write-back cacheable** AXI
SRAM (`.mtk_exctbl`, verified in the map at `0x34022c00`, 1024-aligned) —
then points `VTOR` at it. `knl_init_interrupt()` (`interrupt.c`) later
overwrites entries 2–6 and 11–15 with the kernel's own handlers, including
`knl_exctbl[14] = knl_dispatch_entry` (PendSV). **Every one of those writes
is a plain store, with no cache maintenance anywhere in this BSP.** With the
D-cache enabled and in write-back mode, those entries can still be sitting
dirty in the cache when the CPU's exception-entry logic fetches a vector, so
the fetch can read stale physical memory instead — and nothing ever copies
`.mtk_exctbl`'s load image into RAM at boot (the ST startup code's data-copy
loop only covers `_sdata`.`_edata`, i.e. `.data`), so "stale" here means
whatever the previous image or reset state happened to leave at that
address, not zeros.

This is also the first time in this project's history that a vector table
has lived in runtime-written RAM at all: under FreeRTOS (Sessions 07–10)
`VTOR` still pointed at the linker-placed `.isr_vector`, whose contents come
from the flashed image and are therefore correct in physical memory by
construction. That is a concrete, structural reason PendSV delivery could
have worked for every prior session and stopped working only under
µT-Kernel — and it is exactly the class of bug the reference project could
never have hit, because it runs with the cache off.

**The fix**, three small pieces:

- `FSBL/Src/ms_osal.c` — new `ms_osal_clean_dcache(void *addr, uint32_t size)`,
  a thin wrapper over CMSIS's `SCB_CleanDCache_by_Addr()` plus `DSB`/`ISB`.
  Kept in this project's own code (not the vendored tree) so the CMSIS
  dependency stays on this side of the boundary, matching how every other
  kernel-specific decision in this OSAL is handled.
- `sys_start.c` — call it on `knl_exctbl` immediately after the
  `out_w(SCB_VTOR, ...)` relocation.
- `interrupt.c` — call it again at the end of `knl_init_interrupt()`, after
  the fourteen handler-slot writes.

Verified in the linked `.elf`, not just in source: `ms_osal_clean_dcache`
disassembles to a real `DCCMVAC` loop (`str.w r3, [r2, #616]` — SCB base +
0x268 — stepping 32 bytes per iteration, the Cortex-M55 line size) bracketed
by `dsb sy`/`isb sy`, and `knl_init_interrupt` calls it with `r1 = 844`
(= 211 vectors × 4). Also confirmed in the same disassembly that the PendSV
slot is written as `0x34180791` — Thumb bit set — so a missing Thumb bit is
*not* an additional problem here.

**Status: confirmed on hardware** — see Addendum 8. With these three calls
in place and no other functional change, the first `set_basepri(0)` in
`knl_force_dispatch()` immediately delivered a SysTick exception, where
previously no exception of any kind was ever delivered after the vector
table relocation. This was a real bug and the fix stays.

---

## Addendum 6 — the diagnostic build that answers "why is a pending, unmasked PendSV not being taken?"

Where the investigation stood: after Addendum 5's fixes, task creation
succeeds all the way through `tk_cre_tsk`/`tk_sta_tsk` for the *initial*
task (`sysinit.c` checkpoints 8 and 9). The next step,
`knl_force_dispatch()` (`cpu_cntl.c`) — which sets `ICSR.PENDSVSET` and then
`set_basepri(0)` to let the CPU take the pending PendSV — never delivers the
exception. Direct register readback showed `PRIMASK=0`, `FAULTMASK=0`,
`BASEPRI` going to 0, `SHPR3=0x10F00000` (SysTick=0x10, PendSV=0xF0, exactly
what `SCB_SHPR3_VAL` intends), `ICSR` with both PendSV (bit 28) and SysTick
(bit 26) pending, and `knl_exctbl[14]` correctly holding
`&knl_dispatch_entry`. Probes planted as the literal first instruction of
`knl_dispatch_entry` (via `dispatch.S`) and of `knl_systim_inthdr` never
fire.

Architecturally, only a short list of things can leave an exception pending
when it is unmasked by every architectural mask register, and **two of them
are invisible in `PRIMASK`/`FAULTMASK`/`BASEPRI`**, which is why the earlier
readbacks could not distinguish them:

1. **`IPSR != 0`** — the CPU is already inside an exception handler whose
   priority is at least as high as PendSV's, so PendSV stays pending until
   that handler returns. Nothing printed so far proved we were in Thread
   mode.
2. **`DHCSR.C_MASKINTS` (bit 3)** — a debugger can mask PendSV, SysTick and
   all external interrupts. It survives a resume, and it does not show up in
   any architectural mask register. A debug session that left this set would
   reproduce these symptoms exactly.
3. The exception is taken after all, and the *handler* is what hangs — in
   which case control would never reach the line after `set_basepri(0)`.

`knl_force_dispatch()` was rewritten to settle all three in one boot:

- A full pre-dispatch state dump: `IPSR`, `CONTROL`, `PRIMASK`, `FAULTMASK`,
  `BASEPRI`, `MSP`, `PSP`, `MSPLIM`, `ICSR` decoded into `VECTACTIVE` and
  `VECTPENDING` (what the NVIC itself thinks is pending), `VTOR`, `SHPR3`,
  `SHCSR`, `CCR`, and `DHCSR` decoded into `C_DEBUGEN`/`C_HALT`/`C_STEP`/
  `C_MASKINTS`.
- The PendSV and SysTick vectors read **through the live `VTOR` pointer**
  rather than through the `knl_exctbl[]` symbol — that is what the CPU
  itself would fetch, which the symbol read is not.
- `DSB`/`ISB` after both the `PENDSVSET` write and `set_basepri(0)`, then a
  re-read, so the "still running" line is unambiguous rather than a
  pipeline-timing artifact.
- A `cpsie i` / `cpsie f` last-resort probe and one more `ICSR` read after
  it.

`ms_diag_dispatch_entered()` (the `dispatch.S` probe) now increments a plain
`volatile` counter, `ms_diag_pendsv_hits`, **before** its `printf()`, and
`knl_systim_inthdr()` does the same with `ms_diag_systick_hits`. Both
counters are printed by `knl_force_dispatch()` after the fact. This
separates "the handler was never entered" from "the handler was entered but
its `printf()` went nowhere" — a distinction the previous printf-only probes
could not make.

One real gap closed while here: `knl_default_handler()` is defined **twice**
— a `WEAK_FUNC` version in `exc_hdr.c` and a **strong** one in
`interrupt.c`. The strong one wins the link, and it printed via
`tm_printf()`, which is a silent no-op now that `USE_TMONITOR` is `0`
(Addendum 3). So the "Undefined Exception" message Addendum 5 believed it
had restored in `exc_hdr.c` was still dead in the actual binary.
`interrupt.c`'s version now prints through this project's own `printf()`
with the active `IPSR`.

---

## Addendum 5 — three more real bugs, all verified against the compiled `.elf`

These were found and fixed after Addendum 4 but before this session ran out
of context to write them up; recorded here for completeness. All three were
confirmed by inspecting the linked binary (`arm-none-eabi-objdump`, `nm`,
`size -A`) rather than by reading source alone.

### 6a. `_sbrk()` and µT-Kernel's `knl_init_Imalloc()` both allocated from `&_end`

**The bug.** Newlib's `_sbrk()`
(`STM32CubeIDE/FSBL/Application/User/sysmem.c`) starts its heap at the
linker symbol `_end` and grows up toward the MSP stack.
`knl_start_mtkernel()`'s `USE_IMALLOC` block computes
`knl_lowmem_top = max(INTERNAL_RAM_START, SYSTEMAREA_TOP, &_end)` — which,
in this project's memory map, is also `&_end` — and hands that same address
to `knl_init_Imalloc()` as the base of the *kernel's* heap. Two allocators,
same starting address, zero coordination. Every `printf()` that lazily grows
newlib's heap (stdio allocates its per-stream buffer on first use) silently
overwrote whatever the kernel's allocator had just placed there.

**How it was caught.** Not by inference — by planting diagnostic prints
inside `knl_Imalloc()`/`knl_searchFreeArea()`'s own free-list header
(`mtkernel/kernel/tkernel/memory.c`) and watching a `printf()` call a few
lines later corrupt it.

**The fix.** A fixed 8 KB arena reserved for newlib immediately after
`_end`, agreed on by both sides:
`NEWLIB_HEAP_RESERVE` in `sysmem.c` caps `_sbrk()`'s ceiling at
`&_end + 8 KB`; `MS_NEWLIB_HEAP_RESERVE` in `sys_start.c` starts
`knl_lowmem_top` 8 KB higher. Both files carry a comment pointing at the
other, since the two constants must stay equal.

### 6b. `N_INTVEC` was 196; this project's vector table has 195 IRQ slots

`sysdepend/stm32_cube/cpu/stm32n6/sysdef.h` shipped with `N_INTVEC 196`.
`knl_start_mtkernel()`'s ROM→RAM copy loop runs `N_SYSVEC + N_INTVEC`
iterations, so at 196 it read one word **past the end** of the real table
and copied whatever followed it into `knl_exctbl[]`'s last slot.

Verified against the binary rather than by counting entries in the startup
`.s` file: `arm-none-eabi-size -A` reports `.isr_vector` as exactly **844
bytes = 211 words = 16 system exceptions + 195 IRQs**, ending at
`LTDC_UP_ERR_IRQHandler`. Fixed to `N_INTVEC 195`.

Worth noting for anyone comparing against the upstream BSP: this project's
`startup_stm32n657x0hxq_fsbl.s` is byte-identical to the reference camera
sample's, so the same off-by-one existed there too — it simply never
produced a visible fault on their board.

### 6c. Every CPU fault handler was silent — a side effect of disabling T-Monitor

Addendum 3 set `USE_TMONITOR = 0` to stop the BSP's own serial driver from
re-programming USART1 out from under this project's HAL-managed UART. That
fix is correct and stays. But it had a consequence that was missed at the
time: `exc_hdr.c`'s `EXCEPTION_DBG_MSG()` macro is gated on
`USE_EXCEPTION_DBG_MSG && USE_TMONITOR`, so **every** fault handler —
HardFault, MemManage, BusFault, UsageFault, NMI, SVCall, DebugMon — became a
bare `while(1)` printing nothing.

That matters far beyond cosmetics: for the whole middle of this session, a
genuine CPU fault would have been indistinguishable from every other silent
hang already being chased. All of `exc_hdr.c`'s handlers now print
unconditionally through this project's own `printf()`, with `HFSR`/`CFSR`/
`MMFAR`/`BFAR` register dumps, prefixed `[MS_DIAG] FAULT:`. (See Addendum 6
for the *second* half of this gap — `knl_default_handler()`, whose strong
definition in `interrupt.c` was still silent.)

---

## Diagnostic instrumentation currently in the tree

All of it is marked `TEMPORARY diagnostic instrumentation (MedSight Session
11 bring-up)`. **Strip it once the root cause is confirmed fixed** — and
when doing so, touch nothing else in these files:

**Updated after Addendum 8** — everything on a hot path has been removed
(it was actively causing the remaining hang; see Addendum 8). What is left:

| File | What remains | Why |
|---|---|---|
| `sysdepend/stm32_cube/cpu/core/armv8m/sys_start.c` | checkpoints A–G | one-shot, pre-task; remove in final cleanup |
| `mtkernel/kernel/sysinit/sysinit.c` | checkpoints 1–9, scheduler-state and vector-table verification prints | one-shot, pre-task; remove in final cleanup |
| `mtkernel/kernel/inittask/inittask.c` | checkpoints 11–13 | one-shot, runs once in the init task; remove in final cleanup |
| `sysdepend/stm32_cube/cpu/core/armv8m/exc_hdr.c` | all fault handlers print unconditionally | **keep** — a real fix (Addendum 5c), only ever prints on a fatal fault |
| `sysdepend/stm32_cube/cpu/core/armv8m/interrupt.c` | `knl_default_handler()` prints; `ms_osal_clean_dcache()` call | **keep** — both are real fixes (Addenda 6 and 7) |

Restored byte-for-byte to the vendored upstream, with no instrumentation
left at all: `mtkernel/kernel/tkernel/timer.c`,
`mtkernel/kernel/tkernel/memory.c`,
`mtkernel/kernel/tkernel/task_manage.c`,
`sysdepend/stm32_cube/cpu/core/armv8m/dispatch.S`,
`sysdepend/stm32_cube/cpu/core/armv8m/cpu_cntl.c`.

`ms_osal_clean_dcache()` in `FSBL/Src/ms_osal.c` and its two call sites are
**not** instrumentation — that is the Addendum 7 fix and stays permanently.

---

## Open robustness item (not a current bug, noted while reading the memory map)

`CNF_EXC_STACK_SIZE` is `0`, so
`knl_lowmem_limit = CNF_SYSTEMAREA_END - 0 = 0x34100000` — which is exactly
`_estack`, the top of the MSP stack. The kernel's heap allocates upward from
`knl_lowmem_top` (`0x340c5500` after the 8 KB newlib reserve), so with ~235
KB free and this project's small object set it will not reach the stack in
practice. But nothing structurally prevents it: the linker script reserves
`_Min_Stack_Size` (0x800) for the MSP stack and the kernel's allocator has
no idea. If this ever needs hardening, setting `CNF_EXC_STACK_SIZE` to at
least `0x800` makes `sys_start.c`'s own arithmetic reserve it. Left alone
for now rather than changing allocator behaviour in the middle of a boot-hang
investigation.

---

## The command-line build loop used throughout this session

No STM32CubeIDE GUI needed to iterate — only to flash and run (or use
`STM32_Programmer_CLI` per `AI_LESSONS.md`):

```bash
export PATH="$PATH:/c/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin"
"/c/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.100.202601091506/tools/bin/make.exe" -j12 -C "C:/Users/Dousik/Workspace/TRON/sessions/session_11/STM32CubeIDE/FSBL/Debug" all
```

---

## Addendum 4 — the actual boot-hang root cause: a missing `SysTick_Handler`

Bisected with LED checkpoints planted at every stage of `main()` (since UART
was itself a casualty of this bug — see Addendum 3) down to: checkpoints
after `HAL_Init()`/clocks/UART-init passed, but the very next thing that
needed real elapsed time to pass — a plain diagnostic `HAL_Delay()` —
never returned.

**Root cause:** `stm32n6xx_it.c` has carried this comment since Session 07:
*"SysTick_Handler is defined in ms_osal.c."* That was a real, working
design — the FreeRTOS-era `ms_osal.c` genuinely defined it. This session's
rewrite of `ms_osal.c` dropped that function (µT-Kernel handles SysTick
itself, but only **after** `knl_start_mtkernel()` runs), without adding a
replacement for the window before that point. Net effect: **no
`SysTick_Handler` existed anywhere in the linked program.** Its vector fell
back to the startup file's weak default — literally `Default_Handler: b
Infinite_Loop`, an interrupt handler that never returns. The instant
`HAL_Init()`'s `HAL_InitTick()` enables the SysTick interrupt (line one of
`main()`, functionally), the first tick fires straight into that infinite
loop and the CPU never comes back — which is exactly the very first symptom
ever reported for this session ("stuck in `Default_Handler`," zero UART
output), from the first hardware test onward. Every other bug fixed in this
session's earlier addenda was real and worth fixing, but none of them were
*this* bug — this one had been there since the first build.

**The fix:** restored a plain
```c
void SysTick_Handler(void) { HAL_IncTick(); }
```
in `ms_osal.c`, explicitly scoped (in its own comment) to the pre-kernel
phase only — `knl_init_interrupt()` overwrites this exact vector with its
own `knl_systim_inthdr()` the moment `knl_start_mtkernel()` runs, so this
function and the post-kernel `tk_cre_cyc` HAL-tick bridge (see the main
migration notes above) hand off cleanly with no overlap.

Verified directly in the compiled binary, not just by reading source:
disassembled `.isr_vector` before and after — the SysTick slot changed from
pointing at `Default_Handler`'s address to pointing at the new, real
`SysTick_Handler` symbol.

## Addendum 3 — a second UART-adjacent bug, found once the first one was fixed: T-Monitor's own serial driver hijacking USART1

With the SysTick fix in place, UART output started working again — as far
as printing every diagnostic line up through `knl_main()` being called —
and then went silent again, mid-boot, with no further symptoms (the board
kept running; LCD activity continued, just glitched from the earlier
vector-table relocation freezing DMA mid-frame). This looked like a second
hang, but wasn't: it was the *same* class of problem — vendored BSP code
touching a resource (USART1) this project already owns and manages itself.

**Root cause:** `knl_main()`'s very first step, `libtm_init()`, calls
`tm_com_init()` —
`sysdepend/stm32_cube/lib/libtm/discovery_stm32n657/tm_com.c`, the BSP's
own **T-Monitor** (a separate, ROM-monitor-style debug console this vendored
kernel supports) serial driver. That function writes USART1's `CR1`/`CR2`/
`CR3`/`BRR` control registers directly, by hardcoded physical address
(`0x52001000`), with a baud-rate divisor (`0x022C`) computed for whatever
clock tree the reference camera sample runs at — not this project's actual
`SystemClock_Config()`. This project already has its own fully-working,
HAL-managed USART1 (`BSP_COM_Init()`, Session 02) that every `printf()` in
the whole codebase depends on, including every diagnostic line this
session's debugging added. `tm_com_init()` silently re-programming those
same registers out from under HAL's own UART state is exactly why output
stopped cold right at that call, with no crash, no fault, nothing else
visibly wrong — the board carried on executing just fine, just mute.
(`libtm.c`'s `tm_getchar()`/`tm_getline()` additionally contain outright
blocking reads on that same UART, `while ((UART_ISR & ISR_RXNE) == 0);` —
not reached during a normal boot, but a second, latent hazard from the same
root cause, since this project has no T-Monitor terminal client ever
sending it a byte.)

**The fix:** `mtk3_bsp2/config/config.h`'s `USE_TMONITOR` flipped from `1`
to `0`. This project never uses T-Monitor's interactive console — it has
its own OSAL, its own UART, its own printf-based logging throughout every
prior session — so there's no feature loss, only the removal of a redundant
second UART driver this vendored kernel happened to ship enabled by
default. Traced every other `USE_TMONITOR` call site first to confirm
nothing this project needs depends on it: the only other guarded code is a
one-line version-banner print in `inittask.c` (`"microT-Kernel Version
%x.%02x"`, gated by `USE_SYSTEM_MESSAGE && USE_TMONITOR` together) and the
`SYSTEM_MESSAGE()` macro used by the kernel's own internal fatal-error
paths (`sysinit.c`'s `"!ERROR! ..."` messages) — both become silent no-ops,
neither is something this project's own code calls or relies on. Verified
in the compiled binary: `tm_com_init`, `tm_snd_dat`, `tm_rcv_dat`, and
`libtm_init` no longer exist as symbols anywhere in the link.

## Addendum 2 — silent hang into `Default_Handler`, zero UART output: a real memory-map bug

After the `.project`/`.cproject` fix below let the project build cleanly in
the real STM32CubeIDE, flashing and debugging it hung immediately with the
PC stuck in the startup file's `Default_Handler` infinite loop and **no
UART output at all** — not even the very first task's `"started."` print.
Since `main.c` runs entirely unchanged, bare-metal HAL/camera/LCD bring-up
identical to Session 10, and this codebase has no boot-time printf before a
task actually starts running (the first possible UART line is a task body's
own first statement), "no output" only says the hang happens somewhere
between `main()` starting and the first task executing its first line — it
doesn't say whether that's still in HAL init or already inside the kernel.
Working that back from the memory map (not a guess) found a real bug:

**The bug:** `mtk3_bsp2/config/config.h` ships with
`CNF_SYSTEMAREA_END = 0` ("use system default"). Tracing
`sysdepend/stm32_cube/cpu/core/armv8m/sys_start.c`'s `knl_start_mtkernel()`,
a `0` here makes the kernel's dynamic-memory ceiling
(`knl_lowmem_limit`, the upper bound `tk_cre_tsk`'s task stacks,
`tk_cre_mbf`'s buffers, and `tk_cre_cyc`'s control block all get carved out
of) fall back to `INTERNAL_RAM_END`, which
`include/sys/sysdepend/stm32_cube/cpu/stm32n6/sysdef.h` defines as
`0x341FFF00` — the end of the **entire physical AXI SRAM0 bank** (2047KB).
That is not the same thing as this project's own **linker-script** "RAM"
region: `STM32N657X0HXQ_AXISRAM2_fsbl.ld` only claims the first 1023KB of
that same physical bank (`RAM ORIGIN=0x34000400 LENGTH=1023K`, ending at
`0x34100000`) — the rest of that bank, `0x34100000` through `0x34200000`
(where the camera framebuffer begins, per `SOFTWARE_ARCHITECTURE.md`'s
memory table), is where *this project's own linker script puts its `ROM`
region* (`ROM ORIGIN=0x34180400 LENGTH=511K`, i.e. `.text`/`.rodata` — the
running program's own code).

Left at the vendored default, the very first object `usermain()` creates
(the log queue, a `tk_cre_mbf`) can get handed memory anywhere between
`_end` and `0x341FFF00` — a range that includes this project's own code
region. The kernel would be free to place a task stack or a message-buffer
backing store directly on top of running instructions, corrupting them with
no warning and no UART line, which matches the observed symptom exactly:
a fault so early that literally nothing printed, landing wherever
`Default_Handler` happened to be the nearest resolvable symbol once
execution went off into the weeds.

This is not a hypothetical — `sys_start.c`'s `knl_lowmem_top`/
`knl_lowmem_limit` computation and this project's own linker script are
both plain, readable source; the two numbers just don't agree, and nothing
before this session had ever exercised `mtk3_bsp2`'s dynamic allocator to
surface the mismatch (the reference `prj_stm32n6_cam` sample this BSP was
vendored from has its *own*, smaller-and-differently-based RAM region — see
the main migration notes above — so its identical `config.h` default never
hit this problem on their board).

**The fix**, one line in `FSBL/mtk3_bsp2/config/config.h` (a vendored file,
but this is exactly the kind of project-specific integration constant this
BSP's `config.h`/`config_bsp.h` split exists to let a project override, the
same way this project's own `FreeRTOSConfig.h` was a customization point
before it — not something to leave untouched as read-only third-party
code):

```c
#define CNF_SYSTEMAREA_END  0x34100000  /* was 0 ("use system default") */
```

This pins the kernel's dynamic-memory ceiling to exactly where this
project's own linker script's `RAM` region actually ends, so `tk_cre_mbf`/
`tk_cre_tsk`/`tk_cre_cyc` can never allocate into the `ROM` (code) region,
the gap before it, or the camera framebuffer beyond it. Rebuilt via the
command-line toolchain after this change — same clean result as before (the
`.elf` size is identical, as expected: this changes a runtime heap-ceiling
*value*, not anything the linker or compiler sees). This is a source-level
fix; I still have no way to reflash and confirm the actual boot sequence
completes on hardware — see the "What I could not verify" list at the
bottom, unchanged by this addendum.

## Addendum 1 — real STM32CubeIDE build failed; root cause was `.project`/`.cproject`, not the source

The first version of this session verified the migration by invoking
`arm-none-eabi-gcc`/`make` directly against a hand-edited `Debug/` tree
(`sources.mk`, `makefile`, and ~112 new `subdir.mk` files, all edited
directly with a script). That command-line build was genuinely clean. But
when you opened the actual project in STM32CubeIDE and built it, it failed:

```
make: *** No rule to make target '.../Middlewares/Third_Party/FreeRTOS/Source/event_groups.c',
needed by 'Middlewares/FreeRTOS/event_groups.o'.  Stop.
```

**Root cause:** this project's `.project` file lists every compiled source
file as an individual Eclipse *linked resource* (`<link>` entries — 9 of
them still pointed at `Middlewares/Third_Party/FreeRTOS/...`, which the
first pass had deleted from disk but not un-linked from the project), and
`.cproject` still listed the FreeRTOS include paths as project-wide compiler
settings. STM32CubeIDE's own build regenerates `Debug/sources.mk`,
`Debug/makefile`, and every `subdir.mk` **from `.project` + `.cproject`**
before every build — it does not trust whatever is already sitting in
`Debug/`. My first-pass hand-edits to `Debug/` were therefore real and
correct, but invisible to the actual IDE: it silently discarded them and
regenerated its own (still FreeRTOS-referencing, still mtk3_bsp2-blind) set
the moment you clicked Build. This is exactly the failure mode
`ENGINEERING_LESSONS.md` already warned about generically ("the fix is in
project settings, not the Makefile") — I initially treated it as a
generated-file problem instead of the project-settings problem it actually
was, verified with the wrong tool (raw command-line make instead of the
actual IDE), and didn't catch it because I have no way to open the real
STM32CubeIDE GUI from here.

**The actual fix**, in `STM32CubeIDE/FSBL/.project` and `.cproject`:

- `.project`: removed the 9 individual `<link>` entries for
  `Middlewares/FreeRTOS/*.c`; added **one** folder-level link (`<type>2</type>`,
  the same kind the pre-existing `AI` entry already uses) for `mtk3_bsp2` →
  `PARENT-2-PROJECT_LOC/FSBL/mtk3_bsp2`. This project's `sourceEntries` in
  `.cproject` is a single unfiltered project-root path with no per-folder
  exclusions, so a folder link is all that's needed for Eclipse to pick up
  every file under `mtk3_bsp2/` automatically — no per-file links required,
  unlike the individual FreeRTOS links this replaces.
- `.cproject`: in **both** the Debug and Release configurations' C compiler
  tool settings, removed the two FreeRTOS include-path entries and added the
  same four `mtk3_bsp2` include paths used in the command-line verification
  (`mtk3_bsp2`, `mtk3_bsp2/config`, `mtk3_bsp2/include`,
  `mtk3_bsp2/mtkernel/kernel/knlinc`), plus the `_STM32CUBE_DISCOVERY_N657_`
  define that selects the STM32N6570-DK board profile in `sys/machine.h`.
  Also added the same define and include paths to the **assembler** tool in
  both configs (previously configured with zero include paths, since this
  project had no `.S` files before `mtk3_bsp2/.../dispatch.S`) — confirmed
  `dispatch.S` itself needs `sys/machine.h` (via `#include <sys/machine.h>`)
  for the same board-selection macro the C files need.

Both XML files were validated well-formed after editing. I could not
re-verify with a real STM32CubeIDE GUI build — I have no way to run one
from here — so **this is still the one thing you should check before
trusting the build**: in STM32CubeIDE, do a full **Project → Clean...**
(not just Build) so the IDE regenerates `Debug/sources.mk`, `Debug/makefile`,
and every `subdir.mk` fresh from the now-corrected `.project`/`.cproject`,
rather than mixing IDE-driven regeneration with the hand-edited `Debug/`
tree this session's command-line verification left behind. If the Clean
build reproduces the same `_STM32CUBE_DISCOVERY_N657_`/include-path setup
described above (visible in the compiler invocation lines in the build
console), the migration is confirmed correct at the project-settings level,
not just at the source-code level the first command-line build proved.



## Summary

Swapped the OSAL backend from FreeRTOS to µT-Kernel 3.0 by rewriting
`ms_osal.c` (and adding doc-only comments plus one new constant to
`ms_osal.h`) — per `prompts/session_11.md`'s objective, **no other file in
the project changed**. `main.c` is byte-identical to Session 10's; every
application module still talks to the OS only through `ms_osal.h`'s
unchanged public API. The dispense flow documented in
`milestones/session_10_notes.md` (face match → dispense animation →
"I Took It"/"Skip") is untouched code and, per the build below, still
compiles and links against the new kernel with no changes required anywhere
in `state_machine.c`, `gui_draw.c`, `ai_vision.c`, or `sd_logger.c`.

Working folder: `sessions/session_11/`, copied from `sessions/session_10/`
per the project's "new session = new folder" rule. Before editing, deleted
all `.d`/`.o`/`.su`/`.cyclo`/`.list`/`objects.list` build artifacts under
`Debug/`/`Release/` and text-replaced `session_10` → `session_11` only in
`subdir.mk`/`makefile`/`.project`/`.launch` files (never binaries), per the
two folder-copy lessons in `ENGINEERING_LESSONS.md`. Also fixed two
pre-existing stale-identity bugs found while doing this (carried since
Session 09 and never caught): `Debug/makefile`'s `BUILD_ARTIFACT_NAME` was
still `MedSight_Session09_FSBL`, and the `.launch` file's embedded ELF path
still said `Session09` — both now say `Session11`, matching `Release/makefile`
which at least had been kept current through Session 10.

**Build status: verified locally, 100% clean.** Ran the real command-line
toolchain (`arm-none-eabi-gcc 14.3.rel1` / `make 2.2.100`, same ones the IDE
uses) against `session_11/STM32CubeIDE/FSBL/Debug` — zero errors, only the
two pre-existing warnings in `ll_aton_profiler.c` (verified byte-identical
to Session 10's copy of that file — not introduced this session, and that
file is vendor AI runtime code this session doesn't touch):

```
   text	   data	    bss	    dec	    hex	filename
 736520	   4008	 656824	1397352	 155268	MedSight_Session11_FSBL.elf
```

This confirms the code compiles and links correctly against µT-Kernel; it
does **not** confirm runtime correctness on hardware — per
`MASTER_PROJECT_PLAN.md` §4, flashing and manual verification is still your
step; I have no way to touch the board. See "What I could not verify" below.

---

## Where the kernel came from

`ENGINEERING_LESSONS.md` already named the reference repo for this session:
`https://github.com/tron-forum/mtk3bsp2_samples`. That repo's
`Examples/prj_stm32n6_cam` is an official, TRON-Forum-published µT-Kernel 3.0
BSP2 project that already targets **this exact board** (STM32N6570-DK /
STM32N657X0, Cortex-M55) with a working camera+LCD demo — confirmed by its
own `include/sys/sysdepend/stm32_cube/discovery_stm32n657/` board profile.
Rather than hand-porting the BSP from a generic ARMv8-M reference or
guessing at STM32N6-specific config, I cloned it and vendored its entire
`mtk3_bsp2/` source tree unmodified into `FSBL/mtk3_bsp2/` — this is now
third-party BSP code, like `Middlewares/ST/STM32_ISP_Library` already was,
not something this project's sessions edit.

The reference project's own `Debug/mtk3_bsp2/**/subdir.mk` (already
generated by a real STM32CubeIDE build of that exact project — the repo's
`ReadMe_en.md` states "Operation has been confirmed") was copied wholesale
into `session_11/STM32CubeIDE/FSBL/Debug/mtk3_bsp2/` as the authoritative
list of which of the ~230 files under `mtk3_bsp2/` actually need compiling,
rather than re-deriving that list by inspection. All absolute source and
include paths inside those 112 `subdir.mk` files were then text-replaced
from the reference project's own folder layout to this project's (this
project splits IDE metadata from source — `STM32CubeIDE/FSBL/Debug/` vs.
`FSBL/Src/` — differently from the reference's flatter layout, so this
wasn't a pure path-prefix swap; each of the reference's per-component
include directories — `Core/Inc`, `Drivers/BSP/Components/IMX335`,
`Middlewares/STM32_ISP/inc`, etc. — was mapped to this project's equivalent,
already-existing directory, e.g. `Drivers/BSP/Components/imx335` (lowercase)
and `Middlewares/ST/STM32_ISP_Library/isp/Inc`).

**Why compile the whole tree, including RX231/RA/NXP/XMC variants this board
will never use:** the reference project's own proven build does exactly
that — every non-STM32N6 or non-ARMv8-M source file is guarded end-to-end by
`#if defined(MTKBSP_...)` (via `<sys/machine.h>`, selected by the
`-D_STM32CUBE_DISCOVERY_N657_` compiler define) and compiles to a genuinely
empty translation unit for every board this isn't. Trying to hand-pick "only
the STM32N6 files" would have meant re-deriving, by inspection, exactly
which subset the reference project's own working configuration actually
needs — strictly more risk than reusing their already-verified file list
as-is. This is a deliberate choice, not an oversight; see the objects.list
regeneration note below for the actual object count this produced.

---

## The real problem this session solved (not just an API rename)

`SOFTWARE_ARCHITECTURE.md` §4's OSAL table (`osal_task_create` →
`tk_cre_tsk`/`tk_sta_tsk`, `osal_delay_ms` → `tk_dly_tsk`, etc.) and
`prompts/session_11.md`'s literal implementation steps both frame this as a
mechanical 1:1 function-call swap. It isn't, for one structural reason that
neither doc anticipated:

**µT-Kernel object-creation syscalls (`tk_cre_tsk`, `tk_cre_mbf`,
`tk_cre_mtx`) can only be called after the kernel itself is running.** The
kernel comes up inside `knl_start_mtkernel()` — which is what
`osal_scheduler_start()` must call — and that function **never returns to
its caller**. The kernel then calls this BSP's `usermain()` (via its own
`inittask`) once it's ready to create objects.

But `main.c` — unchanged, per this session's own constraint — calls
`osal_task_create()` four times and `SD_Logger_Queue_Init()`
(→ `osal_queue_create()`) once, and only *then* calls
`osal_scheduler_start()` as the very last thing it does. Every single
creation call in this codebase happens **before** a µT-Kernel object could
possibly exist.

The fix, entirely inside `ms_osal.c` (no other file, including `main.c` and
`sd_logger.c`, needed to change): every `osal_*_create()` call made before
the kernel is running records its parameters into a small static pool (task
fn/arg/name/stack/priority; queue item_count/item_size; mutex — just a
slot) and hands back a stable pointer into that pool as the "handle" —
exactly the kind of value `main.c`/`sd_logger.c` already expect to receive
and hold onto (`sd_logger.c`'s `s_log_queue` is a plain file-static
variable, unaware it's holding a not-yet-real object). This file's new
`usermain()` — called by the kernel once it's alive, running as the BSP's
`inittask`, before any of *our* tasks exist — then walks that pool in a
fixed order and performs the real `tk_cre_mbf`/`tk_cre_mtx`/`tk_cre_tsk`
calls, patching each slot's real object ID in place:

1. **Queues first** — a task's very first line might `osal_queue_receive()`.
2. **Mutexes second** — none are actually called anywhere in this codebase
   today (grepped to confirm — see below), but created for the same reason.
3. **The HAL tick bridge** (see next section) — must be running before any
   task that touches a HAL peripheral (camera, SD) starts.
4. **Tasks last, and only then started** — by the time any task body
   actually runs, every queue/mutex it could possibly touch is already real.

A defensive branch also exists for the (currently unused) case of a future
`osal_task_create()`/`osal_queue_create()`/`osal_mutex_create()` call made
*after* the kernel is already running: it creates the real object
immediately instead of deferring to a `usermain()` pass that has already
happened. Not exercised by anything in this codebase, but leaving it out
would have made the OSAL silently wrong for any future caller that didn't
know about this session's pre/post-kernel distinction.

This is the one piece of this migration I'd flag as genuinely
non-obvious — it's not mentioned in `SOFTWARE_ARCHITECTURE.md` §4 or
`session_11.md` because both were written assuming the FreeRTOS-shaped
"create everything, then start the scheduler" flow generalizes to any RTOS.
It doesn't, for a kernel whose object-creation calls are only valid once a
task is already running.

---

## Two things that turned out to need *zero* changes (verified, not assumed)

`session_11.md`'s step 3 explicitly asks to "Update FatFS OS locking in
`ffsystem.c` to use µT-Kernel mutexes instead of FreeRTOS mutexes." I
checked rather than doing this reflexively, and it's unnecessary:

- **`ffsystem.c`'s entire `OS_TYPE == 3 /* FreeRTOS */` mutex block is
  compiled out.** `FF_FS_REENTRANT` is `0` in this project's `ffconf.h`
  (confirmed identical in both `FSBL/Inc/ffconf.h` and
  `Middlewares/Third_Party/FatFs/source/ffconf.h`), and the whole mutex
  section in `ffsystem.c` is guarded by `#if FF_FS_REENTRANT`. FatFS access
  in this project was never actually made reentrant at the FatFS layer —
  `sd_logger.c` serializes all SD access itself, by construction, through
  its single logger task. There was no FreeRTOS dependency here to replace.

- **The AI runtime (`ll_aton_*`) already runs in a bare-metal / synchronous
  mode that has nothing to do with FreeRTOS or µT-Kernel.**
  `ll_aton_config.h` line 96 reads `#define LL_ATON_OSAL
  LL_ATON_OSAL_BARE_METAL` — already selected, not something this session
  changed. The three `ll_aton_osal_{freertos,threadx,zephyr}.c` files in
  `FSBL/Src/ai/` each wrap their *entire* body in
  `#if (LL_ATON_OSAL == LL_ATON_OSAL_FREERTOS)` (etc.) — since that macro
  resolves to `BARE_METAL`, all three compile to genuinely empty
  translation units (confirmed in the Debug build: `ll_aton_osal_freertos.o`
  is `.text 0x0 .data 0x0 .bss 0x0` in the map file) and were never calling
  into FreeRTOS in the first place, even in Session 10. Removing the
  `Middlewares/Third_Party/FreeRTOS` folder therefore does not break these
  files despite `ll_aton_osal_freertos.c` containing `#include
  "ll_aton_osal_freertos.h"` (which itself `#include`s `"FreeRTOS.h"`) —
  that include is inside the dead branch and the preprocessor never reaches
  it. Left the file compiling (matches "no changes to AI models" from
  Sessions 09/10) rather than editing `AI/subdir.mk` to exclude a file that
  was already provably harmless.

Both are recorded here explicitly, per this project's "record deviations,
don't let them go silent" convention, since a reader following
`session_11.md`'s literal text would expect `ffsystem.c` to have changed and
would not expect three FreeRTOS-named files to still be part of the build.

---

## HAL tick bridge (a real gap `session_11.md` doesn't mention)

µT-Kernel owns SysTick outright — `mtk3_bsp2`'s own
`sysdepend/stm32_cube/cpu/core/armv8m/interrupt.c` wires exception vector
index 15 (SysTick) unconditionally to its own `knl_systim_inthdr()` inside
`knl_init_interrupt()`. This session's old FreeRTOS-era `SysTick_Handler()`
(which called both `xPortSysTickHandler()` and `HAL_IncTick()`) is simply
never invoked again once the kernel starts. Without a replacement,
`HAL_GetTick()`/`HAL_Delay()` — used internally by DCMIPP, the camera ISP,
and SDMMC2 HAL calls for timeouts — would silently freeze at their boot
value, a subtle failure mode that would look like a hardware hang, not an
OS migration bug.

Fixed inside `ms_osal.c` only (not by editing the vendored kernel's
`interrupt.c`, keeping every kernel-specific decision in this one file):
`usermain()` creates a µT-Kernel cyclic handler (`tk_cre_cyc`, `TA_STA` so
it starts immediately) that calls `HAL_IncTick()` once every 10 ms —
matching `mtk3_bsp2/config/config.h`'s `CNF_TIMER_PERIOD` (10), the BSP's
own internal tick period, rather than FreeRTOS's 1 ms tick.

**CORRECTED — this paragraph was wrong; see Addendum 9.** The original claim
here was that `HAL_GetTick()` "now advances in 10 ms steps instead of 1 ms,"
and that this was a harmless, deliberate trade-off because nothing in this
project needs sub-10 ms timeout precision. Both halves were mistaken.
`HAL_IncTick()` adds **one** to `uwTick` per call, so firing it once every
10 ms made HAL's millisecond clock run **10x slow**, not 10x coarse — every
`HAL_GetTick()`-based deadline in the UI stretched tenfold (the mascot's
250 ms frame gate became 2.5 s, the 500 ms touch dwell guard became 5 s).
The survey of call sites also only looked at `HAL_Delay()` and HAL peripheral
timeouts, and missed that `state_machine.c` and `anime_ui.c` read
`HAL_GetTick()` directly for all of their UI timing. `CNF_TIMER_PERIOD` is
now 1 ms, matching FreeRTOS's tick, and `OSAL_HAL_TICK_PERIOD_MS` is derived
from that macro so the bridge cannot drift from the kernel tick again.

---

## Priority mapping

`ms_osal.h`'s task-priority convention (unchanged doc: "1 = lowest, higher
= higher priority", the FreeRTOS convention this project's four
`osal_task_create()` calls in `main.c` were written against — 1=heartbeat,
2=logger, 4=ui, 5=cam_isp) is the *opposite* of µT-Kernel's, where priority
`1` is the **highest**. `ms_osal.c` inverts internally:
`itskpri = OSAL_PRI_CEILING - priority`, with `OSAL_PRI_CEILING = 16` — an
arbitrary internal constant chosen with headroom inside
`mtk3_bsp2`'s configured `CNF_MAX_TSKPRI` (32), not tied to the number of
priority levels this project actually uses. Only the relative ordering
matters (this project's `5 > 4 > 2 > 1` maps to µT-Kernel's `11 < 12 < 14 <
15` — still correctly highest-to-lowest), not the absolute numbers, so this
mapping has no dependency on how many distinct priorities a future session
adds.

---

## Object sizing for the queue → message-buffer mapping

`osal_queue_create()`/`send()`/`receive()` are backed by a µT-Kernel message
buffer (`tk_cre_mbf`/`tk_snd_mbf`/`tk_rcv_mbf`), the closest match to
FreeRTOS's fixed-item-size copy-semantics queue (µT-Kernel's mailbox
primitive passes pointers, not copies, so it wasn't the right fit).
Message buffers reserve a `sizeof(INT)` = 4-byte header per stored message
(`mtk3_bsp2/mtkernel/kernel/tkernel/messagebuf.h`'s `HEADERSZ`) — `bufsz` is
sized as `item_count * (round_up_to_4(item_size) + 4)` to actually hold
`item_count` messages, not `item_count - overhead`. The only queue this
project creates (`sd_logger.c`'s 16-slot, 128-byte log queue) sizes to 2112
bytes under this formula, close to but slightly larger than the "16 × 128 =
2 KB" comment already in `sd_logger.c` — that comment describes the payload
budget, not the actual kernel allocation, and remains accurate as a mental
model; only this file's actual `bufsz` math accounts for the header.

---

## Self-review (per this project's established grep-based checks)

- **Zero direct µT-Kernel API usage outside `ms_osal.c`:** grepped the whole
  `FSBL/Src`+`FSBL/Inc` tree for `tk_cre_tsk`, `tk_sta_tsk`, `tk_dly_tsk`,
  `tk_cre_mbf`, `tk_snd_mbf`, `tk_rcv_mbf`, `tk_cre_mtx`, `tk_loc_mtx`,
  `tk_unl_mtx`, `tk_slp_tsk`, `tk_cre_cyc`, and `#include <tk/tkernel.h>` —
  every hit is inside `ms_osal.c` or the vendored `mtk3_bsp2/` tree, none in
  application code. The OSAL boundary from Session 07 is intact.
- **Zero FreeRTOS symbols in the final binary:** grepped
  `MedSight_Session11_FSBL.map` for `freertos`/`xTaskCreate`/`vTaskDelay`/
  `xQueueSend`/`xSemaphoreTake`/`portTICK` — the only 16 hits are all the
  same already-explained empty `ll_aton_osal_freertos.o` object (zero-sized
  `.text`/`.data`/`.bss`), not a real symbol reference. `main.c` is
  byte-identical to Session 10's — confirmed with `diff`, not by
  inspection.
- **`objects.list` regenerated** using the technique documented in
  `ENGINEERING_LESSONS.md` (scan every `subdir.mk` under `sources.mk`'s
  `SUBDIRS` for `OBJS +=` entries) since deleting build artifacts before
  editing also deletes this linker response file — 321 objects (Session
  10's 96, minus 9 removed FreeRTOS objects, plus ~234 from the vendored
  `mtk3_bsp2` tree, which matches expectations).
- **No embeddings in UART:** this session touches no code path that ever
  handled face-embedding bytes; `ai_vision.c` is untouched.

## What was deliberately NOT done this session (scope discipline)

- **No changes to `main.c`, `state_machine.c`, `gui_draw.c`, `ai_vision.c`,
  `sd_logger.c`, `touch_driver.c`, `interactive_gui.c`,
  `registration_ui.c`, or `anime_ui.c`** — every one of them still only
  calls `ms_osal.h`'s unchanged public API.
- **No changes to `ffsystem.c`** — see above, it never depended on
  FreeRTOS in the first place.
- **No changes to the AI models or the `ll_aton_*` runtime files** — see
  above, `LL_ATON_OSAL_BARE_METAL` was already selected before this session.
- **No `.ioc` file changes** — this project doesn't use one (manual HAL
  only, per `ENGINEERING_LESSONS.md`'s Session 04/05 rule).
- **No physical motors/servos/IR hardware** — still fully software-only,
  per `MASTER_PROJECT_PLAN.md` §6; this session touches only the OS layer.
- **`Release` build config left unfixed.** `Release/sources.mk` and
  `Release/makefile` had their `Middlewares/FreeRTOS` entries removed for
  consistency, but were **not** given the new `mtk3_bsp2` subdirectory
  entries or the `ms_osal.c`/`AI` include-path/define fixes `Debug` got — a
  `Release` build would not currently compile. Every prior session's notes
  (08B/09/10) verify only `Debug` via the command-line toolchain, matching
  this project's actual workflow (`STM32CubeIDE`'s own Debug/Run, or the
  documented `arm-none-eabi-gcc`/`make` command against `.../Debug`), so
  this mirrors established practice rather than introducing a new gap —
  flagged explicitly rather than silently leaving `Release` inconsistent.

## Files changed

- `FSBL/Src/ms_osal.c` — full rewrite, µT-Kernel 3.0 backend (428 lines,
  see file header comment for the deferred-creation architecture).
- `FSBL/Inc/ms_osal.h` — doc-comment updates only, plus one new constant
  (`OSAL_TASK_NAME_MAX`); zero function-signature changes.
- `FSBL/mtk3_bsp2/` — new, ~230 files, vendored unmodified from
  `tron-forum/mtk3bsp2_samples`'s `Examples/prj_stm32n6_cam` (µT-Kernel 3.0
  BSP2 for this exact board).
- `Middlewares/Third_Party/FreeRTOS/` — deleted (22 MB).
- `FSBL/Inc/FreeRTOSConfig.h` — deleted.
- `STM32CubeIDE/FSBL/Debug/mtk3_bsp2/**/subdir.mk` — new, 112 files, copied
  from the reference project's own proven Debug build and re-pathed to this
  project's layout.
- `STM32CubeIDE/FSBL/Debug/sources.mk`, `Debug/makefile` — added the 112
  new `mtk3_bsp2` subdirectories; removed the `Middlewares/FreeRTOS` entry.
- `STM32CubeIDE/FSBL/Debug/Application/User/subdir.mk` — `ms_osal.c`'s
  compile rule only: added `-D_STM32CUBE_DISCOVERY_N657_` and 4 `mtk3_bsp2`
  include paths; removed the now-nonexistent FreeRTOS include paths from
  every rule in this file (harmless no-op for files other than
  `ms_osal.c`, since they never used those headers).
- Every other `subdir.mk` that referenced the two FreeRTOS include paths —
  stripped those two `-I` flags (unused, dangling references to a deleted
  directory).
- `STM32CubeIDE/FSBL/Debug/objects.list` — regenerated (321 objects).
- `STM32CubeIDE/FSBL/{Debug,Release}/makefile`, `.project`, `.launch` —
  build-artifact identity renamed `Session09`/`Session10` → `Session11`
  (two of these were stale carry-overs from Session 09, not something this
  session introduced — fixed while here).
- `STM32CubeIDE/FSBL/Release/sources.mk`, `Release/makefile` — FreeRTOS
  entries removed only (see "What was deliberately NOT done" above).

## Hardware verification — DONE (see Addendum 9 for the run that closed this)

Every item below was open until the Addendum 9 build was flashed. All are now
confirmed from real UART captures on the board.

- [x] **Board boots under µT-Kernel**, home screen renders, camera preview
      live. `knl_start_mtkernel()`'s VTOR relocation does not disturb the
      already-running DCMIPP/DMA2D interrupt handlers — as the design
      analysis predicted, though it took the Addendum 7 D-cache fix to make
      the relocated table actually readable by the CPU's vector fetch.
- [x] **Full dispense cycle end-to-end**, exercising the logger task's
      message-buffer queue, the UI task and the camera task as real
      µT-Kernel tasks: `STATE_INSTRUCT_DISPENSE` → `STATE_CAMERA_DISPENSE`
      → `Detector: Face detected!` → `emb_run done rc=0` →
      `Dispense: matched patient 'DOUSIK'.` → `STATE_DISPENSING` →
      `DISPENSE: DOUSIK 10 pills` → `STATE_CONFIRM_TAKEN` →
      `SD_Write_File(patients.dat): 1630 bytes written.` →
      `CONFIRMED: DOUSIK took pills` → `STATE_HOME`.
      **The Session 10 flow ran unchanged on the new kernel — the OSAL did
      exactly what Sessions 07 and 11 were designed to make it do.**
- [x] **Registration flow end-to-end** (not on the original list, but
      exercised in the same run): face capture → keyboard → pill count →
      confirm → `registration_ui: patient 'DOUSIK' saved to slot 0.` The
      register-then-dispense round trip from Session 09's Definition of Done
      also passes: the newly registered patient was rejected as an intruder
      *before* enrolment and correctly matched by name *after*.
- [x] **Touch responsiveness matches FreeRTOS.** Not true on the first
      working boot — see Addendum 9; it took the `CNF_TIMER_PERIOD` fix.
      Confirmed good by the user after that change.
- [x] **HAL peripheral timeouts behave correctly** (SD mount, camera init,
      NPU init all succeed). Note the original wording of this item was
      based on a wrong premise — see Addendum 9's correction of the "10 ms
      steps" claim.
- [x] **UART capture over a full boot + dispense cycle** shows all four task
      startup messages (`task_camera_isp:`, `task_ui:`, `task_logger:`,
      `task_heartbeat: started.`) and contains **no face-embedding bytes** —
      only names, confidence scores and event strings, per
      `COMPLIANCE_PRIVACY_POSTURE.md`.
- [ ] **Long-running stability** (several minutes of continuous operation,
      matching Session 07's verification bar) — the only item still open.
      Worth a deliberate soak before Session 13 packaging, watching for
      anything the deferred object-creation design in `ms_osal.c` could have
      gotten subtly wrong that a clean compile and a single pass cannot
      catch.

## Session 11 closeout

### Instrumentation strip — DONE

All temporary bring-up instrumentation is out. `sysinit.c`, `inittask.c`,
`timer.c`, `memory.c`, `task_manage.c`, `dispatch.S` and `cpu_cntl.c` are now
**byte-identical to vendored upstream**. `sys_start.c` and `interrupt.c` diff
against upstream by nothing but the real fixes:

- `sys_start.c`: the `MS_NEWLIB_HEAP_RESERVE` gap (Addendum 5a) and the
  `ms_osal_clean_dcache()` call after the VTOR relocation (Addendum 7).
- `interrupt.c`: the `ms_osal_clean_dcache()` call at the end of
  `knl_init_interrupt()` (Addendum 7) and `knl_default_handler()`'s
  `printf()` (Addendum 6).
- `exc_hdr.c`: unconditional fault printing (Addendum 5c). Kept permanently —
  it only ever prints when the CPU has already faulted fatally, and without
  it `USE_TMONITOR 0` leaves every fault silent.
- `ms_osal.c`: `usermain()`'s H–N trace removed; `ms_osal_clean_dcache()` and
  the derived `OSAL_HAL_TICK_PERIOD_MS` stay.

Verified against the `.elf`, not the build log (per `ENGINEERING_LESSONS.md`):
zero `[MS_DIAG] <checkpoint>:` strings remain, the ten fault-handler strings
are still present, both `ms_osal_clean_dcache` call sites are still linked,
and the `T_CCYC` initializer still reads `cyctim=0x00000001`.
Build clean: `text 734320, data 3992, bss 656824`.

### `Release` build config — deliberately NOT hand-fixed

`Release/` has no `mtk3_bsp2` subdirectories at all (0 `subdir.mk` files
against `Debug`'s 112). But hand-generating them would repeat the exact
mistake Addendum 1 documented, and would violate
`ENGINEERING_LESSONS.md` hard rule #2 ("never hand-edit generated Makefiles —
the fix is in project settings"). Two findings say the settings are already
right and the generated tree is the only thing missing:

- **`.cproject` is already correct for `Release`.** Both configurations carry
  the ten `mtk3_bsp2` include-path entries and both `_STM32CUBE_DISCOVERY_N657_`
  defines (C compiler + assembler). `Release` has zero FreeRTOS references;
  `Debug`'s single remaining "FreeRTOS" hit is inside an explanatory XML
  comment, not a setting.
- **`Release/` was already stale before Session 11 ever started.** Its
  `Application/User/subdir.mk` is missing `registration_ui.c` — a *Session 09*
  file — and lacks `-D_STM32CUBE_DISCOVERY_N657_`. So this is not an OSAL
  migration regression; the Release configuration has simply never been built
  since Session 08B.

**The correct fix is one `Project → Clean...` + build of the `Release`
configuration in the real STM32CubeIDE**, which regenerates the whole tree
from `.cproject`. That also closes the other open item below, since it is the
same IDE action. Patching the current stale tree by hand would have produced
a Release build that silently omitted `registration_ui.c`.

### Still open

- [ ] **STM32CubeIDE GUI clean build**, both configurations. Everything since
      Addendum 1 has been built and flashed from the command line. This is
      still the check Addendum 1 asked for, and it is also what fixes
      `Release` (above).
- [ ] **Long-running stability soak** (several minutes, matching Session 07's
      bar) — watching for anything the deferred object-creation design in
      `ms_osal.c` could have gotten subtly wrong that a single pass cannot
      catch.
- [ ] **Third-party software inventory for the contest submission.** TRON
      Programming Contest 2026 rule 1.3 requires, for every piece of existing
      software by others: name, rights holder, acquisition method, function,
      plus a rights-handling guarantee in the documentation. See
      `MASTER_PROJECT_PLAN.md` / Session 13's packaging scope — this is a real
      deliverable, not a formality, and this project uses a lot of it.

