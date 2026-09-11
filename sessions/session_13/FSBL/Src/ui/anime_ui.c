/* anime_ui.c - Buddu animation layer.
 * Every frame comes from Abirami's original drawings. The old generated
 * blink, breathing bob and code-added sparkle sequence has been removed.
 */
#include "ui/anime_ui.h"
#include "ui/gui_draw.h"
#include "ui/ui_assets.h"

static uint32_t s_fb_addr = 0u;
static uint32_t s_fb_width = 800u;
static mascot_state_t s_state = MASCOT_IDLE;
static uint8_t s_frame_idx = 0u;
static uint32_t s_last_tick = 0u;
static uint16_t s_bg_color = THEME_BG;

static const ui_rle_sprite_t *error_frame(uint8_t frame)
{
    switch (frame % 3u) {
        case 1u: return &ui_sprite_buddu_error1;
        case 2u: return &ui_sprite_buddu_error2;
        default: return &ui_sprite_buddu_error0;
    }
}

static void clear_and_draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           const ui_rle_sprite_t *sprite)
{
    if (!s_fb_addr || !sprite) return;
    gui_draw_init(s_fb_addr, (uint16_t)s_fb_width, 480u);
    gui_draw_rect(x, y, w, h, s_bg_color);
    gui_blit_rle_sprite(x, y, sprite);
    gui_draw_flush_rows(y, (uint16_t)(y + h));
}

void anime_ui_init(DMA2D_HandleTypeDef *hdma2d, uint32_t fb_addr,
                   uint32_t fb_width)
{
    (void)hdma2d;
    s_fb_addr = fb_addr;
    s_fb_width = fb_width;
    s_state = MASCOT_IDLE;
    s_frame_idx = 0u;
    s_last_tick = 0u;
}

void anime_ui_set_dest_buffer(uint32_t dest_buffer) { s_fb_addr = dest_buffer; }
void anime_ui_set_bg_color(uint16_t color) { s_bg_color = color; }

void anime_ui_set_state(mascot_state_t state)
{
    if (state != s_state) {
        s_state = state;
        s_frame_idx = 0u;
    }
}

void anime_ui_update(uint32_t tick_ms)
{
    if (s_state != MASCOT_ERROR) return;
    if ((tick_ms - s_last_tick) < BUDDU_FRAME_MS) return;
    s_last_tick = tick_ms;
    clear_and_draw(MASCOT_SAD_X, MASCOT_SAD_Y, MASCOT_SAD_W, MASCOT_SAD_H,
                   error_frame(s_frame_idx++));
}

void anime_ui_dma2d_cplt_cb(void) { }
void anime_ui_state_active(void)
{
    clear_and_draw(MASCOT_BG_X, MASCOT_BG_Y, 190u, 215u,
                   &ui_sprite_buddu_register);
}
void anime_ui_state_success(void)
{
    clear_and_draw(278u, 112u, 245u, 275u, &ui_sprite_buddu_registered0);
}
void anime_ui_state_error(void)
{
    clear_and_draw(MASCOT_SAD_X, MASCOT_SAD_Y, MASCOT_SAD_W, MASCOT_SAD_H,
                   error_frame(s_frame_idx));
}
