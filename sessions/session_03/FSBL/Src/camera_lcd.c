#include "camera_lcd.h"
#include "isp_api.h"
#include "imx335_E27_isp_param_conf.h"
#include "stm32n6570_discovery_xspi.h"

extern DCMIPP_HandleTypeDef hdcmipp;
extern LTDC_HandleTypeDef hltdc;
extern ISP_HandleTypeDef  hcamera_isp;

static IMX335_Object_t   IMX335Obj;
static int32_t isp_gain;
static int32_t isp_exposure;

/* Calculate division factor for a given source and destination dimension (width or height) */
#define DIV_FACTOR(SRC, DST) (((uint32_t)((1024 * DST) / SRC)) > 1023 ? 1023 : ((uint32_t)((1024 * DST) / SRC)))

/* Calculate down scale ratio in unsigned 3.13 fixed-point format */
#define DOWNSCALE_RATIO(SRC, DST) (((uint32_t)(((float_t)(SRC) / (float_t)(DST)) * 8192) < 8192) ? 8192 : \
                                   ((((uint32_t)(((float_t)(SRC) / (float_t)(DST)) * 8192)) > 65535) ? 65535 : \
                                   ((uint32_t)(((float_t)(SRC) / (float_t)(DST)) * 8192))))

static void IMX335_Probe(uint32_t Resolution, uint32_t PixelFormat)
{
  IMX335_IO_t              IOCtx;
  uint32_t                 id;

  IOCtx.Address     = CAMERA_IMX335_ADDRESS;
  IOCtx.Init        = BSP_I2C1_Init;
  IOCtx.DeInit      = BSP_I2C1_DeInit;
  IOCtx.ReadReg     = BSP_I2C1_ReadReg16;
  IOCtx.WriteReg    = BSP_I2C1_WriteReg16;
  IOCtx.GetTick     = BSP_GetTick;

  if (IMX335_RegisterBusIO(&IMX335Obj, &IOCtx) != IMX335_OK) { Error_Handler(); }
  else if (IMX335_ReadID(&IMX335Obj, &id) != IMX335_OK) { Error_Handler(); }
  else
  {
    if (id != (uint32_t) IMX335_CHIP_ID) { Error_Handler(); }
    else
    {
      if (IMX335_Init(&IMX335Obj, Resolution, PixelFormat) != IMX335_OK) { Error_Handler(); }
      else if(IMX335_SetFrequency(&IMX335Obj, IMX335_INCK_24MHZ)!= IMX335_OK) { Error_Handler(); }
      else if (IMX335_MirrorFlipConfig(&IMX335Obj, IMX335_MIRROR) != IMX335_OK) { Error_Handler(); }
      else { return; }
    }
  }
}

static ISP_StatusTypeDef GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo)
{
  UNUSED(Instance);
  return (ISP_StatusTypeDef) IMX335_GetSensorInfo(&IMX335Obj, (IMX335_SensorInfo_t *) SensorInfo);
}

static ISP_StatusTypeDef SetSensorGainHelper(uint32_t Instance, int32_t Gain)
{
  UNUSED(Instance);
  isp_gain = Gain;
  return (ISP_StatusTypeDef) IMX335_SetGain(&IMX335Obj, Gain);
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
  return (ISP_StatusTypeDef) IMX335_SetExposure(&IMX335Obj, Exposure);
}

static ISP_StatusTypeDef GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure)
{
  UNUSED(Instance);
  *Exposure = isp_exposure;
  return ISP_OK;
}

