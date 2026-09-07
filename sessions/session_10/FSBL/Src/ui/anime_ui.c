/* anime_ui.c — Lumio cat mascot procedural renderer
 *
 * Redrawn to match the reference cat image (Session 07):
 *   • Warm grey fur, thick black outline
 *   • Flat-top head with small rounded triangle ears at top corners
 *   • Large black anime eyes with two white glint dots
 *   • Pink oval blush marks, ω-shaped mouth, 3 diagonal whiskers per side
 *   • Round body with large oval arms + white paw tips
 *   • Two short stumpy feet
 *   • Red ring pendant, crescent tail (right side)
 *   • Breathing bob (frames 0-3 shift up 4 px) + blink (frames 6-7)
 *
 * Drawing order (back → front):
 *   tail → body → arms → feet → pendant → ears → head → face features
 */

#include "ui/anime_ui.h"
#include "stm32n6xx_hal.h"
#include <math.h>

/* ── Module state ───────────────────────────────────────────────────────── */
static DMA2D_HandleTypeDef *s_hdma2d    = NULL;
static uint32_t             s_fb_addr   = 0;
static uint32_t             s_fb_width  = 800;
static mascot_state_t       s_state     = MASCOT_IDLE;
static uint8_t              s_frame_idx = 0;
static uint32_t             s_last_tick = 0;
static volatile uint8_t     s_busy      = 0;
static uint16_t             s_bg_color  = 0xFFFF;

/* ═══════════════════════════════════════════════════════════════════════════
 * LOW-LEVEL DRAWING HELPERS
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Fill a clipped rectangle in the framebuffer (screen bounds: 800×480).
 */
static void cat_rect(volatile uint16_t *fb, uint32_t fw,
                      uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      uint16_t color)
{
    for (uint32_t dy = 0; dy < h; dy++) {
        uint32_t row = y + dy;
        if (row >= 480u) break;
        for (uint32_t dx = 0; dx < w; dx++) {
            uint32_t col = x + dx;
            if (col >= 800u) break;
            fb[row * fw + col] = color;
        }
    }
}

/**
 * @brief Draw a filled circle (Bresenham-style, per-row half-width via sqrtf).
 *        cx, cy are absolute screen pixel coordinates.
 */
static void fill_circle(volatile uint16_t *fb, uint32_t fw,
                          uint32_t cx, uint32_t cy, uint32_t r,
                          uint16_t color)
{
    uint32_t r2 = r * r;
    for (uint32_t dy = 0; dy <= r; dy++) {
        uint32_t hw = (uint32_t)(sqrtf((float)(r2 - dy * dy)) + 0.5f);
        if (cx < hw) continue;
        uint32_t x0 = cx - hw;
        uint32_t w  = 2u * hw + 1u;
        if (cy >= dy)
            cat_rect(fb, fw, x0, cy - dy, w, 1u, color);
        if (dy > 0u && (cy + dy) < 480u)
            cat_rect(fb, fw, x0, cy + dy, w, 1u, color);
    }
}

/**
 * @brief Draw a filled axis-aligned ellipse.
 *        rx = horizontal semi-axis, ry = vertical semi-axis.
 */
static void fill_oval(volatile uint16_t *fb, uint32_t fw,
                       uint32_t cx, uint32_t cy,
                       uint32_t rx, uint32_t ry,
                       uint16_t color)
{
    for (uint32_t dy = 0; dy <= ry; dy++) {
        float    t  = (ry > 0u) ? ((float)dy / (float)ry) : 1.0f;
        uint32_t hw = (uint32_t)((float)rx * sqrtf(1.0f - t * t) + 0.5f);
        if (cx < hw) continue;
        uint32_t x0 = cx - hw;
        uint32_t w  = 2u * hw + 1u;
        if (cy >= dy)
            cat_rect(fb, fw, x0, cy - dy, w, 1u, color);
        if (dy > 0u && (cy + dy) < 480u)
            cat_rect(fb, fw, x0, cy + dy, w, 1u, color);
    }
}

/**
 * @brief Draw a filled triangle from a tip point to a horizontal base.
 *        tip_x / tip_y  : apex screen coordinates
 *        base_cx        : x-centre of the base
 *        base_y         : y of the base (must be >= tip_y)
 *        base_hw        : half-width of the base
 */
