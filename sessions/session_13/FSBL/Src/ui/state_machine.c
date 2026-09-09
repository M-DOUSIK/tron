/* state_machine.c — MedSight application state machine
 *
 * Session 07 changes:
 *  - HAL_Delay() replaced with osal_delay_ms() (yields CPU instead of busy-wait)
 *  - SD log calls use SD_Log_Event_Async() (non-blocking, queue-backed)
 *  - HAL_GetTick() kept as-is (works under FreeRTOS via vApplicationTickHook)
 *
 * Session 09B changes:
 *  - STATE_CAMERA_DISPENSE now runs the real face-detect + embed + gallery
 *    match pipeline (ai_vision.c) instead of a mock timer.
 *  - STATE_CAMERA_REGISTER is still a mock timer — real enrollment is
 *    Session 09's job, not this session's.
 *
 * Session 09 changes:
 *  - STATE_CAMERA_REGISTER now runs the real face-detect + embed pipeline
 *    (mirroring STATE_CAMERA_DISPENSE's proven camera_stop()/camera_start()
 *    + 3-retry pattern), then hands off to three new states —
 *    STATE_KEYBOARD_REGISTER, STATE_PILLCOUNT_REGISTER,
 *    STATE_CONFIRM_REGISTER — owned by registration_ui.c, which collect a
 *    name and daily pill count and call gallery_add_patient() on confirm.
 * Session 11 will wire the mascot state (MASCOT_SUCCESS/MASCOT_ERROR) and
 * full dispense-flow orchestration on top of this identification result.
 *
 * Session 12 changes:
 *  - The two face-capture states no longer BLOCK inside the AI pipeline.
 *    ai_vision_run_pipeline() used to be called straight from here, so the
 *    UI task sat inside the NPU for up to several seconds per capture with
 *    touch unpolled and the physical USER1 button dead. Inference now runs
 *    in its own µT-Kernel task (ai_vision.c, priority 3, below this task) and
 *    both capture states became small non-blocking phase machines built from
 *    the same "decide once, then poll HAL_GetTick()" pattern this file
 *    already uses everywhere else. Behaviour is unchanged — same 1.5 s
 *    preview window, same 3 attempts 500 ms apart, same camera_stop()
 *    sequencing — only who executes it and how this task waits.
 *  - FRAME-BUFFER OWNERSHIP. Between ai_vision_capture_request() and the
 *    matching ai_vision_capture_wait() success, the AI task owns
 *    BUFFER_ADDRESS: both NPU networks' activation scratch overlaps it (see
 *    the memory-hazard note at the top of ai_vision.c). This file must draw
 *    nothing at all across that window — which is why a USER1 press during a
 *    capture is recorded as s_cancel_pending and acted on only once the
 *    capture completes, instead of transitioning home and redrawing straight
 *    into memory the NPU is still using.
 *  - Hardening: gallery-full is refused up front, three failed capture
 *    attempts offer TRY AGAIN / CANCEL instead of dead-ending home, and an
 *    unavailable SD card is stated on the home screen instead of failing
 *    silently.
 *  - pill_count is the DOSE, not a stock level. The `pills_remaining`
 *    decrement-and-resave on every confirmed dose is gone, along with the
 *    "refill needed" alerts that were built on top of it — see ai_vision.h.
 */

#include "ui/state_machine.h"
#include "ui/touch_driver.h"
#include "ui/gui_draw.h"
#include "ui/anime_ui.h"
#include "ui/registration_ui.h" /* Session 09: keyboard/pillcount/confirm screens */
#include "ms_osal.h"      /* Session 07: OSAL delay instead of HAL_Delay */
#include "sd_logger.h"    /* Session 07: async log calls */
#include "ai_vision.h"    /* Session 09B: real face detect + embed + gallery match */
#include "stm32n6xx_hal.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

extern LTDC_HandleTypeDef   hltdc;
extern DCMIPP_HandleTypeDef hdcmipp;

/* Session 13, Part D: routine state-trace/bookkeeping output goes behind
 * MEDSIGHT_DEBUG (default off), same pattern as sd_diskio.c's
 * MEDSIGHT_DEBUG_DISKIO. Fault-reporting stays on printf() directly and
 * unconditional: the NPU-init-timeout warning in state_machine_init() and
 * the NPU latency measurement in poll_capture() are NOT touched by this
 * macro on purpose. */
#ifndef MEDSIGHT_DEBUG
#define MEDSIGHT_DEBUG 0
#endif
#if MEDSIGHT_DEBUG
#define MS_DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define MS_DBG_PRINTF(...) do { } while (0)
#endif

/* Session 10: how long STATE_CONFIRM_TAKEN waits for a tap ("I Took It" or
 * "Skip") before treating it as a missed confirmation (SOFTWARE_ARCHITECTURE
 * .md §7's "missed consumption confirmation" edge case). No specific value
 * is given in session_10.md; 30s was chosen as generous for an elderly user
 * reading a large on-screen button, without leaving the device stuck
 * mid-flow indefinitely. */
#define CONFIRM_TAKEN_TIMEOUT_MS  30000u

/* Session 09B: set around ai_vision_run_pipeline() calls. Named per
 * documents/prompts/session_08B.md step 6; the actual hardware freeze is
 * camera_stop() below (DCMIPP DMA into BUFFER_ADDRESS must really stop, not
 * just be flagged) — this flag is bookkeeping/diagnostic on top of that. */
