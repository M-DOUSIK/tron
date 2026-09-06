/* ui/registration_ui.c — Session 09: patient registration UI flow
 * See registration_ui.h for the module-boundary rationale.
 */
#include "ui/registration_ui.h"
#include "ui/gui_draw.h"
#include "ai_vision.h"

#include <string.h>
#include <stdio.h>

/* ── Session state (one registration attempt at a time) ─────────────────── */
static char    s_name[PATIENT_NAME_MAX];
static uint8_t s_name_len;
static uint8_t s_pill_count;
static int8_t  s_embedding[EMBEDDING_SIZE];

#define PILLCOUNT_MIN 1
#define PILLCOUNT_MAX 10

void registration_ui_reset(void)
{
    memset(s_name, 0, sizeof(s_name));
    s_name_len   = 0;
    s_pill_count = PILLCOUNT_MIN;
}

void registration_ui_set_embedding(const int8_t *embedding)
{
    memcpy(s_embedding, embedding, sizeof(s_embedding));
}

/* ── Shared hit-test helper (same pattern as state_machine.c's check_hit) ── */
static bool hit(uint32_t tx, uint32_t ty, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    return (tx >= x && tx <= (uint32_t)(x + w) && ty >= y && ty <= (uint32_t)(y + h));
}

/* ── Small local button-drawing helper (keyboard/pillcount/confirm all use
 * plain filled rectangles with a border — simpler than gui_draw.c's
 * corner-cut draw_button(), which is private to that file, and adequate for
 * the many small keys this screen needs). ──────────────────────────────── */
