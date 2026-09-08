#ifndef GUI_DRAW_H
#define GUI_DRAW_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Elderly-Friendly Colour Palette (RGB565) ───────────────────────────────
 *
 *  Design priorities:
 *   • High contrast — all text at least 4.5:1 ratio against background
 *   • Warm, non-clinical colours — avoids cold blue/grey hospital feel
 *   • Large text (scale 3) on all interactive elements
 *   • Simple, uncluttered layout
 *
 * ─────────────────────────────────────────────────────────────────────────── */
#define COLOR_BG          0xFFFF   /* white background — max contrast         */
#define COLOR_TITLE       0x2104   /* dark charcoal title text                */
#define COLOR_BTN_REG     0x0566   /* deep teal-green  (REGISTER)             */
#define COLOR_BTN_DIS     0xC240   /* warm amber-orange (DISPENSE)            */
#define COLOR_BTN_READY   0x0566   /* same teal as register (READY)           */
#define COLOR_BTN_BORDER  0xFFFF   /* white highlight border on buttons       */
#define COLOR_BTN_SHADOW  0x4208   /* dark shadow border on buttons           */
#define COLOR_WHITE       0xFFFF
#define COLOR_BLACK       0x0000
#define COLOR_DLG_BG      0xFFFF   /* white dialog interior                   */
#define COLOR_DLG_BORDER  0x2104   /* dark border — high contrast             */
#define COLOR_DLG_TEXT    0x2104   /* same dark charcoal for dialog text      */
#define COLOR_GRAY        0xC618   /* light grey accents                      */
#define COLOR_BTN_GREEN   0x266E   /* friendly green (I TOOK IT confirmation) */
#define COLOR_BTN_PROGRESS 0x0566  /* same teal as REGISTER, dispensing bar fill */

/* ── Layout (800 × 480 screen) ───────────────────────────────────────────── */

/* Title strip */
#define TITLE_Y      10
#define TITLE_H      44
#define TITLE_STRIP_COLOR  0x0566  /* same teal as register button */

/* Two action buttons — LARGE touch targets (elderly need at least 60px) */
#define BTN_Y         70
#define BTN_H        180
#define BTN_PAD        10

#define REG_BTN_X     20
#define REG_BTN_W    260
#define REG_BTN_Y    BTN_Y
#define REG_BTN_H    BTN_H

#define DISP_BTN_X   300
#define DISP_BTN_W   260
#define DISP_BTN_Y   BTN_Y
#define DISP_BTN_H   BTN_H

/* READY button (full-width, shown in instruction states) */
#define READY_BTN_X   60
#define READY_BTN_Y   BTN_Y
#define READY_BTN_W   460
#define READY_BTN_H   BTN_H
#define COLOR_READY_BORDER  0xFFFF

/* Mascot bounding box — keep bg clear for anime_ui */
#define MASCOT_BG_X   580
#define MASCOT_BG_Y    55
#define MASCOT_BG_W   220
#define MASCOT_BG_H   220

/* Dialog box at bottom */
#define DLG_BOX_X      6
#define DLG_BOX_Y    265
#define DLG_BOX_W    788
#define DLG_BOX_H    210

/* ── Dispense flow (Session 10) ──────────────────────────────────────────── */

/* STATE_DISPENSING: simple filled-bar countdown */
#define DISPENSE_BAR_X   100u
#define DISPENSE_BAR_Y   280u
#define DISPENSE_BAR_W   600u
#define DISPENSE_BAR_H    70u

/* STATE_CONFIRM_TAKEN: large "I Took It" button fills most of the screen,
 * a small "Skip" button sits in the top-right corner for caretaker use
 * (per SOFTWARE_ARCHITECTURE.md §7 — a smaller button, not a separate
 * full-screen state; see session_10_notes.md for why). */
#define TAKEN_BTN_X      100u
#define TAKEN_BTN_Y      150u
#define TAKEN_BTN_W      600u
#define TAKEN_BTN_H      260u

#define SKIP_BTN_X       660u
#define SKIP_BTN_Y        16u
#define SKIP_BTN_W       124u
#define SKIP_BTN_H        50u

