#include "ui/gui_draw.h"
#include "stm32n6xx_hal.h"
#include <stdio.h>

static uint32_t s_gui_buffer = 0;
static uint16_t s_gui_width  = 0;
static uint16_t s_gui_height = 0;

/* ── 8×8 bitmap font (ASCII 32-127) ──────────────────────────────────────── */
static const uint8_t font8x8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 33 '!'
    {0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // 35 '#'
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // 36 '$'
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // 37 '%'
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, // 38 '&'
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // 39 '\''
    {0x0E,0x1C,0x38,0x38,0x38,0x1C,0x0E,0x00}, // 40 '('
    {0x38,0x1C,0x0E,0x0E,0x0E,0x1C,0x38,0x00}, // 41 ')'
    {0x00,0x18,0x7E,0x3C,0x7E,0x18,0x00,0x00}, // 42 '*'
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // 43 '+'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ','
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // 45 '-'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 '.'
    {0x00,0x03,0x06,0x0C,0x18,0x30,0x60,0x00}, // 47 '/'
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 48 '0'
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 49 '1'
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, // 50 '2'
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 51 '3'
    {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00}, // 52 '4'
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 53 '5'
    {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00}, // 54 '6'
    {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00}, // 55 '7'
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 56 '8'
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, // 57 '9'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // 58 ':'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // 59 ';'
    {0x0E,0x1C,0x38,0x70,0x38,0x1C,0x0E,0x00}, // 60 '<'
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // 61 '='
    {0x70,0x38,0x1C,0x0E,0x1C,0x38,0x70,0x00}, // 62 '>'
    {0x3C,0x66,0x06,0x1C,0x18,0x00,0x18,0x00}, // 63 '?'
    {0x3C,0x66,0x6E,0x6E,0x60,0x60,0x3C,0x00}, // 64 '@'
    {0x3C,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 65 'A'
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // 66 'B'
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // 67 'C'
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // 68 'D'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00}, // 69 'E'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00}, // 70 'F'
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00}, // 71 'G'
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 72 'H'
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 73 'I'
    {0x06,0x06,0x06,0x06,0x06,0x66,0x3C,0x00}, // 74 'J'
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // 75 'K'
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // 76 'L'
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 77 'M'
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // 78 'N'
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 79 'O'
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // 80 'P'
    {0x3C,0x66,0x66,0x66,0x66,0x3C,0x0E,0x00}, // 81 'Q'
    {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00}, // 82 'R'
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 83 'S'
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 'T'
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 85 'U'
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 86 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 'W'
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // 88 'X'
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // 89 'Y'
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // 90 'Z'
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // 91 '['
    {0x00,0x60,0x30,0x18,0x0C,0x06,0x03,0x00}, // 92 backslash
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // 93 ']'
    {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00}, // 94 '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00}, // 95 '_'
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // 96 '`'
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, // 97 'a'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // 98 'b'
    {0x00,0x00,0x3C,0x60,0x60,0x60,0x3C,0x00}, // 99 'c'
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // 100 'd'
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // 101 'e'
    {0x1C,0x30,0x7E,0x30,0x30,0x30,0x30,0x00}, // 102 'f'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C}, // 103 'g'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, // 104 'h'
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 105 'i'
    {0x06,0x00,0x0E,0x06,0x06,0x06,0x06,0x3C}, // 106 'j'
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, // 107 'k'
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 108 'l'
    {0x00,0x00,0x7A,0x7F,0x6B,0x6B,0x6B,0x00}, // 109 'm'
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, // 110 'n'
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // 111 'o'
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, // 112 'p'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06}, // 113 'q'
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, // 114 'r'
    {0x00,0x00,0x3C,0x60,0x3C,0x06,0x3C,0x00}, // 115 's'
    {0x30,0x30,0x7E,0x30,0x30,0x30,0x1C,0x00}, // 116 't'
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // 117 'u'
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // 118 'v'
    {0x00,0x00,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 119 'w'
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // 120 'x'
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C}, // 121 'y'
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // 122 'z'
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // 123 '{'
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}, // 124 '|'
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // 125 '}'
    {0x3A,0x6C,0x00,0x00,0x00,0x00,0x00,0x00}, // 126 '~'
    {0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x00}  // 127 DEL
};

