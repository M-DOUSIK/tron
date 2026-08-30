/* state_machine.c — MedSight application state machine
 *
 * Session 07 changes:
 *  - HAL_Delay() replaced with osal_delay_ms() (yields CPU instead of busy-wait)
 *  - SD log calls use SD_Log_Event_Async() (non-blocking, queue-backed)
 *  - HAL_GetTick() kept as-is (works under FreeRTOS via vApplicationTickHook)
 *
 * Session 11 will replace this placeholder camera state with real AI inference.
 */

#include "ui/state_machine.h"
#include "ui/touch_driver.h"
#include "ui/gui_draw.h"
#include "ui/anime_ui.h"
#include "ms_osal.h"      /* Session 07: OSAL delay instead of HAL_Delay */
#include "sd_logger.h"    /* Session 07: async log calls */
#include "stm32n6xx_hal.h"
#include "main.h"
#include <stdio.h>

extern LTDC_HandleTypeDef   hltdc;
extern DCMIPP_HandleTypeDef hdcmipp;

/* ── Camera helpers ─────────────────────────────────────────────────────── */
static void camera_stop(void)
{
    HAL_DCMIPP_CSI_PIPE_Stop(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0);
}
static void camera_start(void)
{
    HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp, DCMIPP_PIPE1,
                               DCMIPP_VIRTUAL_CHANNEL0,
                               BUFFER_ADDRESS, DCMIPP_MODE_CONTINUOUS);
}
static void switch_ltdc_buffer(uint32_t addr)
{
    HAL_LTDC_SetAddress(&hltdc, addr, LTDC_LAYER_1);
}

/* ── Hitbox test ─────────────────────────────────────────────────────────── */
static bool check_hit(uint32_t tx, uint32_t ty,
                       uint32_t x,  uint32_t y,
                       uint32_t w,  uint32_t h)
{
    return (tx >= x && tx <= (x+w) && ty >= y && ty <= (y+h));
}

/* ── State ───────────────────────────────────────────────────────────────── */
static AppState_t current_state   = STATE_HOME;
static bool       state_init_done  = false;
static uint32_t   state_entry_time = 0;

/*
 * TOUCH EDGE DETECTION
 * ────────────────────
 * was_touching tracks whether the finger was down on the PREVIOUS call.
 * We only act on a NEW touch (rising edge: !was_touching && touched now).
 * This prevents the same finger-down from cascading through multiple states.
 */
static bool was_touching = false;

/* ═══════════════════════════════════════════════════════════════════════════
 * PUBLIC API
 * ═══════════════════════════════════════════════════════════════════════════ */

void state_machine_init(void)
{
    touch_driver_init();
    user_button_init();
    was_touching = false;

    camera_stop();

    gui_draw_init(BUFFER_ADDRESS, FRAME_WIDTH, FRAME_HEIGHT);
    switch_ltdc_buffer(BUFFER_ADDRESS);

    anime_ui_set_dest_buffer(BUFFER_ADDRESS);
    anime_ui_set_bg_color(COLOR_BG);

    gui_draw_home_screen();

    current_state  = STATE_HOME;
    state_init_done = true;
}

