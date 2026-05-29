#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "HardwareProfile.h"
#include "fonts.h"

namespace Graphics {

    // ---- Primitivas ----
    void fill_screen(uint16_t color);
    void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void draw_pixel(int16_t x, int16_t y, uint16_t color);
    void draw_hline(int16_t x, int16_t y, int16_t len, uint16_t color);
    void draw_vline(int16_t x, int16_t y, int16_t len, uint16_t color);
    void fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
    void draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);

    // ---- Texto ----
    void draw_char  (int16_t x, int16_t y, char c,
                     uint16_t fg, uint16_t bg, uint8_t scale);
    void draw_string(int16_t x, int16_t y, const char *s,
                     uint16_t fg, uint16_t bg, uint8_t scale);

    // ---- Sprites de Space Shooter ----
    void draw_player(int16_t x, int16_t y, uint8_t dir, bool thrust);
    void draw_player_scaled(int16_t cx, int16_t cy, uint8_t dir,
                            bool thrust, uint8_t scale);

    void draw_enemy(int16_t x, int16_t y, uint8_t type, uint8_t animFrame);
    void draw_enemy_scaled(int16_t cx, int16_t cy, uint8_t type,
                           uint8_t animFrame, uint8_t scale);

    void draw_bullet(int16_t x, int16_t y, bool friendly);

    void draw_explosion(int16_t x, int16_t y, uint8_t frame);
}

#endif
