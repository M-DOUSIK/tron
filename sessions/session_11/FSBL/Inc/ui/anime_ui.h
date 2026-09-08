/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ui/anime_ui.h
  * @brief   Lumio mascot animation layer — public API.
  *
  *          Session 04: Implements MASCOT_IDLE state with DMA2D alpha-blending
  *          over the live camera framebuffer. All other states are stubbed and
  *          will be filled in during Sessions 05–09.
  *
  *          IP-compliance: Lumio is an original character. No existing franchise,
  *          trademark, or copyrighted IP is referenced anywhere in this module.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef UI_ANIME_UI_H
#define UI_ANIME_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32n6xx_hal.h"
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Mascot state enumeration
 * Defined here once; used by interactive_gui (Session 05), state_machine
 * (Session 11), and this renderer. The renderer never decides *why* a state
 * changes — it only decides *how* to render it.
 * --------------------------------------------------------------------------- */
typedef enum
{
    MASCOT_IDLE    = 0,  /**< Gentle idle breathing loop — implemented Session 04 */
    MASCOT_ACTIVE  = 1,  /**< "In progress" animation   — stubbed, TODO Session 05 */
    MASCOT_SUCCESS = 2,  /**< Happy / green accent        — stubbed, TODO Session 07 */
    MASCOT_ERROR   = 3,  /**< Concerned / amber-red accent — stubbed, TODO Session 07 */
} mascot_state_t;

/* ---------------------------------------------------------------------------
 * Sprite geometry — Lumio is placed at the bottom-centre of the 800x480 panel.
 * --------------------------------------------------------------------------- */
#define LUMIO_SPRITE_W    32u          /**< Sprite width  in pixels (native)     */
#define LUMIO_SPRITE_H    48u          /**< Sprite height in pixels (native)     */
#define LUMIO_SCALE        4u          /**< Pixel-art scale factor (4x)          */
#define LUMIO_DRAW_W     (LUMIO_SPRITE_W * LUMIO_SCALE)   /* = 128 */
#define LUMIO_DRAW_H     (LUMIO_SPRITE_H * LUMIO_SCALE)   /* = 192 */
#define LUMIO_IDLE_FRAMES  8u          /**< Number of frames in the idle loop    */
#define LUMIO_FPS          4u          /**< Target animation frame rate (4 FPS)  */
#define LUMIO_FRAME_MS   (1000u / LUMIO_FPS)  /**< ms per frame = 250 ms        */

/** Right side of screen, beside the two buttons */
#define LUMIO_X_POS      590u   /* pixels from left edge */

/** Vertically in the button zone */
#define LUMIO_Y_BASE      60u   /* pixels from top */

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

/**
  * @brief  Initialise the animation layer.
  * @param  hdma2d_handle  Pointer to the DMA2D handle initialised by main.c.
  *                        The handle must already have been configured for
  *                        Memory-to-Memory-with-blending mode before this call.
  * @param  fb_addr        Base address of the RGB565 LCD framebuffer (e.g. 0x34200000).
  * @param  fb_width       Framebuffer stride in pixels (e.g. 800).
  */
void anime_ui_init(DMA2D_HandleTypeDef *hdma2d_handle,
                   uint32_t             fb_addr,
                   uint32_t             fb_width);

/**
  * @brief  Advance the animation and trigger a DMA2D blend if a new frame is due.
  *         Call this from the main loop on every iteration; it self-throttles to
  *         LUMIO_FPS using HAL_GetTick(). Non-blocking — returns immediately after
  *         kicking off the DMA2D transfer.
  * @param  tick_ms  Current timestamp in milliseconds, e.g. HAL_GetTick().
  */
void anime_ui_update(uint32_t tick_ms);

/**
  * @brief  Switch the mascot to a new state.
  *         Safe to call from any context; the renderer picks up the new state
  *         on the next anime_ui_update() call.
  * @param  state  Target mascot state.
  */
void anime_ui_set_state(mascot_state_t state);

/**
  * @brief  DMA2D transfer-complete callback hook.
  *         Call this from HAL_DMA2D_XferCpltCallback() in stm32n6xx_it.c or
  *         main.c so the animation layer knows the GPU is free for the next frame.
  */
void anime_ui_dma2d_cplt_cb(void);

/**
  * @brief  Override the destination framebuffer address at runtime.
  */
void anime_ui_set_dest_buffer(uint32_t dest_buffer);

/**
  * @brief  Set the background colour used to fill transparent sprite pixels.
  *         Call this whenever the screen background changes so the mascot
  *         bounding box is always redrawn atomically (no clear+draw flicker).
  *         Default: 0xADD6 (light sky-blue).
  */
void anime_ui_set_bg_color(uint16_t color);

/* ---------------------------------------------------------------------------
 * Stubbed state renderers — called internally by anime_ui_update().
 * Declared here so they are visible for unit-test hooks if needed.
 * Full implementations arrive in Sessions 05 and 07.
 * --------------------------------------------------------------------------- */
void anime_ui_state_active(void);   /**< TODO Session 05 */
void anime_ui_state_success(void);  /**< TODO Session 07 */
void anime_ui_state_error(void);    /**< TODO Session 07 */

#ifdef __cplusplus
}
#endif

#endif /* UI_ANIME_UI_H */
