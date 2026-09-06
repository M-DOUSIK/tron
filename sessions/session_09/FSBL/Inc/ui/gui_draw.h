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

/* ── Public API ─────────────────────────────────────────────────────────── */
void gui_draw_init(uint32_t buffer_address, uint16_t width, uint16_t height);
void gui_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void gui_draw_text(uint16_t x, uint16_t y, const char *str, uint16_t color, uint8_t scale);

void gui_draw_home_screen(void);
void gui_draw_dialog_text(const char *text);
void gui_draw_ready_screen(const char *dialog_msg);
void gui_draw_mascot_bg(void);   /* no-op; kept for API compatibility */

#ifdef __cplusplus
}
#endif

#endif /* GUI_DRAW_H */