static void fill_tri(volatile uint16_t *fb, uint32_t fw,
                      uint32_t tip_x, uint32_t tip_y,
                      uint32_t base_cx, uint32_t base_y, uint32_t base_hw,
                      uint16_t color)
{
    if (base_y < tip_y) return;
    uint32_t h = base_y - tip_y;
    for (uint32_t y = tip_y; y <= base_y; y++) {
        if (y >= 480u) break;
        float    p  = (h > 0u) ? ((float)(y - tip_y) / (float)h) : 1.0f;
        uint32_t hw = (uint32_t)((float)base_hw * p + 0.5f);
        int32_t  dx = (int32_t)((float)((int32_t)base_cx - (int32_t)tip_x) * p);
        uint32_t cx = (uint32_t)((int32_t)tip_x + dx);
        if (cx < hw) continue;
        cat_rect(fb, fw, cx - hw, y, 2u * hw + 1u, 1u, color);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CAT RENDERER
 * Drawing area origin: (LUMIO_X_POS, LUMIO_Y_BASE) = (590, 60)
 * Size: LUMIO_DRAW_W × LUMIO_DRAW_H = 128 × 192 px
 * ═══════════════════════════════════════════════════════════════════════════ */
static void draw_lumio_cat_frame(uint8_t frame)
{
    volatile uint16_t *fb = (volatile uint16_t *)s_fb_addr;
    const uint32_t fw = s_fb_width;
    const uint32_t BX = LUMIO_X_POS;   /* 590 */
    const uint32_t BY = LUMIO_Y_BASE;  /* 60  */

    /* ── Colour palette ────────────────────────────────────────────────── */
    const uint16_t FUR      = 0xC638u; /* warm light grey (matches reference) */
    const uint16_t OL       = 0x18C3u; /* thick dark charcoal outline         */
    const uint16_t EYE_W    = 0xFFFFu; /* eye white (sclera + glints)         */
    const uint16_t BLUSH    = 0xFB8Du; /* pink blush ovals                    */
    const uint16_t EAR_PK   = 0xFDB8u; /* light pink inner-ear fill           */
    const uint16_t NOSE_DK  = 0x18C3u; /* nose dark (same as outline)         */
    const uint16_t PAW_W    = 0xEF7Bu; /* paw-tip slightly lighter            */
    const uint16_t PEND_RED = 0xF800u; /* pendant red body                    */
    const uint16_t PEND_W   = 0xFFFFu; /* pendant white ring                  */
    const uint16_t BG       = s_bg_color;

    /* ── Breathing bob ─────────────────────────────────────────────────── */
    /* frames 0-3: entire cat shifts up 4 px; frames 4-5: base position   */
    int32_t bob = (frame < 4u) ? -4 : 0;

    /* Macro helpers — all animated coordinates get +bob applied to Y     */
    #define AX(x)   (BX + (uint32_t)(x))
    #define AY(y)   ((uint32_t)((int32_t)(BY + (uint32_t)(y)) + bob))
    #define FY(y)   (BY + (uint32_t)(y))   /* fixed — no bob */

    /* ── 1. Clear bounding box ─────────────────────────────────────────── */
    cat_rect(fb, fw, BX, BY, LUMIO_DRAW_W, LUMIO_DRAW_H, BG);

    /* ╔══════════════════════════════════════════════════════════╗
     * ║  TAIL  (crescent on right side — drawn behind body)    ║
     * ╚══════════════════════════════════════════════════════════╝
     * Outer oval filled with FUR, inner oval carved with BG.     */
    fill_oval(fb, fw, AX(105), AY(150), 16u, 22u, OL);   /* outline   */
    fill_oval(fb, fw, AX(105), AY(150), 13u, 19u, FUR);  /* fur fill  */
    fill_oval(fb, fw, AX(111), AY(150), 10u, 16u, BG);   /* carve → crescent */

    /* ╔══════════════════════════════════════════════════════════╗
     * ║  BODY  (large round oval, centre of composition)        ║
     * ╚══════════════════════════════════════════════════════════╝ */
    fill_oval(fb, fw, AX(63), AY(153), 46u, 35u, OL);    /* outline   */
    fill_oval(fb, fw, AX(63), AY(153), 43u, 32u, FUR);   /* fur fill  */

    /* ╔══════════════════════════════════════════════════════════╗
     * ║  ARMS  (large oval shapes on each side, over body)      ║
     * ╚══════════════════════════════════════════════════════════╝ */
    /* Left arm */
    fill_oval(fb, fw, AX(17), AY(147), 18u, 26u, OL);
    fill_oval(fb, fw, AX(17), AY(147), 15u, 23u, FUR);
    /* Left paw tip (lighter oval at arm base) */
    fill_oval(fb, fw, AX(20), AY(170), 14u, 10u, OL);
    fill_oval(fb, fw, AX(20), AY(170), 11u,  7u, PAW_W);

    /* Right arm */
    fill_oval(fb, fw, AX(109), AY(147), 18u, 26u, OL);
    fill_oval(fb, fw, AX(109), AY(147), 15u, 23u, FUR);
    /* Right paw tip */
    fill_oval(fb, fw, AX(106), AY(170), 14u, 10u, OL);
    fill_oval(fb, fw, AX(106), AY(170), 11u,  7u, PAW_W);

    /* ╔══════════════════════════════════════════════════════════╗
     * ║  FEET  (two short stumps — fixed, don't bob)            ║
     * ╚══════════════════════════════════════════════════════════╝ */
    /* Left foot */
    fill_oval(fb, fw, AX(43), FY(179), 16u, 11u, OL);
    fill_oval(fb, fw, AX(43), FY(179), 13u,  8u, FUR);
    /* Right foot */
    fill_oval(fb, fw, AX(83), FY(179), 16u, 11u, OL);
    fill_oval(fb, fw, AX(83), FY(179), 13u,  8u, FUR);

    /* ╔══════════════════════════════════════════════════════════╗
     * ║  PENDANT  (red circle with white ring, at chest)        ║
     * ╚══════════════════════════════════════════════════════════╝ */
    cat_rect(fb, fw, AX(62), AY(129), 2u, 8u, OL);        /* cord        */
    fill_circle(fb, fw, AX(63), AY(141), 8u, OL);         /* outline ring */
    fill_circle(fb, fw, AX(63), AY(141), 6u, PEND_RED);   /* red body     */
    fill_circle(fb, fw, AX(63), AY(141), 3u, PEND_W);     /* white centre */

    /* ╔══════════════════════════════════════════════════════════╗
     * ║  EARS  (drawn before head — head covers their base)     ║
     * ╚══════════════════════════════════════════════════════════╝
     * Each ear: outer dark triangle → fur fill → pink inner fill */
    /* Left ear (tip at AX(28), base leans left) */
    fill_tri(fb, fw, AX(28), AY( 9), AX(22), AY(38), 18u, OL);
    fill_tri(fb, fw, AX(28), AY(12), AX(23), AY(36), 14u, FUR);
    fill_tri(fb, fw, AX(28), AY(15), AX(24), AY(32),  8u, EAR_PK);

    /* Right ear (tip at AX(100)) */
    fill_tri(fb, fw, AX(100), AY( 9), AX(106), AY(38), 18u, OL);
    fill_tri(fb, fw, AX(100), AY(12), AX(105), AY(36), 14u, FUR);
    fill_tri(fb, fw, AX(100), AY(15), AX(104), AY(32),  8u, EAR_PK);

    /* ╔══════════════════════════════════════════════════════════╗
     * ║  HEAD  (flat-top rounded shape, covers ear bases)       ║
     * ╚══════════════════════════════════════════════════════════╝
     * Reference: head is wider than it is tall, flat at top.    */
    /* Top rectangle band (flat part of head) */
    cat_rect(fb, fw, AX(14), AY(28), 100u, 36u, OL);   /* outline top band */
    cat_rect(fb, fw, AX(18), AY(31),  92u, 30u, FUR);  /* fur fill          */
    /* Lower rounded portion */
    fill_oval(fb, fw, AX(64), AY(73), 54u, 46u, OL);   /* outline oval      */
    fill_oval(fb, fw, AX(64), AY(73), 51u, 43u, FUR);  /* fur fill          */
    /* Re-fill flat top area so it stays flat (override oval top curvature) */
    cat_rect(fb, fw, AX(18), AY(31),  92u, 14u, FUR);

    /* ╔══════════════════════════════════════════════════════════╗
     * ║  FACE FEATURES                                           ║
     * ╚══════════════════════════════════════════════════════════╝ */

    /* ── Blush ovals (pink, on cheeks, before eyes) ── */
    fill_oval(fb, fw, AX(20), AY(86), 14u, 8u, BLUSH);
    fill_oval(fb, fw, AX(108), AY(86), 14u, 8u, BLUSH);

    /* ── Eyes ── */
    /* Reference: large solid black circle, two white glint dots (top area) */
    if (frame <= 5u)  /* eyes open */
    {
        /* Left eye */
        fill_circle(fb, fw, AX(42), AY(72), 14u, OL);       /* outline ring  */
        fill_circle(fb, fw, AX(42), AY(72), 12u, OL);       /* black fill    */
        /* Two white glints: large oval top-left + small dot top-right */
        fill_oval  (fb, fw, AX(37), AY(66), 6u, 5u, EYE_W); /* main glint    */
        cat_rect   (fb, fw, AX(47), AY(67), 4u, 4u, EYE_W); /* small glint   */

        /* Right eye */
        fill_circle(fb, fw, AX(86), AY(72), 14u, OL);
        fill_circle(fb, fw, AX(86), AY(72), 12u, OL);
        fill_oval  (fb, fw, AX(81), AY(66), 6u, 5u, EYE_W);
        cat_rect   (fb, fw, AX(91), AY(67), 4u, 4u, EYE_W);
    }
    else if (frame == 6u)  /* half blink */
    {
        fill_circle(fb, fw, AX(42), AY(72), 14u, OL);
        fill_circle(fb, fw, AX(42), AY(72), 12u, OL);
        fill_oval  (fb, fw, AX(37), AY(66), 6u, 5u, EYE_W);
        /* Mask lower half */
        cat_rect(fb, fw, AX(28), AY(73), 28u, 14u, FUR);
        cat_rect(fb, fw, AX(29), AY(73), 26u,  4u, OL);
        /* Right */
        fill_circle(fb, fw, AX(86), AY(72), 14u, OL);
        fill_circle(fb, fw, AX(86), AY(72), 12u, OL);
        fill_oval  (fb, fw, AX(81), AY(66), 6u, 5u, EYE_W);
        cat_rect(fb, fw, AX(72), AY(73), 28u, 14u, FUR);
        cat_rect(fb, fw, AX(73), AY(73), 26u,  4u, OL);
    }
    else  /* frame 7 — eyes fully closed */
    {
        cat_rect(fb, fw, AX(29), AY(72), 26u, 5u, OL);  /* left closed line  */
        cat_rect(fb, fw, AX(73), AY(72), 26u, 5u, OL);  /* right closed line */
    }

    /* ── Nose (small dark oval, centre of face) ── */
    fill_oval(fb, fw, AX(64), AY(90), 5u, 4u, NOSE_DK);

    /* ── Mouth (ω shape — two downward arcs, r=6 each) ──
     * Left arc centred at AX(57), right arc at AX(71), gap=14 px between
     * Drawing only the ring border (left edge + right edge per row).      */
    for (uint32_t dy = 1u; dy <= 6u; dy++) {
        float    fdy = (float)dy;
        uint32_t hw  = (uint32_t)(sqrtf(36.0f - fdy * fdy) + 0.5f);
        uint32_t yr  = AY(97u) + dy;
        if (yr >= 480u) break;
        /* Left arc edges */
        if (AX(57u) >= hw)
            cat_rect(fb, fw, AX(57u) - hw, yr, 2u, 1u, OL);
        cat_rect(fb, fw, AX(57u) + hw - 1u, yr, 2u, 1u, OL);
        /* Right arc edges */
        if (AX(71u) >= hw)
            cat_rect(fb, fw, AX(71u) - hw, yr, 2u, 1u, OL);
        cat_rect(fb, fw, AX(71u) + hw - 1u, yr, 2u, 1u, OL);
    }
    /* Close bottoms */
    cat_rect(fb, fw, AX(51u), AY(103u), 12u, 2u, OL); /* left arc bottom   */
    cat_rect(fb, fw, AX(65u), AY(103u), 12u, 2u, OL); /* right arc bottom  */

    /* ── Whiskers (3 per side, thin diagonal lines) ──
     * Reference: whiskers angle slightly upward from the cheek.           */
    /* Left whiskers  — angle up-left from the blush area */
    cat_rect(fb, fw, BX,       AY(79u), 26u, 2u, OL); /* top    (flat)     */
    cat_rect(fb, fw, BX,       AY(84u), 24u, 2u, OL); /* middle            */
    cat_rect(fb, fw, BX + 3u,  AY(90u), 20u, 2u, OL); /* lower (slight up) */
    /* Right whiskers */
    cat_rect(fb, fw, AX(102u), AY(79u), 26u, 2u, OL);
    cat_rect(fb, fw, AX(104u), AY(84u), 24u, 2u, OL);
    cat_rect(fb, fw, AX(105u), AY(90u), 20u, 2u, OL);

    /* ╔══════════════════════════════════════════════════════════╗
     * ║  CACHE FLUSH — row-by-row, ONLY the 128px cat strip    ║
     * ║                                                          ║
     * ║  Old approach:  192 rows × 1600 bytes = 307 KB flush    ║
     * ║  New approach:  192 rows ×  256 bytes =  49 KB flush    ║
     * ║                                                          ║
     * ║  This eliminates the flicker caused by LTDC reading     ║
     * ║  the UI area (x=0-589) while it was being flushed.     ║
     * ╚══════════════════════════════════════════════════════════╝ */
    for (uint32_t row = 0u; row < LUMIO_DRAW_H; row++) {
        uint32_t row_addr = s_fb_addr
                          + (BY + row) * fw * 2u   /* row start in framebuffer  */
                          + BX         * 2u;        /* skip to cat x-start       */
        /* Align down to 32-byte cache line boundary */
        uint32_t aligned  = row_addr & ~0x1Fu;
        uint32_t extra    = row_addr - aligned;
        SCB_CleanDCache_by_Addr((uint32_t *)aligned,
                                 (int32_t)(LUMIO_DRAW_W * 2u + extra));
    }

    #undef AX
    #undef AY
    #undef FY
}


/* ═══════════════════════════════════════════════════════════════════════════
 * PUBLIC API
 * ═══════════════════════════════════════════════════════════════════════════ */

void anime_ui_init(DMA2D_HandleTypeDef *hdma2d,
                   uint32_t             fb_addr,
                   uint32_t             fb_width)
{
    s_hdma2d    = hdma2d;
    s_fb_addr   = fb_addr;
    s_fb_width  = fb_width;
    s_state     = MASCOT_IDLE;
    s_frame_idx = 0u;
    s_last_tick = 0u;
    s_busy      = 0u;
}

void anime_ui_set_dest_buffer(uint32_t dest_buffer) { s_fb_addr  = dest_buffer; }
void anime_ui_set_bg_color(uint16_t color)           { s_bg_color = color;       }

void anime_ui_set_state(mascot_state_t state)
{
    if (state != s_state) { s_state = state; s_frame_idx = 0u; }
}

void anime_ui_update(uint32_t tick_ms)
{
    if ((tick_ms - s_last_tick) < LUMIO_FRAME_MS) return;
    s_last_tick = tick_ms;
    if (s_busy) return;
    s_busy = 1u;

    switch (s_state) {
        case MASCOT_IDLE:
            draw_lumio_cat_frame(s_frame_idx);
            s_frame_idx = (uint8_t)((s_frame_idx + 1u) % LUMIO_IDLE_FRAMES);
            break;
        case MASCOT_ACTIVE:   anime_ui_state_active();  break;
        case MASCOT_SUCCESS:  anime_ui_state_success(); break;
        case MASCOT_ERROR:    anime_ui_state_error();   break;
        default: break;
    }

    s_busy = 0u;
}

void anime_ui_dma2d_cplt_cb(void) { s_busy = 0u; }

/* ── Stubbed state renderers ──────────────────────────────────────────────── */
void anime_ui_state_active(void)
{
    draw_lumio_cat_frame(s_frame_idx);
    s_frame_idx = (uint8_t)((s_frame_idx + 1u) % LUMIO_IDLE_FRAMES);
}
void anime_ui_state_success(void) { draw_lumio_cat_frame(4u); }
void anime_ui_state_error(void)   { draw_lumio_cat_frame(7u); }