static void draw_btn(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      uint16_t body_col, const char *label, uint8_t scale)
{
    gui_draw_rect(x, y, w, h, body_col);
    gui_draw_rect(x, y, w, 3, COLOR_BTN_BORDER);
    gui_draw_rect(x, y, 3, h, COLOR_BTN_BORDER);
    gui_draw_rect(x, (uint16_t)(y + h - 3), w, 3, COLOR_BTN_SHADOW);
    gui_draw_rect((uint16_t)(x + w - 3), y, 3, h, COLOR_BTN_SHADOW);

    if (label && *label)
    {
        uint16_t len = 0;
        for (const char *cp = label; *cp; cp++) len++;
        uint16_t char_w = (uint16_t)(9u * scale);
        uint16_t tx = (uint16_t)(x + (w > len * char_w ? (w - len * char_w) / 2u : 2u));
        uint16_t ty = (uint16_t)(y + (h > 8u * scale ? (h - 8u * scale) / 2u : 2u));
        gui_draw_text(tx, ty, label, COLOR_WHITE, scale);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * KEYBOARD screen
 * ══════════════════════════════════════════════════════════════════════════ */

#define KEY_W   70u
#define KEY_H   56u
#define KEY_GAP  8u

#define KB_ROW1_Y 140u
#define KB_ROW2_Y (KB_ROW1_Y + KEY_H + KEY_GAP)
#define KB_ROW3_Y (KB_ROW2_Y + KEY_H + KEY_GAP)
#define KB_ROW4_Y (KB_ROW3_Y + KEY_H + KEY_GAP)

#define NAME_BOX_X 100u
#define NAME_BOX_Y 60u
#define NAME_BOX_W 600u
#define NAME_BOX_H 56u

static const char *KB_ROW1 = "QWERTYUIOP"; /* 10 keys */
static const char *KB_ROW2 = "ASDFGHJKL";  /* 9 keys */
static const char *KB_ROW3 = "ZXCVBNM";    /* 7 keys */

static uint16_t kb_row_start_x(int n_keys)
{
    uint16_t total = (uint16_t)(n_keys * KEY_W + (n_keys - 1) * KEY_GAP);
    return (uint16_t)((800u - total) / 2u);
}

static void draw_name_box(void)
{
    gui_draw_rect(NAME_BOX_X, NAME_BOX_Y, NAME_BOX_W, NAME_BOX_H, COLOR_WHITE);
    gui_draw_rect(NAME_BOX_X, NAME_BOX_Y, NAME_BOX_W, 3, COLOR_DLG_BORDER);
    gui_draw_rect(NAME_BOX_X, NAME_BOX_Y, 3, NAME_BOX_H, COLOR_DLG_BORDER);
    gui_draw_rect(NAME_BOX_X, (uint16_t)(NAME_BOX_Y + NAME_BOX_H - 3), NAME_BOX_W, 3, COLOR_DLG_BORDER);
    gui_draw_rect((uint16_t)(NAME_BOX_X + NAME_BOX_W - 3), NAME_BOX_Y, 3, NAME_BOX_H, COLOR_DLG_BORDER);

    char display[PATIENT_NAME_MAX + 2];
    snprintf(display, sizeof(display), "%s_", s_name);
    gui_draw_text((uint16_t)(NAME_BOX_X + 14), (uint16_t)(NAME_BOX_Y + 16), display, COLOR_DLG_TEXT, 3);
}

void registration_ui_draw_keyboard(void)
{
    gui_draw_rect(0, 0, 800, 480, COLOR_BG);
    gui_draw_text(60, 16, "REGISTER PATIENT - ENTER NAME", COLOR_TITLE, 2);

    draw_name_box();

    /* Row 1: QWERTYUIOP */
    {
        uint16_t x = kb_row_start_x(10);
        char label[2] = {0, 0};
        for (int i = 0; i < 10; i++)
        {
            label[0] = KB_ROW1[i];
            draw_btn((uint16_t)(x + i * (KEY_W + KEY_GAP)), KB_ROW1_Y, KEY_W, KEY_H,
                     COLOR_BTN_REG, label, 3);
        }
    }
    /* Row 2: ASDFGHJKL */
    {
        uint16_t x = kb_row_start_x(9);
        char label[2] = {0, 0};
        for (int i = 0; i < 9; i++)
        {
            label[0] = KB_ROW2[i];
            draw_btn((uint16_t)(x + i * (KEY_W + KEY_GAP)), KB_ROW2_Y, KEY_W, KEY_H,
                     COLOR_BTN_REG, label, 3);
        }
    }
    /* Row 3: ZXCVBNM */
    {
        uint16_t x = kb_row_start_x(7);
        char label[2] = {0, 0};
        for (int i = 0; i < 7; i++)
        {
            label[0] = KB_ROW3[i];
            draw_btn((uint16_t)(x + i * (KEY_W + KEY_GAP)), KB_ROW3_Y, KEY_W, KEY_H,
                     COLOR_BTN_REG, label, 3);
        }
    }
    /* Row 4: SPACE, BACKSPACE, DONE */
    {
        uint16_t x = kb_row_start_x(7); /* reuse row-3 span for centering */
        draw_btn(x, KB_ROW4_Y, 260, KEY_H, COLOR_BTN_REG, "SPACE", 2);
        draw_btn((uint16_t)(x + 260 + KEY_GAP), KB_ROW4_Y, 130, KEY_H, COLOR_BTN_DIS, "DEL", 2);
        draw_btn((uint16_t)(x + 260 + KEY_GAP + 130 + KEY_GAP), KB_ROW4_Y, 148, KEY_H, COLOR_BTN_READY, "DONE", 2);
    }
}

bool registration_ui_handle_keyboard_touch(uint32_t tx, uint32_t ty)
{
    /* Row 1 */
    {
        uint16_t x = kb_row_start_x(10);
        for (int i = 0; i < 10; i++)
        {
            if (hit(tx, ty, (uint16_t)(x + i * (KEY_W + KEY_GAP)), KB_ROW1_Y, KEY_W, KEY_H))
            {
                if (s_name_len < PATIENT_NAME_MAX - 1)
                {
                    s_name[s_name_len++] = KB_ROW1[i];
                    s_name[s_name_len]   = '\0';
                    draw_name_box();
                }
                return false;
            }
        }
    }
    /* Row 2 */
    {
        uint16_t x = kb_row_start_x(9);
        for (int i = 0; i < 9; i++)
        {
            if (hit(tx, ty, (uint16_t)(x + i * (KEY_W + KEY_GAP)), KB_ROW2_Y, KEY_W, KEY_H))
            {
                if (s_name_len < PATIENT_NAME_MAX - 1)
                {
                    s_name[s_name_len++] = KB_ROW2[i];
                    s_name[s_name_len]   = '\0';
                    draw_name_box();
                }
                return false;
            }
        }
    }
    /* Row 3 */
    {
        uint16_t x = kb_row_start_x(7);
        for (int i = 0; i < 7; i++)
        {
            if (hit(tx, ty, (uint16_t)(x + i * (KEY_W + KEY_GAP)), KB_ROW3_Y, KEY_W, KEY_H))
            {
                if (s_name_len < PATIENT_NAME_MAX - 1)
                {
                    s_name[s_name_len++] = KB_ROW3[i];
                    s_name[s_name_len]   = '\0';
                    draw_name_box();
                }
                return false;
            }
        }
    }
    /* Row 4: SPACE / DEL / DONE */
    {
        uint16_t x = kb_row_start_x(7);
        if (hit(tx, ty, x, KB_ROW4_Y, 260, KEY_H))
        {
            if (s_name_len < PATIENT_NAME_MAX - 1)
            {
                s_name[s_name_len++] = ' ';
                s_name[s_name_len]   = '\0';
                draw_name_box();
            }
            return false;
        }
        if (hit(tx, ty, (uint16_t)(x + 260 + KEY_GAP), KB_ROW4_Y, 130, KEY_H))
        {
            if (s_name_len > 0)
            {
                s_name[--s_name_len] = '\0';
                draw_name_box();
            }
            return false;
        }
        if (hit(tx, ty, (uint16_t)(x + 260 + KEY_GAP + 130 + KEY_GAP), KB_ROW4_Y, 148, KEY_H))
        {
            /* Trim trailing spaces so an accidental space-only name can't
             * pass as "non-empty". */
            while (s_name_len > 0 && s_name[s_name_len - 1] == ' ')
            {
                s_name[--s_name_len] = '\0';
            }
            return (s_name_len > 0);
        }
    }
    return false;
}

/* ══════════════════════════════════════════════════════════════════════════
 * PILL COUNT screen
 * ══════════════════════════════════════════════════════════════════════════ */

#define PC_MINUS_X  120u
#define PC_PLUS_X   580u
#define PC_BTN_Y    150u
#define PC_BTN_SIZE 120u
#define PC_NEXT_Y   340u
#define PC_NEXT_X   250u
#define PC_NEXT_W   300u
#define PC_NEXT_H   90u

static void draw_pillcount_number(void)
{
    /* Clear just the number area, then redraw. */
    gui_draw_rect(340, 150, 120, 120, COLOR_BG);
    char buf[4];
    snprintf(buf, sizeof(buf), "%u", (unsigned)s_pill_count);
    /* Scale 8: ~72px tall digit, large and legible for elderly users. */
    gui_draw_text((uint16_t)(s_pill_count >= 10 ? 350 : 375), 175, buf, COLOR_TITLE, 8);
}

void registration_ui_draw_pillcount(void)
{
    gui_draw_rect(0, 0, 800, 480, COLOR_BG);
    gui_draw_text(60, 16, "HOW MANY PILLS PER DAY?", COLOR_TITLE, 2);
    gui_draw_text(100, 60, s_name, COLOR_DLG_TEXT, 2);

    draw_btn(PC_MINUS_X, PC_BTN_Y, PC_BTN_SIZE, PC_BTN_SIZE, COLOR_BTN_DIS, "-", 6);
    draw_btn(PC_PLUS_X,  PC_BTN_Y, PC_BTN_SIZE, PC_BTN_SIZE, COLOR_BTN_REG, "+", 6);
    draw_pillcount_number();

    draw_btn(PC_NEXT_X, PC_NEXT_Y, PC_NEXT_W, PC_NEXT_H, COLOR_BTN_READY, "NEXT", 3);
}

bool registration_ui_handle_pillcount_touch(uint32_t tx, uint32_t ty)
{
    if (hit(tx, ty, PC_MINUS_X, PC_BTN_Y, PC_BTN_SIZE, PC_BTN_SIZE))
    {
        if (s_pill_count > PILLCOUNT_MIN)
        {
            s_pill_count--;
            draw_pillcount_number();
        }
        return false;
    }
    if (hit(tx, ty, PC_PLUS_X, PC_BTN_Y, PC_BTN_SIZE, PC_BTN_SIZE))
    {
        if (s_pill_count < PILLCOUNT_MAX)
        {
            s_pill_count++;
            draw_pillcount_number();
        }
        return false;
    }
    if (hit(tx, ty, PC_NEXT_X, PC_NEXT_Y, PC_NEXT_W, PC_NEXT_H))
    {
        return true;
    }
    return false;
}

/* ══════════════════════════════════════════════════════════════════════════
 * CONFIRM screen
 * ══════════════════════════════════════════════════════════════════════════ */

/* Reuse the home screen's two-button geometry (gui_draw.h) for a familiar,
 * consistently-sized pair of large touch targets. */
void registration_ui_draw_confirm(void)
{
    gui_draw_rect(0, 0, 800, 480, COLOR_BG);
    gui_draw_text(60, 16, "CONFIRM REGISTRATION", COLOR_TITLE, 2);

    char line[64];
    gui_draw_text(80, 90, "Name:", COLOR_DLG_TEXT, 3);
    snprintf(line, sizeof(line), "%s", s_name);
    gui_draw_text(260, 90, line, COLOR_DLG_TEXT, 3);

    gui_draw_text(80, 140, "Pills/day:", COLOR_DLG_TEXT, 3);
    snprintf(line, sizeof(line), "%u", (unsigned)s_pill_count);
    gui_draw_text(340, 140, line, COLOR_DLG_TEXT, 3);

    draw_btn(REG_BTN_X, REG_BTN_Y, REG_BTN_W, REG_BTN_H, COLOR_BTN_REG, "CONFIRM", 3);
    draw_btn(DISP_BTN_X, DISP_BTN_Y, DISP_BTN_W, DISP_BTN_H, COLOR_BTN_DIS, "RETRY", 3);
}

reg_confirm_result_t registration_ui_handle_confirm_touch(uint32_t tx, uint32_t ty)
{
    if (hit(tx, ty, REG_BTN_X, REG_BTN_Y, REG_BTN_W, REG_BTN_H))
    {
        int slot = gallery_add_patient(s_name, s_embedding, (int)s_pill_count);
        if (slot < 0)
        {
            printf("registration_ui: gallery full, could not save '%s'.\r\n", s_name);
            return REG_CONFIRM_FULL;
        }
        printf("registration_ui: patient '%s' saved to slot %d.\r\n", s_name, slot);
        return REG_CONFIRM_SAVED;
    }
    if (hit(tx, ty, DISP_BTN_X, DISP_BTN_Y, DISP_BTN_W, DISP_BTN_H))
    {
        return REG_CONFIRM_RETRY;
    }
    return REG_CONFIRM_NONE;
}
