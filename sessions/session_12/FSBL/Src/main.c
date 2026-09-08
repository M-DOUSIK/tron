/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body — Session 07: FreeRTOS via OSAL
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "isp_api.h"
#include "imx335_E27_isp_param_conf.h"
#include "ui/anime_ui.h"
#include "ui/state_machine.h"
#include "sd_logger.h"
#include "ai_vision.h"
#include "ms_osal.h"    /* Session 07: all RTOS access goes through OSAL */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* Session 12: the Session 11 boot-hang bring-up instrumentation — the solid
 * LED checkpoints and the six per-step GREEN blinks through camera/LCD
 * bring-up — is now compile-time gated and OFF by default. It cost ~3.7 s of
 * blinking on every single boot (six blinks of 620 ms plus a 1 s checkpoint-4
 * burst) for a hang that session_11_notes.md's Addenda 4-8 root-caused and
 * fixed. That file's closeout says all bring-up instrumentation was stripped;
 * it stripped the vendored kernel tree but missed main.c, so this closes it
 * out. Kept gated rather than deleted because it is genuinely the right first
 * tool if the board ever goes dark before UART comes up again — set this to 1
 * and it is all back, unchanged.
 * NOTE this does NOT touch Error_Handler()'s own blinking RED, which is a
 * permanent fault indicator, not instrumentation. */
#ifndef MS_BOOT_LED_CHECKPOINTS
#define MS_BOOT_LED_CHECKPOINTS   0
#endif

/* Session 12: how often task_heartbeat_fn prints the idle/power figure.
 * 10 s is frequent enough to watch a change take effect on the bench and
 * infrequent enough that it never competes with the flow's own logging. */
#define MS_IDLE_REPORT_MS   10000u

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* Calculate division factor for a given source and destination dimension */
#define DIV_FACTOR(SRC, DST) (((uint32_t)((1024 * DST) / SRC)) > 1023 ? 1023 : ((uint32_t)((1024 * DST) / SRC)))

/* Calculate down scale ratio in unsigned 3.13 fixed-point format */
#define DOWNSCALE_RATIO(SRC, DST) (((uint32_t)(((float_t)(SRC) / (float_t)(DST)) * 8192) < 8192) ? 8192 : \
                                   ((((uint32_t)(((float_t)(SRC) / (float_t)(DST)) * 8192)) > 65535) ? 65535 : \
                                   ((uint32_t)(((float_t)(SRC) / (float_t)(DST)) * 8192))))
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DCMIPP_HandleTypeDef hdcmipp;
LTDC_HandleTypeDef   hltdc;
ISP_HandleTypeDef    hcamera_isp;
SD_HandleTypeDef     hsd2;
DMA2D_HandleTypeDef  hdma2d;