/* ── Core primitives ──────────────────────────────────────────────────────── */
void gui_draw_init(uint32_t buffer_address, uint16_t width, uint16_t height)
{
    s_gui_buffer = buffer_address;
    s_gui_width  = width;
    s_gui_height = height;
}

void gui_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (!s_gui_buffer) return;
    if (x >= s_gui_width || y >= s_gui_height) return;
    if (x + w > s_gui_width)  w = s_gui_width  - x;
    if (y + h > s_gui_height) h = s_gui_height - y;

    volatile uint16_t *fb = (volatile uint16_t *)s_gui_buffer;
    for (uint16_t row = 0; row < h; row++)
    {
        uint32_t off = (uint32_t)(y + row) * s_gui_width + x;
        for (uint16_t col = 0; col < w; col++)
            fb[off + col] = color;
    }
}

void gui_draw_text(uint16_t x, uint16_t y,
                   const char *str, uint16_t color, uint8_t scale)
{
    if (!scale) scale = 1;
    uint16_t sx = x;
    while (*str)
    {
        if (*str == '\n')
        {
            y  = (uint16_t)(y + 10u * scale);
            x  = sx;
            str++;
            continue;
        }
        uint8_t ch = (uint8_t)*str;
        if (ch < 32 || ch > 127) ch = '?';
        const uint8_t *g = font8x8[ch - 32];
        for (uint8_t row = 0; row < 8; row++)
        {
            uint8_t rd = g[row];
            for (uint8_t col = 0; col < 8; col++)
                if (rd & (1u << (7 - col)))
                    gui_draw_rect(x + col * scale, y + row * scale, scale, scale, color);
        }
        x = (uint16_t)(x + 9u * scale);
        str++;
    }
}

/* ── Button helper (rounded look using corner-cut technique) ─────────────── */
static void draw_button(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         uint16_t body_col,
                         const char *line1, const char *line2)
{
    uint16_t p = 12;  /* corner radius in pixels */

    /* Body */
    gui_draw_rect(x + p, y,     w - 2*p, h,       body_col);
    gui_draw_rect(x,     y + p, w,       h - 2*p, body_col);

    /* Cut corners with BG colour */
    gui_draw_rect(x,         y,         p, p, COLOR_BG);
    gui_draw_rect(x + w - p, y,         p, p, COLOR_BG);
    gui_draw_rect(x,         y + h - p, p, p, COLOR_BG);
    gui_draw_rect(x + w - p, y + h - p, p, p, COLOR_BG);

    /* Highlight border (top + left — lighter stripe) */
    gui_draw_rect(x + p,     y + 1,     w - 2*p, 4, COLOR_BTN_BORDER);
    gui_draw_rect(x + 1,     y + p,     4, h - 2*p, COLOR_BTN_BORDER);

    /* Shadow border (bottom + right) */
    gui_draw_rect(x + p,     y + h - 5, w - 2*p, 4, COLOR_BTN_SHADOW);
    gui_draw_rect(x + w - 5, y + p,     4, h - 2*p, COLOR_BTN_SHADOW);

    /* Labels — scale 3 for elderly readability */
    uint8_t  sc     = 3;
    uint16_t char_w = (uint16_t)(9u * sc);

    if (line1 && *line1)
    {
        uint16_t len = 0;
        for (const char *cp = line1; *cp; cp++) len++;
        uint16_t tx = x + (w - len * char_w) / 2u;
        uint16_t ty = (line2 && *line2) ? y + h/2 - 30 : y + h/2 - 14;
        gui_draw_text(tx, ty, line1, COLOR_WHITE, sc);
    }
    if (line2 && *line2)
    {
        uint16_t len = 0;
        for (const char *cp = line2; *cp; cp++) len++;
        uint16_t tx = x + (w - len * char_w) / 2u;
        gui_draw_text(tx, y + h/2 + 6, line2, COLOR_WHITE, sc);
    }
}