static volatile bool g_isp_suspend = false;

/* Session 09B: identification result for the current STATE_CAMERA_DISPENSE
 * pass, decided once up front and then just displayed/timed out. */
static bool     s_dispense_identified = false;
static char     s_dispense_patient_name[PATIENT_NAME_MAX];
/* Gallery slot of the matched patient, and their dose — the number of pills
 * shown on STATE_DISPENSING and recorded in the audit log.
 *
 * Session 12 correction: this used to read (and decrement) a `pills_remaining`
 * stock counter. `pill_count` is the DOSE, fixed per patient; nothing about
 * dispensing changes it. See the comment on PatientRecord in ai_vision.h. */
static int      s_dispense_slot       = -1;
static uint8_t  s_dispense_pill_count = 0;
/* Session 10: true while STATE_CONFIRM_TAKEN is holding its post-tap
 * acknowledgement ("Thank You!") on screen before returning home. */
static bool     s_taken_ack_shown      = false;

/* Session 12: Sessions 09/10 each kept a "hold the error dialog on screen for
 * 2.5 s, then go home regardless" flag here (s_register_face_failed /
 * s_dispense_error_shown). Both are gone: a failed capture now goes to
 * STATE_FACE_RETRY, which waits for the user instead of deciding for them. */

/* Session 09: set once gallery_add_patient()/the "gallery full" message has
 * been shown on STATE_CONFIRM_REGISTER, so that screen can hold for a
 * couple of seconds before returning home. */
static bool     s_confirm_done = false;

/* ── Session 12: non-blocking face capture ──────────────────────────────── */

/* How long the live preview runs before the camera is frozen for the NPU
 * pass — unchanged from Sessions 09/10, just no longer an osal_delay_ms(). */
#define CAPTURE_PREVIEW_MS        1500u
/* One-shot warning if the AI task has not answered after this long. The wait
 * itself is deliberately NOT abandoned: the AI task still owns
 * BUFFER_ADDRESS, so pressing on would mean drawing into memory the NPU is
 * using. Sessions 09-11 had exactly the same unbounded behaviour (the
 * pipeline call simply blocked this task), so waiting is a non-regression;
 * the warning just makes a wedged NPU visible on the console instead of
 * looking like a UI freeze. */
#define CAPTURE_STUCK_WARN_MS    15000u

typedef enum {
    CAP_PREVIEW = 0,   /* camera live, letting the user get into frame       */
    CAP_WAITING        /* camera stopped, AI task owns BUFFER_ADDRESS        */
} capture_phase_t;

static capture_phase_t s_cap_phase       = CAP_PREVIEW;
static bool            s_cap_warned      = false;
/* True from ai_vision_capture_request() until its result has been consumed.
 * While set, nothing in this file may draw — see the header comment. */
static bool            s_cap_in_flight   = false;
/* USER1 pressed during a capture: honoured once the capture completes. */
static bool            s_cancel_pending  = false;

/* Which flow STATE_FACE_RETRY should resume — capture failures can come from
 * either registration or dispensing and TRY AGAIN must go back to the right
 * one — and what to say, since "no face at all" and "a face I do not know"
 * call for different advice. */
#define NO_FACE_MESSAGE  "I could not see a face.\n\n" \
                         "Please sit in front of the\n" \
                         "camera in good light."
/* Neither screen may sit on the display forever if the user walks away. */
#define FACE_RETRY_TIMEOUT_MS   30000u
#define ALERT_TIMEOUT_MS        30000u

static AppState_t      s_retry_target    = STATE_HOME;
static const char     *s_retry_message   = NO_FACE_MESSAGE;

/* STATE_ALERT context: what to show, and where to go when it is dismissed. */
static const char     *s_alert_title     = "";
static const char     *s_alert_message   = "";
static uint16_t        s_alert_accent    = COLOR_WARN;
static AppState_t      s_alert_next      = STATE_HOME;

static void show_alert(const char *title, const char *message,
                        uint16_t accent, AppState_t next_state);

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

/* ── Session 12 helpers ──────────────────────────────────────────────────── */

/* Queue a one-button alert screen. Safe to call from any state; the screen
 * itself is drawn on STATE_ALERT's entry, never from here, so this can be
 * called at a point where the framebuffer is not ours to draw into. */
static void show_alert(const char *title, const char *message,
                        uint16_t accent, AppState_t next_state)
{
    s_alert_title   = title;
    s_alert_message = message;
    s_alert_accent  = accent;
    s_alert_next    = next_state;
    current_state   = STATE_ALERT;
    state_init_done = false;
}

/* ── Session 13, Part A: the second framebuffer does not exist ────────────
 *
 * GUI_BUFFER_ADDRESS (0x342BB800) was going to be a second framebuffer so
 * the UI could keep drawing while the NPU ran. A canary check written for
 * exactly this question answered it on hardware, on the first capture:
 *
 *   CANARY FAIL: GUI_BUFFER_ADDRESS (0x342BB800-0x34376FFF) was overwritten
 *   during capture (first=3B185504 last=3E2699ED, expected C0FFEEA5)
 *
 * Both ends of the 750 KB region were clobbered, with what are plainly
 * float bit patterns — NPU activation data. So the activation scratch does
 * not merely start at BUFFER_ADDRESS, it runs past the whole of the second
 * framebuffer: at least 0x34200000-0x34376FFF, over 1.5 MB.
 *
 * prompts/session_13.md was explicit about this case: "If it does not fit,
 * say so and stop — do not shrink the framebuffer or move the NPU's pools
 * to make it fit." So this file is back to Session 12's single-buffer
 * behaviour, and the capture window is handled by blanking the display
 * instead (see display_blank()). See session_13_notes.md Addendum 2.
 * ──────────────────────────────────────────────────────────────────────── */