void state_machine_update(void)
{
    uint32_t tx = 0, ty = 0;
    bool touched   = touch_driver_get_touch(&tx, &ty);
    bool new_touch = touched && !was_touching;   /* rising-edge only */
    was_touching   = touched;

    bool btn_pressed = user_button_is_pressed();

    /* Physical USER1 button → back to home */
    if (btn_pressed && current_state != STATE_HOME)
    {
        camera_stop();
        current_state   = STATE_HOME;
        state_init_done = false;
        was_touching    = false;
        /* Session 07: use OSAL delay instead of HAL_Delay — yields CPU */
        osal_delay_ms(200);
        return;
    }

    switch (current_state)
    {
        /* ── HOME ─────────────────────────────────────────────────────── */
        case STATE_HOME:
            if (!state_init_done)
            {
                camera_stop();
                switch_ltdc_buffer(BUFFER_ADDRESS);
                anime_ui_set_dest_buffer(BUFFER_ADDRESS);
                anime_ui_set_bg_color(COLOR_BG);
                gui_draw_home_screen();
                state_init_done = true;
                printf("STATE_HOME\n");
                /* Session 07: async log via queue — does not block the UI task */
                SD_Log_Event_Async("STATE: HOME");
            }

            anime_ui_update(HAL_GetTick());

            if (new_touch)
            {
                if (check_hit(tx, ty, REG_BTN_X, REG_BTN_Y, REG_BTN_W, REG_BTN_H))
                {
                    current_state   = STATE_INSTRUCT_REGISTER;
                    state_init_done = false;
                }
                else if (check_hit(tx, ty, DISP_BTN_X, DISP_BTN_Y, DISP_BTN_W, DISP_BTN_H))
                {
                    current_state   = STATE_INSTRUCT_DISPENSE;
                    state_init_done = false;
                }
            }
            break;

        /* ── REGISTER instruction screen ─────────────────────────────── */
        case STATE_INSTRUCT_REGISTER:
            if (!state_init_done)
            {
                gui_draw_ready_screen(
                    "Please face the camera.\n"
                    "\n"
                    "Press READY when set."
                );
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                was_touching     = true;   /* force release before new touch */
                printf("STATE_INSTRUCT_REGISTER\n");
                SD_Log_Event_Async("STATE: INSTRUCT_REGISTER");
            }

            anime_ui_update(HAL_GetTick());

            /*
             * TOUCH GUARD — two conditions BOTH must be true:
             *  1. Rising-edge touch (new_touch) — finger was up, now down
             *  2. At least 500 ms in this state — prevents bleed from the
             *     button tap that CAUSED the state transition.
             */
            if (new_touch &&
                (HAL_GetTick() - state_entry_time > 500u) &&
                check_hit(tx, ty, READY_BTN_X, READY_BTN_Y, READY_BTN_W, READY_BTN_H))
            {
                current_state   = STATE_CAMERA_REGISTER;
                state_init_done = false;
            }
            break;

        /* ── REGISTER camera (face scan) ──────────────────────────────── */
        case STATE_CAMERA_REGISTER:
            if (!state_init_done)
            {
                camera_start();
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                printf("STATE_CAMERA_REGISTER\n");
                SD_Log_Event_Async("STATE: CAMERA_REGISTER");
            }
            /* Mock: 5 s then return home */
            if (HAL_GetTick() - state_entry_time > 5000u)
            {
                printf("Registration complete (mock).\n");
                SD_Log_Event_Async("EVENT: Registration complete (mock)");
                current_state   = STATE_HOME;
                state_init_done = false;
            }
            break;

        /* ── DISPENSE instruction screen ──────────────────────────────── */
        case STATE_INSTRUCT_DISPENSE:
            if (!state_init_done)
            {
                gui_draw_ready_screen(
                    "Please face the camera.\n"
                    "\n"
                    "Press READY when set."
                );
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                was_touching     = true;
                printf("STATE_INSTRUCT_DISPENSE\n");
                SD_Log_Event_Async("STATE: INSTRUCT_DISPENSE");
            }

            anime_ui_update(HAL_GetTick());

            /* Same 500ms dwell guard as INSTRUCT_REGISTER */
            if (new_touch &&
                (HAL_GetTick() - state_entry_time > 500u) &&
                check_hit(tx, ty, READY_BTN_X, READY_BTN_Y, READY_BTN_W, READY_BTN_H))
            {
                current_state   = STATE_CAMERA_DISPENSE;
                state_init_done = false;
            }
            break;

        /* ── DISPENSE camera (identify + collect + adherence) ─────────── */
        case STATE_CAMERA_DISPENSE:
            if (!state_init_done)
            {
                camera_start();
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                printf("STATE_CAMERA_DISPENSE\n");
                SD_Log_Event_Async("STATE: CAMERA_DISPENSE");
            }
            /* Mock: 8 s then return home */
            if (HAL_GetTick() - state_entry_time > 8000u)
            {
                printf("Dispense complete (mock).\n");
                SD_Log_Event_Async("EVENT: Dispense complete (mock)");
                current_state   = STATE_HOME;
                state_init_done = false;
            }
            break;
    }
}
