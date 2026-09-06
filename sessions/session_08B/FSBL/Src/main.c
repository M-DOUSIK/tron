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
 * TASK DEFINITIONS — Session 07
 *
 * Priority scheme (deliberate — see session_07_notes.md):
 *   5 = task_camera_isp  — hardware timing, must never be starved
 *   4 = task_ui          — touch response target ≤ 20ms, above logging
 *   2 = task_logger      — SD writes tolerate latency, below UI
 *   1 = task_heartbeat   — lowest, purely a liveness LED blink
 *
 * Stack sizes are in 32-bit WORDS:
 *   1024 words = 4 KB  — adequate for FATFS local buffers (logger) and
 *                        the ISP middleware's background-processing locals.
 *   512 words  = 2 KB  — adequate for UI (no large local arrays).
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
    SD_Log_Event_Async("System Boot - Session 07 RTOS started");

    /* Session 08B: one-time NPU + face-gallery init. Runs here (not its own
     * task) because ai_vision_run_pipeline() is now called synchronously
     * from state_machine_update() during STATE_CAMERA_DISPENSE, replacing
     * Session 08A's free-running throwaway-model task. */
    ai_vision_init();

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
    for (;;) {
        BSP_LED_Toggle(LED_RED);
        osal_delay_ms(500u);
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

    /* Initialize all configured peripherals */
    MX_DCMIPP_Init();

    /* Initialize the IMX335 Sensor */
    IMX335_Probe(IMX335_R2592_1944, IMX335_RAW_RGGB10);

    /* USER CODE BEGIN 2 */
    LCD_Init(FRAME_WIDTH, FRAME_HEIGHT);

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

    if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0,
                                   BUFFER_ADDRESS, DCMIPP_MODE_CONTINUOUS) != HAL_OK) {
        Error_Handler();
    }

    /* Start the Image Signal Processing */
    if (ISP_Start(&hcamera_isp) != ISP_OK) {
        Error_Handler();
    }

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

    /* ── Create all RTOS tasks via OSAL ─────────────────────────────────── */
    /*
     * Stack sizes are in 32-bit WORDS.
     * Priorities:  5 = camera/ISP  4 = UI/touch  2 = logger  1 = heartbeat
     * See session_07_notes.md for the full rationale.
     */
    osal_task_create(task_camera_isp_fn, "cam_isp",   1024u, NULL, 5u);
    /* Session 08B: ai_vision no longer runs as its own free-running task —
     * ai_vision_init() runs once inside task_ui_fn, and
     * ai_vision_run_pipeline() is called synchronously from
     * state_machine_update() when the dispense flow needs a face check. */
    osal_task_create(task_ui_fn,         "ui",         2048u, NULL, 4u);
    osal_task_create(task_logger_fn,     "logger",     1024u, NULL, 2u);
    osal_task_create(task_heartbeat_fn,  "heartbeat",  256u,  NULL, 1u);

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