static void MX_DCMIPP_Init(void)
{
  DCMIPP_PipeConfTypeDef pPipeConf = {0};
  DCMIPP_CSI_PIPE_ConfTypeDef pCSIPipeConf = {0};
  DCMIPP_CSI_ConfTypeDef csiconf = {0};
  DCMIPP_DownsizeTypeDef DonwsizeConf ={0};

  hdcmipp.Instance = DCMIPP;
  if (HAL_DCMIPP_Init(&hdcmipp) != HAL_OK) { Error_Handler(); }

  csiconf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csiconf.NumberOfLanes   = DCMIPP_CSI_TWO_DATA_LANES;
  csiconf.PHYBitrate      = DCMIPP_CSI_PHY_BT_1600;
  if(HAL_DCMIPP_CSI_SetConfig(&hdcmipp, &csiconf) != HAL_OK) { Error_Handler(); }
  if(HAL_DCMIPP_CSI_SetVCConfig(&hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10) != HAL_OK) { Error_Handler(); }

  pCSIPipeConf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  pCSIPipeConf.DataTypeIDA  = DCMIPP_DT_RAW10;
  pCSIPipeConf.DataTypeIDB  = DCMIPP_DT_RAW10; /* Don't Care */
  if (HAL_DCMIPP_CSI_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &pCSIPipeConf) != HAL_OK) { Error_Handler(); }

  pPipeConf.FrameRate  = DCMIPP_FRAME_RATE_ALL;
  pPipeConf.PixelPackerFormat = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;
  pPipeConf.PixelPipePitch  = 1600 ; /* Number of bytes */
  if (HAL_DCMIPP_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &pPipeConf) != HAL_OK) { Error_Handler(); }

  DonwsizeConf.HSize       = 800; /* Destination Image Width  */
  DonwsizeConf.VSize       = 480; /* Destination Image Height */
  DonwsizeConf.HRatio      = DOWNSCALE_RATIO(IMX335_WIDTH, DonwsizeConf.HSize);
  DonwsizeConf.VRatio      = DOWNSCALE_RATIO(IMX335_HEIGHT, DonwsizeConf.VSize);
  DonwsizeConf.HDivFactor  = DIV_FACTOR(IMX335_WIDTH, DonwsizeConf.HSize);
  DonwsizeConf.VDivFactor  = DIV_FACTOR(IMX335_HEIGHT, DonwsizeConf.VSize);

  if(HAL_DCMIPP_PIPE_SetDownsizeConfig(&hdcmipp, DCMIPP_PIPE1, &DonwsizeConf) != HAL_OK) { Error_Handler(); }
  if(HAL_DCMIPP_PIPE_EnableDownsize(&hdcmipp, DCMIPP_PIPE1) != HAL_OK) { Error_Handler(); }
}

static void LCD_Init(uint32_t Width, uint32_t Height)
{
  LTDC_LayerCfgTypeDef pLayerCfg ={0};

  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;

  hltdc.Init.HorizontalSync     = RK050HR18_HSYNC - 1;
  hltdc.Init.AccumulatedHBP     = RK050HR18_HSYNC + RK050HR18_HBP - 1;
  hltdc.Init.AccumulatedActiveW = RK050HR18_HSYNC + Width + RK050HR18_HBP -1;
  hltdc.Init.TotalWidth         = RK050HR18_HSYNC + Width + RK050HR18_HBP + RK050HR18_HFP - 1;
  hltdc.Init.VerticalSync       = RK050HR18_VSYNC - 1;
  hltdc.Init.AccumulatedVBP     = RK050HR18_VSYNC + RK050HR18_VBP - 1;
  hltdc.Init.AccumulatedActiveH = RK050HR18_VSYNC + Height + RK050HR18_VBP -1 ;
  hltdc.Init.TotalHeigh         = RK050HR18_VSYNC + Height + RK050HR18_VBP + RK050HR18_VFP - 1;

  hltdc.Init.Backcolor.Blue  = 0x0;
  hltdc.Init.Backcolor.Green = 0xFF;
  hltdc.Init.Backcolor.Red   = 0x0;

  if(HAL_LTDC_Init(&hltdc) != HAL_OK) { Error_Handler(); }

  pLayerCfg.WindowX0       = 0;
  pLayerCfg.WindowX1       = Width;
  pLayerCfg.WindowY0       = 0;
  pLayerCfg.WindowY1       = Height;
  pLayerCfg.PixelFormat    = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.FBStartAdress  = BUFFER_ADDRESS;
  pLayerCfg.Alpha = LTDC_LxCACR_CONSTA;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.ImageWidth = Width;
  pLayerCfg.ImageHeight = Height;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if(HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, LTDC_LAYER_1)) { Error_Handler(); }
}

void camera_lcd_init(void)
{
  /* Initialize External PSRAM */
  if (BSP_XSPI_RAM_Init(0) != BSP_ERROR_NONE) { Error_Handler(); }
  if (BSP_XSPI_RAM_EnableMemoryMappedMode(0) != BSP_ERROR_NONE) { Error_Handler(); }

  MX_DCMIPP_Init();
  IMX335_Probe(IMX335_R2592_1944, IMX335_RAW_RGGB10);
  LCD_Init(FRAME_WIDTH, FRAME_HEIGHT);
}

void camera_lcd_start(void)
{
  ISP_AppliHelpersTypeDef appliHelpers = {0};
  appliHelpers.GetSensorInfo = GetSensorInfoHelper;
  appliHelpers.SetSensorGain = SetSensorGainHelper;
  appliHelpers.GetSensorGain = GetSensorGainHelper;
  appliHelpers.SetSensorExposure = SetSensorExposureHelper;
  appliHelpers.GetSensorExposure = GetSensorExposureHelper;

  if(ISP_Init(&hcamera_isp, &hdcmipp, 0, &appliHelpers, ISP_IQParamCacheInit[0]) != ISP_OK)
  {
    Error_Handler();
  }

  if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0 , BUFFER_ADDRESS, DCMIPP_MODE_CONTINUOUS) != HAL_OK)
  {
    Error_Handler();
  }

  if (ISP_Start(&hcamera_isp) != ISP_OK)
  {
    Error_Handler();
  }
}