/* ── Dialog box with bold border ────────────────────────────────────────── */
static void draw_dialog_box(void)
{
    /* White fill */
    gui_draw_rect(DLG_BOX_X, DLG_BOX_Y, DLG_BOX_W, DLG_BOX_H, COLOR_DLG_BG);

    /* 5-pixel outer border */
    uint16_t b = 5;
    gui_draw_rect(DLG_BOX_X,               DLG_BOX_Y,               DLG_BOX_W, b,         COLOR_DLG_BORDER);
    gui_draw_rect(DLG_BOX_X,               DLG_BOX_Y + DLG_BOX_H-b, DLG_BOX_W, b,         COLOR_DLG_BORDER);
    gui_draw_rect(DLG_BOX_X,               DLG_BOX_Y,               b,         DLG_BOX_H, COLOR_DLG_BORDER);
    gui_draw_rect(DLG_BOX_X + DLG_BOX_W-b, DLG_BOX_Y,               b,         DLG_BOX_H, COLOR_DLG_BORDER);

    /* Inner accent rule at top (teal stripe under the border) */
    gui_draw_rect(DLG_BOX_X, DLG_BOX_Y, DLG_BOX_W, (uint16_t)(b + 24), 0x0566u);
}

/* ── Mascot bg stub (kept for API compatibility) ────────────────────────── */
void gui_draw_mascot_bg(void) { /* anime_ui handles its own clear */ }

/* ── Flush helper ─────────────────────────────────────────────────────────── */
static void flush_all(void)
{
    SCB_CleanDCache_by_Addr((uint32_t *)s_gui_buffer,
                             (int32_t)(s_gui_width * s_gui_height * 2));
}
static void flush_rows(uint16_t y_start, uint16_t y_end)
{
    uint32_t start = s_gui_buffer + (uint32_t)(y_start * s_gui_width * 2);
    uint32_t size  = (uint32_t)((y_end - y_start + 1) * s_gui_width * 2);
    SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)size);
}

/* Public wrappers — see gui_draw.h for why other modules need these. */
void gui_draw_flush(void) { flush_all(); }
void gui_draw_flush_rows(uint16_t y_start, uint16_t y_end) { flush_rows(y_start, y_end); }

/* ── HOME SCREEN ─────────────────────────────────────────────────────────── */
void gui_draw_home_screen(void)
{
    /* 1. White background */
    gui_draw_rect(0, 0, s_gui_width, s_gui_height, COLOR_BG);

    /* 2. Teal title strip */
    gui_draw_rect(0, 0, s_gui_width, (uint16_t)(TITLE_Y + TITLE_H + 4), 0x0566u);

    /* 3. Title text (white on teal, scale 3) */
    gui_draw_text(130, TITLE_Y + 6, "SMART PILL DISPENSER", COLOR_WHITE, 3);

    /* 4. Thin separator line under title */
    gui_draw_rect(0, TITLE_Y + TITLE_H + 4, s_gui_width, 3, COLOR_BTN_SHADOW);

    /* 5. REGISTER button (teal-green) */
    draw_button(REG_BTN_X, REG_BTN_Y, REG_BTN_W, REG_BTN_H,
                COLOR_BTN_REG, "REGISTER", "PATIENT");

    /* 6. DISPENSE button (amber-orange) */
    draw_button(DISP_BTN_X, DISP_BTN_Y, DISP_BTN_W, DISP_BTN_H,
                COLOR_BTN_DIS, "DISPENSE", "PILLS");

    /* 7. Dialog box */
    draw_dialog_box();
    gui_draw_text(DLG_BOX_X + 16, DLG_BOX_Y + 40,
                  "Hello! I am Lumio, your helper.", COLOR_DLG_TEXT, 2);
    gui_draw_text(DLG_BOX_X + 16, DLG_BOX_Y + 66,
                  "Tap REGISTER to add a new patient,", COLOR_DLG_TEXT, 2);
    gui_draw_text(DLG_BOX_X + 16, DLG_BOX_Y + 92,
                  "or DISPENSE to collect your pills.", COLOR_DLG_TEXT, 2);

    flush_all();
}