/* USER CODE BEGIN PV */
static __IO uint32_t NbMainFrames = 0;
static IMX335_Object_t   IMX335Obj;
static int32_t isp_gain;
static int32_t isp_exposure;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_DCMIPP_Init(void);
static void LCD_Init(uint32_t Width, uint32_t Height);
static void MX_SDMMC2_SD_Init(void);
/* USER CODE BEGIN PFP */
static void IMX335_Probe(uint32_t Resolution, uint32_t PixelFormat);
static ISP_StatusTypeDef GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo);
static ISP_StatusTypeDef SetSensorGainHelper(uint32_t Instance, int32_t Gain);
static ISP_StatusTypeDef GetSensorGainHelper(uint32_t Instance, int32_t *Gain);
static ISP_StatusTypeDef SetSensorExposureHelper(uint32_t Instance, int32_t Exposure);
static ISP_StatusTypeDef GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* ════════════════════════════════════════════════════════════════════════════
 * TASK DEFINITIONS — Session 07, priorities re-derived in Session 12
 *
 * ── The priority scheme, and where the numbers come from ──────────────────
 *
 * Sessions 07-11 used 5/4/2/1 with the ordering argued informally ("camera
 * must never be starved", "touch should feel immediate"). The ordering was
 * right; the numbers had never been justified against each task's actual
 * period and deadline. Session 12 derived them properly, rate-monotonic
 * style — shortest period gets the highest priority — and the result agrees
 * with what was already there, which is why nothing was renumbered. The one
 * change is that the previously-vacant level 3 is now occupied by the new
 * NPU task, deliberately placed BELOW the UI.
 *
 *  pri | task            | period      | deadline / why it sits there
 *  ----+-----------------+-------------+--------------------------------------
 *   5  | task_camera_isp | 1 ms        | Shortest period in the system and the
 *      |                 |             | only one tied to external hardware
 *      |                 |             | timing: ISP_BackgroundProcess() must
 *      |                 |             | consume each frame's statistics
 *      |                 |             | before the next VSYNC (~33 ms at
 *      |                 |             | 30 fps). Missing it degrades AE/AWB
 *      |                 |             | convergence, which is visible.
 *   4  | task_ui         | 10 ms       | Touch-to-response budget. Humans
 *      |                 |             | notice input lag past ~100 ms; a
 *      |                 |             | 10 ms poll plus the 4 FPS mascot
 *      |                 |             | frame gate keeps the whole UI an
 *      |                 |             | order of magnitude inside that.
 *      |                 |             | Session 11's Addendum 9 is the
 *      |                 |             | evidence for why this matters: when
 *      |                 |             | the kernel tick made these deadlines
 *      |                 |             | stretch 10x, the device immediately
 *      |                 |             | "felt slower than FreeRTOS".
 *   3  | task_ai         | on demand   | NO deadline. Hundreds of ms of solid
 *      |    (Session 12) | (~0.5-4 s   | NPU/CPU work per request, a few
 *      |                 |  per burst) | times per session, in response to a
 *      |                 |             | button press the user already knows
 *      |                 |             | takes a moment. Placed below the UI
 *      |                 |             | on purpose: this is precisely the
 *      |                 |             | work that must be preemptible, so
 *      |                 |             | touch and the physical USER1 button
 *      |                 |             | keep responding while it runs. Above
 *      |                 |             | the logger because the user is
 *      |                 |             | actively waiting on its result and
 *      |                 |             | nobody is waiting on a log line.
 *   2  | task_logger     | event-driven| Tolerates seconds of latency by
 *      |                 |             | construction — the async queue
 *      |                 |             | exists so callers never wait on an
 *      |                 |             | SD write (10-50 ms each, occasionally
 *      |                 |             | much worse on a slow card).
 *   1  | task_heartbeat  | 500 ms      | No deadline at all; it is a liveness
 *      |                 |             | indicator. Deliberately lowest so
 *      |                 |             | that "the LED stopped blinking" means
 *      |                 |             | "something above me is starving the
 *      |                 |             | system", which is exactly the signal
 *      |                 |             | you want it to carry.
 *
 * ms_osal.h's convention is 1 = lowest; ms_osal.c inverts it onto
 * µT-Kernel's opposite scale (1 = highest) as OSAL_PRI_CEILING - priority,
 * so 5/4/3/2/1 become itskpri 11/12/13/14/15. Only the ordering is load
 * bearing; the absolute numbers leave headroom on both sides inside
 * mtk3_bsp2's CNF_MAX_TSKPRI of 32.
 *
 * Stack sizes are in 32-bit WORDS:
 *   2048 words = 8 KB  — UI (deep call chain through gui_draw/registration_ui)
 *                        and AI (the ST Edge AI runtime's own call depth; this
 *                        is the stack the pipeline already ran on when it was
 *                        part of the UI task, carried over unchanged rather
 *                        than re-tuned blind).
 *   1024 words = 4 KB  — adequate for FATFS local buffers (logger) and
 *                        the ISP middleware's background-processing locals.
 *   256 words  = 1 KB  — minimal, heartbeat has no deep callstack.
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── Task: Camera / ISP background processing ───────────────────────────── */
/* Priority 5 — highest. ISP_BackgroundProcess() must run every frame period.
 * Running it in a dedicated task prevents UI/logging jitter from delaying it. */
static void task_camera_isp_fn(void *arg)
{
    (void)arg;
    printf("task_camera_isp: started.\r\n");

    for (;;) {
        BSP_LED_Toggle(LED_GREEN);
        if (ISP_BackgroundProcess(&hcamera_isp) != ISP_OK) {
            BSP_LED_Toggle(LED_RED);
        }
        /* Yield for 1 tick (1ms) — ISP needs to run once per frame (~30fps),
         * so 1ms sleep is faster than needed but ensures other tasks get CPU.
         * The ISP library is self-throttling internally. */
        osal_delay_ms(1u);
    }
}

/* ── Task: UI — touch polling, state machine, mascot animation ─────────── */
/* Priority 4 — second-highest. Touch events polled every 10ms = 100Hz.
 * This gives ≤10ms touch-to-response latency, well within "immediate" feel.
 * The mascot animation self-throttles to 4 FPS inside anime_ui_update().   */
static void task_ui_fn(void *arg)
{
    (void)arg;
    printf("task_ui: started.\r\n");

    /* state_machine_init() configures touch IC, draws home screen, etc.
     * Called here (inside the task) because some BSP functions need the
     * RTOS scheduler already running to use HAL I2C blocking functions
     * correctly. */
    state_machine_init();
    SD_Log_Event_Async("System Boot - MedSight on uT-Kernel 3.0");

    /* Session 12: ai_vision_init() no longer runs here. Session 09B put the
     * one-time NPU + gallery bring-up on this task because inference itself
     * was a synchronous call from state_machine_update(); it now lives in
     * task_ai_fn (ai_vision.c), which is the task that owns the NPU. */

    for (;;) {
        state_machine_update();
        /* 10ms poll period: responsive touch without burning CPU.
         * anime_ui_update() inside state_machine_update() uses HAL_GetTick()
         * for its own 250ms frame gate — unaffected by this sleep. */
        osal_delay_ms(10u);
    }
}

/* ── Task: Logger — dequeue and write to SD ─────────────────────────────── */
/* Priority 2 — low. SD writes can take 10–50ms; other tasks preempt freely.
 * task_logger_fn is defined in sd_logger.c (the log queue is private there). */

/* ── Task: Heartbeat — liveness LED blink ───────────────────────────────── */
/* Priority 1 — lowest. If this stops blinking, the scheduler has stalled.
 * Blink period: 500ms on, 500ms off = 1Hz. */
static void task_heartbeat_fn(void *arg)
{
    (void)arg;
    printf("task_heartbeat: started.\r\n");

    /* Session 12: this task also carries the power-saving measurement, since
     * it is the one task in the system with a fixed, known period and no
     * deadline of its own. Every MS_IDLE_REPORT_MS it prints what proportion
     * of wall-clock time the Cortex-M55 spent inside WFI in µT-Kernel's idle
     * path — a real number for TRON rule 1.4's "power saving" criterion,
     * rather than the claim "we call WFI".
     *
     * Deliberately printed from TASK context on a 10 s period, never from
     * the idle hook itself: session_11_notes.md Addendum 8 is the record of
     * what putting a printf on a per-tick path costs on this hardware. */
    uint32_t last_report_tick   = HAL_GetTick();
    uint64_t last_idle_cycles   = 0u;
    uint32_t last_idle_entries  = 0u;

    for (;;) {
        BSP_LED_Toggle(LED_RED);
        osal_delay_ms(500u);

        uint32_t now = HAL_GetTick();
        if ((now - last_report_tick) >= MS_IDLE_REPORT_MS) {
            uint64_t idle_cycles = 0u;
            uint32_t idle_entries = 0u;
            bool     cyccnt_ok = ms_osal_idle_stats(&idle_cycles, &idle_entries);

            uint32_t elapsed_ms = now - last_report_tick;
            uint64_t d_cycles   = idle_cycles - last_idle_cycles;
            uint32_t d_entries  = idle_entries - last_idle_entries;

            if (cyccnt_ok && elapsed_ms > 0u) {
                /* Busy cycles available in the window = CPU clock * seconds.
                 * Integer maths only — this toolchain's nano.specs printf has
                 * no %f, and a per-mille figure is precise enough to quote. */
                uint64_t window_cycles =
                    ((uint64_t)SystemCoreClock / 1000u) * (uint64_t)elapsed_ms;
                uint32_t permille = (window_cycles > 0u)
                    ? (uint32_t)((d_cycles * 1000u) / window_cycles) : 0u;
                if (permille > 1000u) permille = 1000u;
                printf("power: idle %u.%u%% of last %ums (%u WFI entries)\r\n",
                       (unsigned)(permille / 10u), (unsigned)(permille % 10u),
                       (unsigned)elapsed_ms, (unsigned)d_entries);
            } else {
                printf("power: %u WFI entries in last %ums "
                       "(cycle counter unavailable)\r\n",
                       (unsigned)d_entries, (unsigned)elapsed_ms);
            }

            last_report_tick  = now;
            last_idle_cycles  = idle_cycles;
            last_idle_entries = idle_entries;
        }
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int (never returns — ends with osal_scheduler_start())
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    ISP_AppliHelpersTypeDef appliHelpers = {0};
    /* USER CODE END 1 */

    /* Enable the CPU Cache */
    SCB_EnableICache();
    SCB_EnableDCache();

    /* MCU Configuration */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Configure the peripherals common clocks */
    PeriphCommonClock_Config();

    /* USER CODE BEGIN SysInit */
    BSP_LED_Init(LED_GREEN);
    BSP_LED_Init(LED_RED);

    /* Session 11 bring-up checkpoint 1 — see MS_BOOT_LED_CHECKPOINTS above.
     * Cache enable, HAL_Init(), SystemClock_Config(),
     * PeriphCommonClock_Config() and LED GPIO init all completed.
     * Solid GREEN = passed checkpoint 1. */
#if MS_BOOT_LED_CHECKPOINTS
    BSP_LED_On(LED_GREEN);
#endif

    /* UART log */
#if USE_COM_LOG
    COM_InitTypeDef COM_Init;
    COM_Init.BaudRate   = 115200;
    COM_Init.WordLength = COM_WORDLENGTH_8B;
    COM_Init.StopBits   = COM_STOPBITS_1;
    COM_Init.Parity     = COM_PARITY_NONE;
    COM_Init.HwFlowCtl  = COM_HWCONTROL_NONE;
    BSP_COM_Init(COM1, &COM_Init);
    if (BSP_COM_SelectLogPort(COM1) != BSP_ERROR_NONE) {
        Error_Handler();
    }
#endif
    /* USER CODE END SysInit */

    /* Checkpoint 2: UART/COM init (BSP_COM_Init, BSP_COM_SelectLogPort)
     * completed without Error_Handler firing. Solid GREEN+RED. */
#if MS_BOOT_LED_CHECKPOINTS
    BSP_LED_On(LED_RED);
#endif

    /* Per-step blink through the six camera/LCD bring-up calls below. RED
     * stays solid throughout (it only starts BLINKING if Error_Handler()
     * fires); count the GREEN blinks to see exactly how far boot gets. */
#if MS_BOOT_LED_CHECKPOINTS
#define _MS_BLINK() do { BSP_LED_On(LED_GREEN); HAL_Delay(120); \
                          BSP_LED_Off(LED_GREEN); HAL_Delay(500); } while (0)
#else
#define _MS_BLINK() do { } while (0)
#endif

    /* Initialize all configured peripherals */
    MX_DCMIPP_Init();
    _MS_BLINK();   /* blink #1: MX_DCMIPP_Init() returned */

    /* Initialize the IMX335 Sensor */
    IMX335_Probe(IMX335_R2592_1944, IMX335_RAW_RGGB10);
    _MS_BLINK();   /* blink #2: IMX335_Probe() returned */

    /* USER CODE BEGIN 2 */
    LCD_Init(FRAME_WIDTH, FRAME_HEIGHT);
    _MS_BLINK();   /* blink #3: LCD_Init() returned */

    /* Fill init struct with Camera driver helpers */
    appliHelpers.GetSensorInfo     = GetSensorInfoHelper;
    appliHelpers.SetSensorGain     = SetSensorGainHelper;
    appliHelpers.GetSensorGain     = GetSensorGainHelper;
    appliHelpers.SetSensorExposure = SetSensorExposureHelper;
    appliHelpers.GetSensorExposure = GetSensorExposureHelper;

    /* Initialize the Image Signal Processing middleware */
    if (ISP_Init(&hcamera_isp, &hdcmipp, 0, &appliHelpers, ISP_IQParamCacheInit[0]) != ISP_OK) {
        Error_Handler();
    }
    _MS_BLINK();   /* blink #4: ISP_Init() returned ISP_OK */

    if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0,
                                   BUFFER_ADDRESS, DCMIPP_MODE_CONTINUOUS) != HAL_OK) {
        Error_Handler();
    }
    _MS_BLINK();   /* blink #5: HAL_DCMIPP_CSI_PIPE_Start() returned HAL_OK */

    /* Start the Image Signal Processing */
    if (ISP_Start(&hcamera_isp) != ISP_OK) {
        Error_Handler();
    }
    _MS_BLINK();   /* blink #6: ISP_Start() returned ISP_OK — camera pipeline is fully up */

    /* Session 04: initialise DMA2D for mascot alpha-blending */
    hdma2d.Instance          = DMA2D;
    hdma2d.Init.Mode         = DMA2D_M2M_BLEND;
    hdma2d.Init.ColorMode    = DMA2D_OUTPUT_RGB565;
    hdma2d.Init.OutputOffset = 0;
    if (HAL_DMA2D_Init(&hdma2d) != HAL_OK) {
        Error_Handler();
    }

    /* Register the DMA2D completion callback for anime_ui */
    extern void HAL_DMA2D_XferCpltCallback(DMA2D_HandleTypeDef *hdma2d);
    hdma2d.XferCpltCallback = HAL_DMA2D_XferCpltCallback;
    /* DMA2D priority must be NUMERICALLY >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5)
     * to sit below FreeRTOS's BASEPRI mask. Priority 6 = safe, never calls FreeRTOS API. */
    HAL_NVIC_SetPriority(DMA2D_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA2D_IRQn);

    /* Initialise the Lumio mascot animation layer */
    anime_ui_init(&hdma2d, BUFFER_ADDRESS, FRAME_WIDTH);

    /* Session 06: SDMMC hardware init (mounting moved to task_logger_fn) */
    MX_SDMMC2_SD_Init();

    /* Session 07: initialise the async log queue (must be before task creation) */
    SD_Logger_Queue_Init();

    /* Session 12: create the AI service's event flag. Deliberately here, in
     * main(), alongside every other osal_*_create() call — ms_osal.c's
     * "create after the kernel is already running" branch exists but has
     * never been exercised on hardware, and this session is not the place to
     * become the first thing that depends on it. */
    ai_vision_service_init();

    /* ── Create all RTOS tasks via OSAL ─────────────────────────────────── */
    /*
     * Stack sizes are in 32-bit WORDS.
     * Priorities:  5 = camera/ISP  4 = UI/touch  3 = AI/NPU  2 = logger
     *              1 = heartbeat
     * See the task-definition block above for the derivation.
     */
    osal_task_create(task_camera_isp_fn, "cam_isp",   1024u, NULL, 5u);
    osal_task_create(task_ui_fn,         "ui",         2048u, NULL, 4u);
    /* Session 12: the NPU pipeline is a task again — but unlike Session 08A's
     * free-running 1 Hz demo task, this one is idle until the UI asks it for
     * a capture over an event flag. See ai_vision.h. */
    osal_task_create(task_ai_fn,         "ai",         2048u, NULL, 3u);
    osal_task_create(task_logger_fn,     "logger",     1024u, NULL, 2u);
    osal_task_create(task_heartbeat_fn,  "heartbeat",  256u,  NULL, 1u);

    /* Checkpoint 4: every osal_task_create() call and
     * SD_Logger_Queue_Init()/MX_SDMMC2_SD_Init() returned — everything
     * main() does is finished, the very next line hands off to
     * osal_scheduler_start() -> knl_start_mtkernel(). Fast alternating
     * GREEN/RED blink for ~1 s. If you see this blink but nothing after, the
     * hang is inside knl_start_mtkernel()/usermain() itself. If you never
     * see this blink, the hang is somewhere in the camera/DMA2D/SD bring-up
     * above (i.e. unrelated to the kernel). */