/* ── Hardening screens (Session 12) ──────────────────────────────────────── */

/* Two big centred choice buttons — used by the "Face not recognised" screen
 * (TRY AGAIN / CANCEL), which replaced Session 10's dead end where the only
 * outcome of three failed capture attempts was an unconditional return home.
 * Same centred geometry registration_ui.c's CONFIRM/RETRY screen already
 * uses, hoisted here because state_machine.c now needs to hit-test it too. */
#define CHOICE_BTN_Y     260u
#define CHOICE_BTN_W     260u
#define CHOICE_BTN_H     170u
#define CHOICE_BTN_GAP    40u
#define CHOICE_LEFT_X    ((800u - (2u * CHOICE_BTN_W + CHOICE_BTN_GAP)) / 2u)
#define CHOICE_RIGHT_X   (CHOICE_LEFT_X + CHOICE_BTN_W + CHOICE_BTN_GAP)

/* Single full-width dismiss button — used by the alert screens (gallery
 * full, refill needed, SD card unavailable). */
#define ALERT_BTN_X      170u
#define ALERT_BTN_Y      300u
#define ALERT_BTN_W      460u
#define ALERT_BTN_H      140u

#define COLOR_ALERT      0xF800   /* strong red — alert title strip          */
#define COLOR_WARN       0xC240   /* warm amber — warning title strip        */

/* ── Public API ─────────────────────────────────────────────────────────── */
void gui_draw_init(uint32_t buffer_address, uint16_t width, uint16_t height);
void gui_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void gui_draw_text(uint16_t x, uint16_t y, const char *str, uint16_t color, uint8_t scale);

void gui_draw_home_screen(void);
void gui_draw_dialog_text(const char *text);
void gui_draw_ready_screen(const char *dialog_msg);
void gui_draw_mascot_bg(void);   /* no-op; kept for API compatibility */

/* ── Dispense flow screens (Session 10) ──────────────────────────────────── */

/* Full-screen draw: title, patient name, pill count, and an empty progress
 * bar outline. Call once on entering STATE_DISPENSING. */
void gui_draw_dispensing_screen(const char *patient_name, uint8_t pill_count);
/* Redraws only the progress bar's fill, 0-100. Call repeatedly during the
 * simulated dispense countdown. */
void gui_draw_dispensing_progress(uint16_t percent);

/* Full-screen draw: "TAKE YOUR PILL" + large "I Took It" button + small
 * "Skip" button. Call once on entering STATE_CONFIRM_TAKEN. */
void gui_draw_confirm_taken_screen(void);
/* Full-screen brief acknowledgement (e.g. "Thank You!") shown after "I Took
 * It" is tapped, before returning home. */
void gui_draw_taken_thankyou_screen(void);

/* ── Hardening screens (Session 12) ──────────────────────────────────────── */

/* Full-screen two-choice prompt. `title` goes in the strip at the top (drawn
 * in `accent`), `message` in the body (embedded '\n' starts a new line), and
 * the two labels on the CHOICE_LEFT_X / CHOICE_RIGHT_X buttons the caller
 * hit-tests with the CHOICE_* constants above. */
void gui_draw_two_choice_screen(const char *title, const char *message,
                                uint16_t accent,
                                const char *left_label, const char *right_label);

/* Full-screen single-button alert. Same layout language as above with one
 * ALERT_BTN_* dismiss button carrying `button_label`. */
void gui_draw_alert_screen(const char *title, const char *message,
                           uint16_t accent, const char *button_label);

/* Clean the D-Cache over the drawn framebuffer region so the LTDC (which
 * reads the framebuffer directly, bypassing the CPU cache) actually sees
 * what was just drawn. gui_draw_home_screen()/_ready_screen()/_dialog_text()
 * already call this internally — any OTHER module (e.g. registration_ui.c)
 * that draws directly via gui_draw_rect()/gui_draw_text() must call one of
 * these itself after each visible frame, or the LTDC can show stale/
 * partially-updated pixels. */
void gui_draw_flush(void);
void gui_draw_flush_rows(uint16_t y_start, uint16_t y_end);

#ifdef __cplusplus
}
#endif

#endif /* GUI_DRAW_H */