/* ── INSTRUCTION SCREEN (with READY button) ─────────────────────────────── */
void gui_draw_ready_screen(const char *dialog_msg)
{
    /* Clear the button area only (preserve title + dialog) */
    gui_draw_rect(0, BTN_Y - 8, MASCOT_BG_X, BTN_H + 16, COLOR_BG);

    /* Large READY button (teal, centred) */
    draw_button(READY_BTN_X, READY_BTN_Y, READY_BTN_W, READY_BTN_H,
                COLOR_BTN_READY, "I AM READY", "");

    /* Update dialog text */
    gui_draw_rect(DLG_BOX_X + 5, DLG_BOX_Y + 28, DLG_BOX_W - 10, DLG_BOX_H - 33, COLOR_DLG_BG);
    gui_draw_text(DLG_BOX_X + 16, DLG_BOX_Y + 40, dialog_msg, COLOR_DLG_TEXT, 2);

    flush_rows((uint16_t)(BTN_Y - 8),
               (uint16_t)(DLG_BOX_Y + DLG_BOX_H));
}

/* ── DIALOG UPDATE ────────────────────────────────────────────────────────── */
void gui_draw_dialog_text(const char *text)
{
    gui_draw_rect(DLG_BOX_X + 5, DLG_BOX_Y + 28, DLG_BOX_W - 10, DLG_BOX_H - 33, COLOR_DLG_BG);
    gui_draw_text(DLG_BOX_X + 16, DLG_BOX_Y + 40, text, COLOR_DLG_TEXT, 2);
    flush_rows((uint16_t)DLG_BOX_Y, (uint16_t)(DLG_BOX_Y + DLG_BOX_H));
}

/* ── DISPENSE FLOW SCREENS (Session 10) ──────────────────────────────────── */

void gui_draw_dispensing_screen(const char *patient_name, uint8_t pill_count)
{
    gui_draw_rect(0, 0, s_gui_width, s_gui_height, COLOR_BG);
    gui_draw_rect(0, 0, s_gui_width, (uint16_t)(TITLE_Y + TITLE_H + 4), 0x0566u);
    gui_draw_text(116, TITLE_Y + 6, "DISPENSING YOUR PILLS", COLOR_WHITE, 3);
    gui_draw_rect(0, TITLE_Y + TITLE_H + 4, s_gui_width, 3, COLOR_BTN_SHADOW);

    char line[80];
    snprintf(line, sizeof(line), "For: %s", patient_name ? patient_name : "");
    gui_draw_text(100, 150, line, COLOR_TITLE, 3);
    snprintf(line, sizeof(line), "%u pill%s", (unsigned)pill_count, pill_count == 1u ? "" : "s");
    gui_draw_text(100, 200, line, COLOR_TITLE, 3);

    /* Empty bar outline; gui_draw_dispensing_progress() fills it in. */
    gui_draw_rect(DISPENSE_BAR_X, DISPENSE_BAR_Y, DISPENSE_BAR_W, DISPENSE_BAR_H, COLOR_WHITE);
    gui_draw_rect(DISPENSE_BAR_X, DISPENSE_BAR_Y, DISPENSE_BAR_W, 3, COLOR_DLG_BORDER);
    gui_draw_rect(DISPENSE_BAR_X, DISPENSE_BAR_Y, 3, DISPENSE_BAR_H, COLOR_DLG_BORDER);
    gui_draw_rect(DISPENSE_BAR_X, (uint16_t)(DISPENSE_BAR_Y + DISPENSE_BAR_H - 3), DISPENSE_BAR_W, 3, COLOR_DLG_BORDER);
    gui_draw_rect((uint16_t)(DISPENSE_BAR_X + DISPENSE_BAR_W - 3), DISPENSE_BAR_Y, 3, DISPENSE_BAR_H, COLOR_DLG_BORDER);

    flush_all();
}

void gui_draw_dispensing_progress(uint16_t percent)
{
    if (percent > 100u) percent = 100u;

    uint16_t inner_x = (uint16_t)(DISPENSE_BAR_X + 3);
    uint16_t inner_y = (uint16_t)(DISPENSE_BAR_Y + 3);
    uint16_t inner_w = (uint16_t)(DISPENSE_BAR_W - 6);
    uint16_t inner_h = (uint16_t)(DISPENSE_BAR_H - 6);
    uint16_t fill_w  = (uint16_t)((uint32_t)inner_w * percent / 100u);

    gui_draw_rect(inner_x, inner_y, inner_w, inner_h, COLOR_BG);
    gui_draw_rect(inner_x, inner_y, fill_w, inner_h, COLOR_BTN_PROGRESS);

    flush_rows(DISPENSE_BAR_Y, (uint16_t)(DISPENSE_BAR_Y + DISPENSE_BAR_H));
}