#if MS_BOOT_LED_CHECKPOINTS
    for (int _diag_i = 0; _diag_i < 10; _diag_i++) {
        BSP_LED_Toggle(LED_GREEN);
        BSP_LED_Toggle(LED_RED);
        HAL_Delay(100);
    }
#endif

    /* ── Hand control to the RTOS scheduler — does not return ───────────── */
    osal_scheduler_start();

    /* USER CODE END 2 */

    /* Execution never reaches here. osal_scheduler_start() triggers Error_Handler
     * via configASSERT if heap is too small to start the scheduler. */
    while (1) {}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    if (HAL_PWREx_ConfigSupply(PWR_EXTERNAL_SOURCE_SUPPLY) != HAL_OK) {
        Error_Handler();
    }

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
    if ((RCC_ClkInitStruct.CPUCLKSource == RCC_CPUCLKSOURCE_IC1) ||
        (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_IC2_IC6_IC11))
    {
        RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK);
        RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK) {
            Error_Handler();
        }
    }

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
    RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL1.PLLM = 4;
    RCC_OscInitStruct.PLL1.PLLN = 75;
    RCC_OscInitStruct.PLL1.PLLFractional = 0;
    RCC_OscInitStruct.PLL1.PLLP1 = 1;
    RCC_OscInitStruct.PLL1.PLLP2 = 1;
    RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_HCLK
                                | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1
                                | RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_PCLK5
                                | RCC_CLOCKTYPE_PCLK4;
    RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
    RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;
    RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
    RCC_ClkInitStruct.IC1Selection.ClockDivider = 2;
    RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
    RCC_ClkInitStruct.IC2Selection.ClockDivider = 3;
    RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
    RCC_ClkInitStruct.IC6Selection.ClockDivider = 3;
    RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
    RCC_ClkInitStruct.IC11Selection.ClockDivider = 3;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CKPER;
    PeriphClkInitStruct.CkperClockSelection = RCC_CLKPCLKSOURCE_HSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief DCMIPP Initialization Function
  */