/* Blank the panel while the NPU owns BUFFER_ADDRESS, rather than leaving a
 * frozen camera frame on screen to be visibly scribbled over for ~750 ms.
 * Disabling the layer makes the LTDC show its background colour (set to the
 * theme blush in main.c's LCD_Init), which reads as an intentional pause.
 *
 * Gated so it can be turned off without unpicking anything if it ever
 * misbehaves on hardware: set to 0 and the old Session 12 behaviour (a
 * corrupting frozen frame) comes back. */
#ifndef MS_BLANK_DURING_CAPTURE
#define MS_BLANK_DURING_CAPTURE 1
#endif

static bool s_display_blanked = false;

static void display_blank(bool on)
{
#if MS_BLANK_DURING_CAPTURE
    if (on == s_display_blanked) return;
    if (on) __HAL_LTDC_LAYER_DISABLE(&hltdc, LTDC_LAYER_1);
    else    __HAL_LTDC_LAYER_ENABLE(&hltdc, LTDC_LAYER_1);
    __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);
    s_display_blanked = on;
#else
    (void)on;
#endif
}

/* Begin a face capture: freeze the camera and hand BUFFER_ADDRESS to the AI
 * task. Nothing in this file may draw until poll_capture() reports a result
 * other than "still running" — the NPU owns the one framebuffer we have. */
static void begin_capture(void)
{
    g_isp_suspend = true;
    camera_stop();
    s_cap_in_flight = true;
    s_cap_warned    = false;

    display_blank(true);

    ai_vision_capture_request();
    s_cap_phase = CAP_WAITING;
}

/* Poll the AI task without blocking. Returns AI_CAPTURE_TIMEOUT while the
 * capture is still running (the caller must simply come back next tick), and
 * clears the in-flight/ownership flag on any other result. */