void gui_draw_confirm_taken_screen(void)
{
    gui_draw_rect(0, 0, s_gui_width, s_gui_height, COLOR_BG);
    gui_draw_rect(0, 0, s_gui_width, (uint16_t)(TITLE_Y + TITLE_H + 4), 0xC240u);
    gui_draw_text(220, TITLE_Y + 6, "TAKE YOUR PILL NOW", COLOR_WHITE, 3);
    gui_draw_rect(0, TITLE_Y + TITLE_H + 4, s_gui_width, 3, COLOR_BTN_SHADOW);

    draw_button(TAKEN_BTN_X, TAKEN_BTN_Y, TAKEN_BTN_W, TAKEN_BTN_H,
                COLOR_BTN_GREEN, "I TOOK IT!", "");
    draw_button(SKIP_BTN_X, SKIP_BTN_Y, SKIP_BTN_W, SKIP_BTN_H,
                COLOR_GRAY, "SKIP", "");

    flush_all();
}

void gui_draw_taken_thankyou_screen(void)
{
    gui_draw_rect(0, 0, s_gui_width, s_gui_height, COLOR_BG);
    gui_draw_text(260, 200, "Thank You!", COLOR_TITLE, 4);
    flush_all();
}

/* ── HARDENING SCREENS (Session 12) ──────────────────────────────────────── */

/* Both screens below share one layout so the device's error/alert language is
 * consistent: a coloured title strip, a body message in the same large,
 * high-contrast style as every other screen, and big touch targets sized to
 * the same elderly-friendly rules as the home screen's buttons. They are
 * error and alert paths for cases the Session 09/10 flows previously ended in
 * silently or as a dead end — not new features. */

static void draw_alert_frame(const char *title, const char *message, uint16_t accent)
{
    gui_draw_rect(0, 0, s_gui_width, s_gui_height, COLOR_BG);
    gui_draw_rect(0, 0, s_gui_width, (uint16_t)(TITLE_Y + TITLE_H + 4), accent);

    if (title && *title)
    {
        uint16_t len = 0;
        for (const char *cp = title; *cp; cp++) len++;
        /* Centre on the 800px strip: scale-3 glyphs advance 27px each. */
        uint16_t tx = (uint16_t)((s_gui_width > len * 27u)
                                  ? (s_gui_width - len * 27u) / 2u : 0u);
        gui_draw_text(tx, TITLE_Y + 6, title, COLOR_WHITE, 3);
    }
    gui_draw_rect(0, TITLE_Y + TITLE_H + 4, s_gui_width, 3, COLOR_BTN_SHADOW);

    if (message && *message)
    {
        gui_draw_text(60, 110, message, COLOR_DLG_TEXT, 3);
    }
}

void gui_draw_two_choice_screen(const char *title, const char *message,
                                uint16_t accent,
                                const char *left_label, const char *right_label)
{
    draw_alert_frame(title, message, accent);
    draw_button(CHOICE_LEFT_X,  CHOICE_BTN_Y, CHOICE_BTN_W, CHOICE_BTN_H,
                COLOR_BTN_REG, left_label, "");
    draw_button(CHOICE_RIGHT_X, CHOICE_BTN_Y, CHOICE_BTN_W, CHOICE_BTN_H,
                COLOR_BTN_DIS, right_label, "");
    flush_all();
}

void gui_draw_alert_screen(const char *title, const char *message,
                           uint16_t accent, const char *button_label)
{
    draw_alert_frame(title, message, accent);
    draw_button(ALERT_BTN_X, ALERT_BTN_Y, ALERT_BTN_W, ALERT_BTN_H,
                COLOR_BTN_READY, button_label, "");
    flush_all();
}