static void MX_DCMIPP_Init(void)
{
    DCMIPP_PipeConfTypeDef pPipeConf = {0};
    DCMIPP_CSI_PIPE_ConfTypeDef pCSIPipeConf = {0};
    DCMIPP_CSI_ConfTypeDef csiconf = {0};
    DCMIPP_DownsizeTypeDef DonwsizeConf = {0};

    hdcmipp.Instance = DCMIPP;
    if (HAL_DCMIPP_Init(&hdcmipp) != HAL_OK) {
        Error_Handler();
    }

    csiconf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
    csiconf.NumberOfLanes   = DCMIPP_CSI_TWO_DATA_LANES;
    csiconf.PHYBitrate      = DCMIPP_CSI_PHY_BT_1600;
    if (HAL_DCMIPP_CSI_SetConfig(&hdcmipp, &csiconf) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_DCMIPP_CSI_SetVCConfig(&hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10) != HAL_OK) {
        Error_Handler();
    }

    pCSIPipeConf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
    pCSIPipeConf.DataTypeIDA  = DCMIPP_DT_RAW10;
    pCSIPipeConf.DataTypeIDB  = DCMIPP_DT_RAW10;
    if (HAL_DCMIPP_CSI_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &pCSIPipeConf) != HAL_OK) {
        Error_Handler();
    }

    pPipeConf.FrameRate  = DCMIPP_FRAME_RATE_ALL;
    pPipeConf.PixelPackerFormat = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;
    pPipeConf.PixelPipePitch  = 1600;
    if (HAL_DCMIPP_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &pPipeConf) != HAL_OK) {
        Error_Handler();
    }

    DonwsizeConf.HSize      = 800;
    DonwsizeConf.VSize      = 480;
    DonwsizeConf.HRatio     = DOWNSCALE_RATIO(IMX335_WIDTH, DonwsizeConf.HSize);
    DonwsizeConf.VRatio     = DOWNSCALE_RATIO(IMX335_HEIGHT, DonwsizeConf.VSize);
    DonwsizeConf.HDivFactor = DIV_FACTOR(IMX335_WIDTH, DonwsizeConf.HSize);
    DonwsizeConf.VDivFactor = DIV_FACTOR(IMX335_HEIGHT, DonwsizeConf.VSize);
    if (HAL_DCMIPP_PIPE_SetDownsizeConfig(&hdcmipp, DCMIPP_PIPE1, &DonwsizeConf) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_DCMIPP_PIPE_EnableDownsize(&hdcmipp, DCMIPP_PIPE1) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief SDMMC2 SD Initialization
  */
static void MX_SDMMC2_SD_Init(void)
{
    hsd2.Instance = SDMMC2;
    hsd2.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    hsd2.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    hsd2.Init.BusWide = SDMMC_BUS_WIDE_4B;
    hsd2.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd2.Init.ClockDiv = 2;
}

/**
  * @brief LCD Init
  */
static void LCD_Init(uint32_t Width, uint32_t Height)
{
    LTDC_LayerCfgTypeDef pLayerCfg = {0};

    hltdc.Instance = LTDC;
    hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    hltdc.Init.HorizontalSync     = RK050HR18_HSYNC - 1;
    hltdc.Init.AccumulatedHBP     = RK050HR18_HSYNC + RK050HR18_HBP - 1;
    hltdc.Init.AccumulatedActiveW = RK050HR18_HSYNC + Width + RK050HR18_HBP - 1;
    hltdc.Init.TotalWidth         = RK050HR18_HSYNC + Width + RK050HR18_HBP + RK050HR18_HFP - 1;
    hltdc.Init.VerticalSync       = RK050HR18_VSYNC - 1;
    hltdc.Init.AccumulatedVBP     = RK050HR18_VSYNC + RK050HR18_VBP - 1;
    hltdc.Init.AccumulatedActiveH = RK050HR18_VSYNC + Height + RK050HR18_VBP - 1;
    hltdc.Init.TotalHeigh         = RK050HR18_VSYNC + Height + RK050HR18_VBP + RK050HR18_VFP - 1;
    hltdc.Init.Backcolor.Blue  = 0x0;
    hltdc.Init.Backcolor.Green = 0xFF;
    hltdc.Init.Backcolor.Red   = 0x0;
    if (HAL_LTDC_Init(&hltdc) != HAL_OK) {
        Error_Handler();
    }

    pLayerCfg.WindowX0       = 0;
    pLayerCfg.WindowX1       = Width;
    pLayerCfg.WindowY0       = 0;
    pLayerCfg.WindowY1       = Height;
    pLayerCfg.PixelFormat    = LTDC_PIXEL_FORMAT_RGB565;
    pLayerCfg.FBStartAdress  = BUFFER_ADDRESS;
    pLayerCfg.Alpha          = LTDC_LxCACR_CONSTA;
    pLayerCfg.Alpha0         = 0;
    pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    pLayerCfg.ImageWidth     = Width;
    pLayerCfg.ImageHeight    = Height;
    pLayerCfg.Backcolor.Blue  = 0;
    pLayerCfg.Backcolor.Green = 0;
    pLayerCfg.Backcolor.Red   = 0;
    if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, LTDC_LAYER_1)) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