static ai_capture_result_t poll_capture(int8_t *out_embedding, uint32_t since_ms)
{
    ai_capture_result_t r = ai_vision_capture_wait(out_embedding, OSAL_NO_WAIT);
    if (r == AI_CAPTURE_TIMEOUT) {
        if (!s_cap_warned && (since_ms > CAPTURE_STUCK_WARN_MS)) {
            s_cap_warned = true;
            MS_DBG_PRINTF("state_machine: WARNING AI capture still running after %ums.\n",
                   (unsigned)since_ms);
        }
        return r;
    }
    /* Session 13, Part E: NPU inference latency, wall-clock from
     * begin_capture()'s ai_vision_capture_request() to this result -
     * AI_PIPELINE.md §5 asked for a measured number and never had one.
     * since_ms already IS that interval (state_entry_time is reset right
     * after begin_capture() is called). One line, not a hot path, left
     * unconditional like the sleep-clocks readback — this is exactly the
     * number this session needs recorded from real hardware. */
    printf("AI capture: result after %ums (detector+embedder wall time, "
           "includes AI-task IPC/scheduling).\r\n", (unsigned)since_ms);

    g_isp_suspend   = false;
    s_cap_in_flight = false;
    return r;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PUBLIC API
 * ═══════════════════════════════════════════════════════════════════════════ */

/* How long state_machine_init() will wait for the NPU to finish coming up
 * before drawing anyway. ai_vision_init() takes roughly 0.5-1 s on this
 * board; 20 s is far beyond any legitimate value, so reaching it means the
 * AI task is wedged, not slow. In that case the UI comes up regardless —
 * losing face recognition is bad, but a blank screen is worse, and the
 * console line below says exactly which of the two happened. */
#define AI_INIT_WAIT_MS   20000u

void state_machine_init(void)
{
    /* Touch first: gt911_hardware_reset() spends 140 ms in osal_delay_ms(),
     * which is 140 ms of NPU bring-up the wait below will not have to pay
     * for. Neither call touches the framebuffer. */
    touch_driver_init();
    user_button_init();
    was_touching = false;

    camera_stop();

    /* NOTHING above this line writes to BUFFER_ADDRESS; nothing below it may
     * run until the NPU has stopped writing there. See the long comment on
     * ai_vision_wait_init() in ai_vision.h — this one line is the whole fix
     * for the cold-boot "screen draws, glitches, then greys out" fault. */
    if (!ai_vision_wait_init(AI_INIT_WAIT_MS)) {
        printf("state_machine: NPU init did not finish in %ums - "
               "drawing anyway, face recognition may be unavailable.\n",
               AI_INIT_WAIT_MS);
    }

    /* One framebuffer. GUI_BUFFER_ADDRESS was proven unusable on hardware —
     * see the Part A note above begin_capture(). */
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

    /* Physical USER1 button → back to home.
     *
     * Session 12: this now works DURING a face capture, which it could not
     * before (the UI task was blocked inside the pipeline and never reached
     * this line). It must not act immediately though: the AI task owns
     * BUFFER_ADDRESS until its capture finishes, and going home here would
     * redraw the home screen into memory the NPU is still writing. Record
     * the intent and let the capture state honour it on completion. */
    if (btn_pressed && current_state != STATE_HOME)
    {
        if (s_cap_in_flight)
        {
            if (!s_cancel_pending)
            {
                s_cancel_pending = true;
                MS_DBG_PRINTF("state_machine: USER1 during capture - cancel pending.\n");
            }
        }
        else
        {
            camera_stop();
            anime_ui_set_state(MASCOT_IDLE);
            current_state   = STATE_HOME;
            state_init_done = false;
            was_touching    = false;
            /* Session 07: use OSAL delay instead of HAL_Delay — yields CPU */
            osal_delay_ms(200);
            return;
        }
    }

    switch (current_state)
    {
        /* ── HOME ─────────────────────────────────────────────────────── */
        case STATE_HOME:
            if (!state_init_done)
            {
                camera_stop();
                switch_ltdc_buffer(BUFFER_ADDRESS);
                gui_draw_init(BUFFER_ADDRESS, FRAME_WIDTH, FRAME_HEIGHT);
                anime_ui_set_dest_buffer(BUFFER_ADDRESS);
                anime_ui_set_bg_color(COLOR_BG);
                gui_draw_home_screen();

                /* Session 12: an unusable SD card is stated, not hidden. The
                 * device deliberately keeps working without one — dispensing
                 * a dose matters more than recording it — but the operator
                 * has to be able to see that the audit log is not being
                 * written. Drawn over the home screen's dialog box, so it
                 * costs no layout and disappears by itself once the card is
                 * back and the screen is next redrawn. */
                if (!SD_Logger_Is_Available())
                {
                    gui_draw_dialog_text(
                        "No SD card - doses are not being recorded.\n"
                        "The device still works normally.");
                }

                state_init_done = true;
                MS_DBG_PRINTF("STATE_HOME\n");
                /* Session 07: async log via queue — does not block the UI task */
                SD_Log_Event_Async("STATE: HOME");
            }

            anime_ui_update(HAL_GetTick());

            if (new_touch)
            {
                if (check_hit(tx, ty, REG_BTN_X, REG_BTN_Y, REG_BTN_W, REG_BTN_H))
                {
                    /* Session 12: refuse a full gallery HERE, before asking
                     * the patient to stand in front of the camera and run a
                     * capture whose result can only be thrown away. Session
                     * 09 checked at the very end of the flow, after the face
                     * capture, the keyboard and the pill count. */
                    if (gallery_count() >= MAX_PATIENTS)
                    {
                        SD_Log_Event_Async("EVENT: Registration - refused, gallery full");
                        show_alert("GALLERY FULL",
                                   "This device already holds\n"
                                   "10 patients, the maximum.\n\n"
                                   "Please contact your administrator.",
                                   COLOR_WARN, STATE_HOME);
                        break;
                    }
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
                    "REGISTER PATIENT", ACCENT_NEUTRAL,
                    "Let's get you set up!\n"
                    "Face the camera, then press READY."
                );
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                was_touching     = true;   /* force release before new touch */
                MS_DBG_PRINTF("STATE_INSTRUCT_REGISTER\n");
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
        /* Session 09 established this flow: a brief live-preview window so
         * the user can get into frame, then freeze the camera DMA and run
         * the NPU pass, retrying up to three times. The captured embedding
         * is only HELD (in registration_ui's static buffer) — nothing
         * reaches the gallery until the user has typed a name and a pill
         * count and tapped Confirm.
         *
         * UNLIKE STATE_CAMERA_DISPENSE, the camera is deliberately NOT
         * resumed after the capture attempt (found on real hardware in
         * Session 09: resuming it left the DCMIPP DMA continuously
         * overwriting BUFFER_ADDRESS, which wiped out both the keyboard
         * screen drawn on success and the error dialog drawn on failure).
         * Every state from here on is static UI, not a camera preview, so
         * the camera must stay off.
         *
         * Session 12 keeps all of that and removes only the blocking: the
         * preview window is a timestamp comparison instead of
         * osal_delay_ms(1500), and the retry loop lives in the AI task. */
        case STATE_CAMERA_REGISTER:
            if (!state_init_done)
            {
                camera_start();
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                s_cap_phase      = CAP_PREVIEW;
                s_cancel_pending = false;
                MS_DBG_PRINTF("STATE_CAMERA_REGISTER\n");
                SD_Log_Event_Async("STATE: CAMERA_REGISTER");
            }

            if (s_cap_phase == CAP_PREVIEW)
            {
                if ((HAL_GetTick() - state_entry_time) > CAPTURE_PREVIEW_MS)
                {
                    begin_capture();
                    state_entry_time = HAL_GetTick();
                }
            }
            else if (s_cap_phase == CAP_WAITING)
            {
                int8_t embedding[EMBEDDING_SIZE];
                ai_capture_result_t r =
                    poll_capture(embedding, HAL_GetTick() - state_entry_time);

                if (r == AI_CAPTURE_TIMEOUT)
                {
                    break;   /* still running; the AI task owns the frame */
                }

                if (s_cancel_pending)
                {
                    /* USER1 was pressed mid-capture. Now that the AI task
                     * has released BUFFER_ADDRESS it is safe to redraw. */
                    s_cancel_pending = false;
                    MS_DBG_PRINTF("Registration: cancelled by USER1.\n");
                    SD_Log_Event_Async("EVENT: Registration - cancelled");
                    current_state   = STATE_HOME;
                    state_init_done = false;
                }
                else if (r == AI_CAPTURE_OK)
                {
                    registration_ui_reset();
                    registration_ui_set_embedding(embedding);
                    MS_DBG_PRINTF("Registration: face captured.\n");
                    SD_Log_Event_Async("EVENT: Registration - face captured");
                    current_state   = STATE_KEYBOARD_REGISTER;
                    state_init_done = false;
                }
                else if (r == AI_CAPTURE_NOT_READY)
                {
                    MS_DBG_PRINTF("Registration: AI pipeline not ready.\n");
                    SD_Log_Event_Async("EVENT: Registration - AI not ready");
                    show_alert("CAMERA NOT READY",
                               "The face camera did not start.\n\n"
                               "Please restart the device.",
                               COLOR_ALERT, STATE_HOME);
                }
                else
                {
                    /* Session 12: three failed attempts used to dead-end
                     * with a 2.5 s dialog and an unconditional return home.
                     * Offer the retry the user obviously wants instead. */
                    MS_DBG_PRINTF("Registration: no face detected after 3 attempts.\n");
                    SD_Log_Event_Async("EVENT: Registration - no face detected");
                    s_retry_target  = STATE_CAMERA_REGISTER;
                    s_retry_message = NO_FACE_MESSAGE;
                    current_state   = STATE_FACE_RETRY;
                    state_init_done = false;
                }
            }
            break;

        /* ── REGISTER keyboard (on-screen name entry) ───────────────────── */
        case STATE_KEYBOARD_REGISTER:
            if (!state_init_done)
            {
                registration_ui_draw_keyboard();
                state_init_done = true;
                was_touching    = true;   /* force release before new touch */
                MS_DBG_PRINTF("STATE_KEYBOARD_REGISTER\n");
                SD_Log_Event_Async("STATE: KEYBOARD_REGISTER");
            }

            if (new_touch && registration_ui_handle_keyboard_touch(tx, ty))
            {
                current_state   = STATE_PILLCOUNT_REGISTER;
                state_init_done = false;
            }
            break;

        /* ── REGISTER pill count (daily dose count, 1-10) ───────────────── */
        case STATE_PILLCOUNT_REGISTER:
            if (!state_init_done)
            {
                registration_ui_draw_pillcount();
                state_init_done = true;
                was_touching    = true;
                MS_DBG_PRINTF("STATE_PILLCOUNT_REGISTER\n");
                SD_Log_Event_Async("STATE: PILLCOUNT_REGISTER");
            }

            if (new_touch && registration_ui_handle_pillcount_touch(tx, ty))
            {
                current_state   = STATE_CONFIRM_REGISTER;
                state_init_done = false;
            }
            break;

        /* ── REGISTER confirm (summary, save or retry) ──────────────────── */
        case STATE_CONFIRM_REGISTER:
            if (!state_init_done)
            {
                registration_ui_draw_confirm();
                state_init_done  = true;
                was_touching     = true;
                MS_DBG_PRINTF("STATE_CONFIRM_REGISTER\n");
                SD_Log_Event_Async("STATE: CONFIRM_REGISTER");
            }

            if (new_touch && !s_confirm_done)
            {
                reg_confirm_result_t result = registration_ui_handle_confirm_touch(tx, ty);
                switch (result)
                {
                    case REG_CONFIRM_SAVED:
                        gui_draw_dialog_text("Registered!\nWelcome aboard.");
                        SD_Log_Event_Async("EVENT: Registration - patient saved");
                        s_confirm_done    = true;
                        state_entry_time  = HAL_GetTick();
                        break;
                    case REG_CONFIRM_SAVED_NO_SD:
                        /* Session 13, found on hardware: the patient IS in the
                         * gallery and will be recognised right now, but
                         * patients.dat could not be written, so the enrolment
                         * dies with the power. Saying "Registered!" here was a
                         * straight untruth about whether the data is safe. */
                        gui_draw_dialog_text("Saved for now - no SD card.\n"
                                             "Re-register after restarting.");
                        SD_Log_Event_Async("EVENT: Registration - saved to RAM only (no SD)");
                        s_confirm_done    = true;
                        state_entry_time  = HAL_GetTick();
                        break;
                    case REG_CONFIRM_FULL:
                        /* Session 12: normally unreachable now — the home
                         * screen refuses a full gallery before the flow even
                         * starts. Kept as the backstop for the one case that
                         * check cannot cover: a slot filled by something else
                         * between then and now. Upgraded from a 2 s dialog to
                         * the same alert screen, so the two paths say the same
                         * thing. */
                        SD_Log_Event_Async("EVENT: Registration - gallery full");
                        show_alert("GALLERY FULL",
                                   "This device already holds\n"
                                   "10 patients, the maximum.\n\n"
                                   "Please contact your administrator.",
                                   COLOR_WARN, STATE_HOME);
                        break;
                    case REG_CONFIRM_RETRY:
                        current_state    = STATE_CAMERA_REGISTER;
                        state_init_done  = false;
                        break;
                    case REG_CONFIRM_NONE:
                    default:
                        break;
                }
            }

            if (s_confirm_done && (HAL_GetTick() - state_entry_time > 2000u))
            {
                s_confirm_done   = false;
                current_state    = STATE_HOME;
                state_init_done  = false;
            }
            break;

        /* ── DISPENSE instruction screen ──────────────────────────────── */
        case STATE_INSTRUCT_DISPENSE:
            if (!state_init_done)
            {
                gui_draw_ready_screen(
                    "DISPENSE PILLS", ACCENT_DISPENSE,
                    "Let's find your pills!\n"
                    "Face the camera, then press READY."
                );
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                was_touching     = true;
                MS_DBG_PRINTF("STATE_INSTRUCT_DISPENSE\n");
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

        /* ── DISPENSE camera (identify against the patient gallery) ────── */
        /* Same shape as STATE_CAMERA_REGISTER above: preview window, freeze
         * the camera, hand the frame to the AI task, poll for the result.
         *
         * Session 10 established that the camera stays STOPPED from here on
         * — every remaining screen in the dispense flow (the error screens,
         * STATE_DISPENSING, STATE_CONFIRM_TAKEN) is static UI, not a camera
         * preview, and resuming the DCMIPP DMA would race those draws
         * exactly like the bug STATE_CAMERA_REGISTER hit and fixed in
         * Session 09 ("Bug 2" in session_09_notes.md). That still holds. */
        case STATE_CAMERA_DISPENSE:
            if (!state_init_done)
            {
                camera_start();
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                s_cap_phase      = CAP_PREVIEW;
                s_cancel_pending = false;
                MS_DBG_PRINTF("STATE_CAMERA_DISPENSE\n");
                SD_Log_Event_Async("STATE: CAMERA_DISPENSE");
            }

            if (s_cap_phase == CAP_PREVIEW)
            {
                if ((HAL_GetTick() - state_entry_time) > CAPTURE_PREVIEW_MS)
                {
                    begin_capture();
                    state_entry_time = HAL_GetTick();
                }
            }
            else if (s_cap_phase == CAP_WAITING)
            {
                int8_t embedding[EMBEDDING_SIZE];
                ai_capture_result_t r =
                    poll_capture(embedding, HAL_GetTick() - state_entry_time);

                if (r == AI_CAPTURE_TIMEOUT)
                {
                    break;   /* still running; the AI task owns the frame */
                }

                s_dispense_identified = false;
                s_dispense_slot       = -1;
                s_dispense_patient_name[0] = '\0';

                if (s_cancel_pending)
                {
                    s_cancel_pending = false;
                    MS_DBG_PRINTF("Dispense: cancelled by USER1.\n");
                    SD_Log_Event_Async("EVENT: Dispense - cancelled");
                    current_state   = STATE_HOME;
                    state_init_done = false;
                }
                else if (r == AI_CAPTURE_NOT_READY)
                {
                    MS_DBG_PRINTF("Dispense: AI pipeline not ready.\n");
                    SD_Log_Event_Async("EVENT: Dispense - AI not ready");
                    show_alert("CAMERA NOT READY",
                               "The face camera did not start.\n\n"
                               "Please restart the device.",
                               COLOR_ALERT, STATE_HOME);
                }
                else if (r == AI_CAPTURE_OK)
                {
                    float confidence = -1.0f;
                    int   slot = gallery_find_best_match(embedding, &confidence);
                    if (slot >= 0)
                    {
                        s_dispense_identified = true;
                        s_dispense_slot       = slot;
                        s_dispense_pill_count = patient_gallery[slot].pill_count;
                        strncpy(s_dispense_patient_name, patient_gallery[slot].name,
                                PATIENT_NAME_MAX - 1);
                        s_dispense_patient_name[PATIENT_NAME_MAX - 1] = '\0';
                        MS_DBG_PRINTF("Dispense: matched patient '%s' (%u pill(s) per dose).\n",
                               s_dispense_patient_name,
                               (unsigned)s_dispense_pill_count);
                        SD_Log_Event_Async("EVENT: Dispense - patient matched");

                        /* Session 12 removed a "no pills remaining" check that
                         * used to live here. It was reading a software stock
                         * counter that this device has no way to keep honest —
                         * nothing tells the firmware when a carer tops the
                         * hopper up. Real hopper-level sensing arrives in
                         * Session 14 with the IR break-beam counter, which
                         * measures pills physically dropping instead of
                         * assuming a number. */
                        current_state   = STATE_DISPENSING;
                        state_init_done = false;
                    }
                    else
                    {
                        /* A face WAS found, it just is not in the gallery.
                         * Still logged as an intruder event (that is the
                         * security-relevant record), but the user now gets
                         * the retry Session 10 did not offer — a marginal
                         * match often succeeds on a second, better-framed
                         * attempt. */
                        MS_DBG_PRINTF("Dispense: face detected but no gallery match (intruder).\n");
                        SD_Log_Event_Async("EVENT: Dispense - INTRUDER (no gallery match)");
                        s_retry_target  = STATE_CAMERA_DISPENSE;
                        s_retry_message = "Sorry, I do not recognise you.\n\n"
                                          "Move a little closer and\n"
                                          "look straight at the camera.";
                        current_state   = STATE_FACE_RETRY;
                        state_init_done = false;
                    }
                }
                else
                {
                    MS_DBG_PRINTF("Dispense: no face detected after 3 attempts.\n");
                    SD_Log_Event_Async("EVENT: Dispense - no face detected");
                    s_retry_target  = STATE_CAMERA_DISPENSE;
                    s_retry_message = NO_FACE_MESSAGE;
                    current_state   = STATE_FACE_RETRY;
                    state_init_done = false;
                }
            }
            break;

        /* ── FACE RETRY (Session 12) ────────────────────────────────────── */
        /* Session 10 ended a failed identification with a 2.5 s dialog and an
         * unconditional return to the home screen — the user then had to walk
         * the whole flow again from the top to get a second attempt at
         * something that fails for entirely ordinary reasons (standing too
         * far back, looking away, poor light). This offers the retry
         * directly. It is an error path, not a new feature. */
        case STATE_FACE_RETRY:
            if (!state_init_done)
            {
                gui_draw_two_choice_screen("FACE NOT RECOGNISED",
                                           s_retry_message,
                                           COLOR_WARN,
                                           "TRY AGAIN", "CANCEL");
                /* Session 13: the mascot has a real error state now - the
                 * designer's three-frame crying pose, animated in the box
                 * this screen reserves for it (MASCOT_SAD_*). The screen
                 * draws frame 0 itself; anime_ui takes over from here. */
                anime_ui_set_state(MASCOT_ERROR);
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                was_touching     = true;   /* force release before new touch */
                MS_DBG_PRINTF("STATE_FACE_RETRY\n");
                SD_Log_Event_Async("STATE: FACE_RETRY");
            }

            anime_ui_update(HAL_GetTick());

            if (new_touch && (HAL_GetTick() - state_entry_time > 500u))
            {
                if (check_hit(tx, ty, CHOICE_LEFT_X, CHOICE_BTN_Y,
                              CHOICE_BTN_W, CHOICE_BTN_H))
                {
                    anime_ui_set_state(MASCOT_IDLE);
                    current_state   = s_retry_target;
                    state_init_done = false;
                }
                else if (check_hit(tx, ty, CHOICE_RIGHT_X, CHOICE_BTN_Y,
                                   CHOICE_BTN_W, CHOICE_BTN_H))
                {
                    anime_ui_set_state(MASCOT_IDLE);
                    current_state   = STATE_HOME;
                    state_init_done = false;
                }
            }
            /* Never strand the device on this screen if nobody answers. */
            else if ((HAL_GetTick() - state_entry_time) > FACE_RETRY_TIMEOUT_MS)
            {
                MS_DBG_PRINTF("Face retry: timed out, returning home.\n");
                anime_ui_set_state(MASCOT_IDLE);
                current_state   = STATE_HOME;
                state_init_done = false;
            }
            break;

        /* ── ALERT (Session 12) ─────────────────────────────────────────── */
        /* One-button screen for conditions the user has to act on outside the
         * device: the gallery is full, a patient has no pills left, or the SD
         * card is not writable. Each of these previously either failed
         * silently or flashed past in a timed dialog. */
        case STATE_ALERT:
            if (!state_init_done)
            {
                gui_draw_alert_screen(s_alert_title, s_alert_message,
                                      s_alert_accent, "OK");
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                was_touching     = true;
                MS_DBG_PRINTF("STATE_ALERT: %s\n", s_alert_title);
                /* Session 12 Part C5: every state transition reaches the SD
                 * audit log, this one included. The specific condition
                 * (gallery full, refill needed, save failed) was already
                 * logged by whoever raised the alert. */
                SD_Log_Event_Async("STATE: ALERT");
            }

            if (new_touch && (HAL_GetTick() - state_entry_time > 500u) &&
                check_hit(tx, ty, ALERT_BTN_X, ALERT_BTN_Y,
                          ALERT_BTN_W, ALERT_BTN_H))
            {
                current_state   = s_alert_next;
                state_init_done = false;
            }
            else if ((HAL_GetTick() - state_entry_time) > ALERT_TIMEOUT_MS)
            {
                current_state   = s_alert_next;
                state_init_done = false;
            }
            break;

        /* ── DISPENSING (simulated dispense animation) ──────────────────── */
        case STATE_DISPENSING:
            if (!state_init_done)
            {
                state_init_done = true;
                MS_DBG_PRINTF("STATE_DISPENSING\n");
                SD_Log_Event_Async("STATE: DISPENSING");

                char log_line[SD_LOG_MSG_MAX_LEN];
                snprintf(log_line, sizeof(log_line), "DISPENSE: %s %u pills",
                          s_dispense_patient_name, (unsigned)s_dispense_pill_count);
                SD_Log_Event_Async(log_line);

                gui_draw_dispensing_screen(s_dispense_patient_name, s_dispense_pill_count);

                /* Session 13: unblank HERE, not at the bottom of this
                 * function. This state does all its work inside its own entry
                 * block and clears state_init_done again before it breaks, so
                 * the end-of-function unblank (which waits for
                 * state_init_done) never fired for it — the whole dispensing
                 * screen and its two-second progress animation ran behind a
                 * disabled LTDC layer and the user simply never saw the
                 * progress bar. The screen is on the framebuffer by this
                 * line, so it is safe to show it. */
                display_blank(false);

                /* ~2s animated countdown — a filled-bar progress wipe, per
                 * session_10.md ("a progress bar is fine"). Blocking here is
                 * consistent with this file's existing pattern of blocking
                 * for known, bounded operations (e.g. the capture retry
                 * loops above).
                 *
                 * Session 13: ONE STEP PER PILL. This ran 1..10 regardless of
                 * the dose, so a 6-pill dose showed a percentage marching
                 * 10, 20, 30... that had nothing to do with the six pills.
                 * A first fix kept four smooth sub-steps inside each pill's
                 * slice, which was still wrong for the same reason at a
                 * smaller scale: at a 2-pill dose the number still ticked
                 * through eight values for two pills.
                 *
                 * The bar is a report on a physical event, not a spinner.
                 * One pill leaves the hopper, one gem lights, the bar and
                 * the number move once. At pill 1 of 2 the bar is exactly
                 * half full and reads 50%, and it holds there until the
                 * second pill drops. When Session 14 replaces this dwell
                 * with a real IR break-beam count, the loop body is already
                 * the right shape — only the wait changes. */
                {
                    uint32_t pills = (s_dispense_pill_count > 0u)
                                     ? s_dispense_pill_count : 1u;
                    /* Time per pill: ~2 s of travel spread over the dose,
                     * but never so brief that a step cannot be read, nor so
                     * long that a single-pill dose feels stalled. */
                    uint32_t dwell = 2000u / pills;
                    if (dwell < 300u)  dwell = 300u;
                    if (dwell > 900u)  dwell = 900u;

                    for (uint32_t pill = 1; pill <= pills; pill++)
                    {
                        osal_delay_ms(dwell);   /* the pill falls... */
                        gui_draw_dispensing_progress(
                            (uint16_t)(pill * 100u / pills), (uint8_t)pill);
                    }
                }

                current_state   = STATE_CONFIRM_TAKEN;
                state_init_done = false;
            }
            break;

        /* ── CONFIRM TAKEN ("I Took It" / "Skip") ───────────────────────── */
        case STATE_CONFIRM_TAKEN:
            if (!state_init_done)
            {
                gui_draw_confirm_taken_screen();
                state_init_done  = true;
                state_entry_time = HAL_GetTick();
                was_touching     = true;   /* force release before new touch */
                MS_DBG_PRINTF("STATE_CONFIRM_TAKEN\n");
                SD_Log_Event_Async("STATE: CONFIRM_TAKEN");
            }

            if (!s_taken_ack_shown && new_touch)
            {
                if (check_hit(tx, ty, TAKEN_BTN_X, TAKEN_BTN_Y, TAKEN_BTN_W, TAKEN_BTN_H))
                {
                    char log_line[SD_LOG_MSG_MAX_LEN];
                    snprintf(log_line, sizeof(log_line), "CONFIRMED: %s took pills",
                              s_dispense_patient_name);
                    SD_Log_Event_Async(log_line);

                    /* Session 12 correction: nothing is written to the
                     * gallery here any more.
                     *
                     * Sessions 10-12 decremented a `pills_remaining` field and
                     * re-saved patients.dat on every confirmed dose. That was
                     * wrong — pill_count is the DOSE, not a stock level (see
                     * ai_vision.h) — and it also meant a full 1.6 KB card
                     * write on the patient's most latency-sensitive tap, for
                     * data that never needed to change. The dose itself is
                     * still recorded, in the append-only event log above,
                     * which is where an adherence record belongs.
                     *
                     * The patient record is now written exactly once, at
                     * registration. */

                    gui_draw_taken_thankyou_screen();
                    s_taken_ack_shown = true;
                    state_entry_time  = HAL_GetTick();
                }
                else if (check_hit(tx, ty, SKIP_BTN_X, SKIP_BTN_Y, SKIP_BTN_W, SKIP_BTN_H))
                {
                    /* Caretaker skip. session_10.md's literal instruction was
                     * "returns home without logging", and Session 10 read
                     * that as writing nothing at all. Session 12's Part C5
                     * log review revised that: a dose that was dispensed and
                     * then NOT confirmed is exactly the kind of thing an
                     * adherence audit trail exists to record, and an audit
                     * log with a silent hole in it is worse than one with an
                     * inconvenient entry. No CONFIRMED line is written — the
                     * prompt's actual requirement — but the skip itself is. */
                    MS_DBG_PRINTF("Dispense: consumption confirmation skipped.\n");
                    SD_Log_Event_Async("EVENT: Dispense - confirmation SKIPPED (caretaker)");
                    current_state   = STATE_HOME;
                    state_init_done = false;
                }
            }

            if (s_taken_ack_shown && (HAL_GetTick() - state_entry_time > 1500u))
            {
                s_taken_ack_shown = false;
                current_state     = STATE_HOME;
                state_init_done   = false;
            }
            else if (!s_taken_ack_shown &&
                     (HAL_GetTick() - state_entry_time > CONFIRM_TAKEN_TIMEOUT_MS))
            {
                /* Missed consumption confirmation — neither button tapped
                 * within the timeout. Per SOFTWARE_ARCHITECTURE.md §7's
                 * edge-case table: log and return home. */
                MS_DBG_PRINTF("Dispense: consumption confirmation timed out.\n");
                SD_Log_Event_Async("EVENT: Dispense - missed confirmation (timeout)");
                current_state   = STATE_HOME;
                state_init_done = false;
            }
            break;
    }

    /* Unblank once the capture is finished AND the state that follows it has
     * drawn itself. state_init_done is exactly that signal: it is set at the
     * end of every state's entry block, after its screen is on the
     * framebuffer. Unblanking any earlier would show the corrupted frame the
     * NPU left behind, which is the whole thing this is avoiding. */
    if (s_display_blanked && !s_cap_in_flight && state_init_done)
    {
        display_blank(false);
    }
}