static void IMX335_Probe(uint32_t Resolution, uint32_t PixelFormat)
{
    IMX335_IO_t IOCtx;
    uint32_t    id;

    IOCtx.Address  = CAMERA_IMX335_ADDRESS;
    IOCtx.Init     = BSP_I2C1_Init;
    IOCtx.DeInit   = BSP_I2C1_DeInit;
    IOCtx.ReadReg  = BSP_I2C1_ReadReg16;
    IOCtx.WriteReg = BSP_I2C1_WriteReg16;
    IOCtx.GetTick  = BSP_GetTick;

    if (IMX335_RegisterBusIO(&IMX335Obj, &IOCtx) != IMX335_OK) {
        Error_Handler();
    } else if (IMX335_ReadID(&IMX335Obj, &id) != IMX335_OK) {
        Error_Handler();
    } else {
        if (id != (uint32_t)IMX335_CHIP_ID) {
            Error_Handler();
        } else {
            if (IMX335_Init(&IMX335Obj, Resolution, PixelFormat) != IMX335_OK) {
                Error_Handler();
            } else if (IMX335_SetFrequency(&IMX335Obj, IMX335_INCK_24MHZ) != IMX335_OK) {
                Error_Handler();
            } else if (IMX335_MirrorFlipConfig(&IMX335Obj, IMX335_MIRROR) != IMX335_OK) {
                Error_Handler();
            } else {
                return;
            }
        }
    }
}

static ISP_StatusTypeDef GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo)
{
    UNUSED(Instance);
    return (ISP_StatusTypeDef)IMX335_GetSensorInfo(&IMX335Obj, (IMX335_SensorInfo_t *)SensorInfo);
}

static ISP_StatusTypeDef SetSensorGainHelper(uint32_t Instance, int32_t Gain)
{
    UNUSED(Instance);
    isp_gain = Gain;
    return (ISP_StatusTypeDef)IMX335_SetGain(&IMX335Obj, Gain);
}

static ISP_StatusTypeDef GetSensorGainHelper(uint32_t Instance, int32_t *Gain)
{
    UNUSED(Instance);
    *Gain = isp_gain;
    return ISP_OK;
}

static ISP_StatusTypeDef SetSensorExposureHelper(uint32_t Instance, int32_t Exposure)
{
    UNUSED(Instance);
    isp_exposure = Exposure;
    return (ISP_StatusTypeDef)IMX335_SetExposure(&IMX335Obj, Exposure);
}

static ISP_StatusTypeDef GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure)
{
    UNUSED(Instance);
    *Exposure = isp_exposure;
    return ISP_OK;
}

void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
    NbMainFrames++;
}

void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
    UNUSED(hdcmipp);
    switch (Pipe) {
        case DCMIPP_PIPE0:
            ISP_IncDumpFrameId(&hcamera_isp);
            break;
        case DCMIPP_PIPE1:
            ISP_IncMainFrameId(&hcamera_isp);
            ISP_GatherStatistics(&hcamera_isp);
            break;
        case DCMIPP_PIPE2:
            ISP_IncAncillaryFrameId(&hcamera_isp);
            break;
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    while (1) {
        HAL_Delay(250);
        BSP_LED_Toggle(LED_RED);
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    while (1) {}
}
#endif /* USE_FULL_ASSERT */
