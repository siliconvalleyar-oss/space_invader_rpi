// ============================================================
//  Graphics.cpp — Raspberry Pi ST7789
//  Space Shooter sprites + primitivas gráficas
// ============================================================

#include "../include/Graphics.h"
#include "../include/HardwareProfile.h"

#include <stdlib.h>

// ============================================================
//  PRIMITIVAS
// ============================================================

void Graphics::fill_screen(uint16_t color) {
    set_window(0, 0, TFT_W-1, TFT_H-1);
    push_color_n(color, (uint32_t)TFT_W * TFT_H);
}

void Graphics::fill_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                          uint16_t color) {
    if(x>=(int16_t)TFT_W || y>=(int16_t)TFT_H || w<=0 || h<=0) return;
    if(x<0){ w+=x; x=0; }
    if(y<0){ h+=y; y=0; }
    if(x+w>(int16_t)TFT_W) w = TFT_W - x;
    if(y+h>(int16_t)TFT_H) h = TFT_H - y;
    set_window(x, y, x+w-1, y+h-1);
    push_color_n(color, (uint32_t)w * h);
}

void Graphics::draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if((uint16_t)x >= TFT_W || (uint16_t)y >= TFT_H) return;
    set_window(x, y, x, y);
    push_color(color);
}

void Graphics::draw_hline(int16_t x, int16_t y, int16_t len, uint16_t c) {
    fill_rect(x, y, len, 1, c);
}
void Graphics::draw_vline(int16_t x, int16_t y, int16_t len, uint16_t c) {
    fill_rect(x, y, 1, len, c);
}

// sqrt entera rápida
static int16_t isqrt(int32_t v) {
    if(v <= 0) return 0;
    int16_t x = (int16_t)(v < 65536L ? (int16_t)v : 256);
    for(int i=0;i<8;i++) { int16_t nx=(int16_t)((x + v/x)>>1); if(nx>=x) break; x=nx; }
    while((int32_t)x*x > v) x--;
    return x;
}

void Graphics::fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    for(int16_t dy=-r; dy<=r; dy++) {
        int16_t dx = isqrt((int32_t)r*r - (int32_t)dy*dy);
        fill_rect(cx-dx, cy+dy, 2*dx+1, 1, color);
    }
}

void Graphics::draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    int16_t x=0, y=r, d=3-2*r;
    while(x<=y) {
        draw_pixel(cx+x,cy+y,color); draw_pixel(cx-x,cy+y,color);
        draw_pixel(cx+x,cy-y,color); draw_pixel(cx-x,cy-y,color);
        draw_pixel(cx+y,cy+x,color); draw_pixel(cx-y,cy+x,color);
        draw_pixel(cx+y,cy-x,color); draw_pixel(cx-y,cy-x,color);
        if(d<0) d+=4*x+6; else { d+=4*(x-y)+10; y--; }
        x++;
    }
}

// ============================================================
//  TEXTO
// ============================================================
void Graphics::draw_char(int16_t x, int16_t y, char c,
                          uint16_t fg, uint16_t bg, uint8_t scale) {
    if(c < 0x20 || c > 0x7E) c = ' ';
    uint8_t idx = c - 0x20;
    for(uint8_t col=0; col<5; col++) {
        uint8_t line = font5x7[idx][col];
        for(uint8_t row=0; row<7; row++) {
            fill_rect(x+col*scale, y+row*scale, scale, scale,
                      (line&(1<<row)) ? fg : bg);
        }
    }
}

void Graphics::draw_string(int16_t x, int16_t y, const char *s,
                             uint16_t fg, uint16_t bg, uint8_t scale) {
    while(*s) { draw_char(x, y, *s++, fg, bg, scale); x += 6*scale; }
}

// ============================================================
//  SPRITE: NAVE DEL JUGADOR
//  Dibuja una nave triangular estilizada de 14x14 a escala
// ============================================================
void Graphics::draw_player(int16_t x, int16_t y, uint8_t dir, bool thrust) {
    draw_player_scaled(x + PLAYER_W/2, y + PLAYER_H/2, dir, thrust, 1);
}

void Graphics::draw_player_scaled(int16_t cx, int16_t cy, uint8_t dir,
                                   bool thrust, uint8_t sc) {
    int16_t hw = (PLAYER_W/2) * sc;
    int16_t hh = (PLAYER_H/2) * sc;

    // Borrar bounding box
    fill_rect(cx - hw - sc, cy - hh - sc, 2*hw + 2*sc + 1, 2*hh + 2*sc + 1, BLACK);

    // ========== CUERPO PRINCIPAL ==========
    // Triángulo superior (cabina/nariz) - escaneo vertical
    for(int16_t row = -hh; row <= hh; row++) {
        // Progresión: más ancho en el centro, más angosto en los extremos
        int16_t t = (row < 0) ? (-row) : row; // distancia desde centro
        int16_t halfW = hw - (t * hw) / (hh + 1);
        if(halfW < 1) halfW = 1;
        fill_rect(cx - halfW, cy + row, 2*halfW + 1, 1, CYAN);
    }

    // Ala izquierda - línea diagonal
    for(int16_t row = -hh/2; row <= hh/2; row++) {
        int16_t wingX = cx - hw - sc/2;
        if(row > 0) wingX -= row/2;
        fill_rect(wingX, cy + row, hw/3, 1, CYAN);
    }
    // Ala derecha
    for(int16_t row = -hh/2; row <= hh/2; row++) {
        int16_t wingX = cx + hw + sc/2 - hw/3;
        if(row > 0) wingX += row/2;
        fill_rect(wingX, cy + row, hw/3, 1, CYAN);
    }

    // ========== COCKPIT ==========
    fill_circle(cx, cy - hh/4, sc + 1, COLOR565(0, 200, 255));

    // ========== LLAMA DE PROPULSIÓN ==========
    if(thrust) {
        int16_t flameLen = hh/2 + (rand() % (hh/3));
        for(int16_t f = 0; f < flameLen; f++) {
            int16_t fw = (flameLen - f) / 2 + 1;
            uint16_t fcolor = (f < flameLen/3) ? YELLOW :
                              (f < flameLen*2/3) ? ORANGE : RED;
            fill_rect(cx - fw, cy + hh + f, 2*fw + 1, 1, fcolor);
        }
    }

    // ========== DETALLES ==========
    // Línea del borde del ala izquierda
    draw_vline(cx - hw, cy - hh/2, hh, COLOR565(0, 100, 200));
    draw_vline(cx + hw, cy - hh/2, hh, COLOR565(0, 100, 200));
}

// Helper: draw_rect (contorno)
static void draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
    Graphics::draw_hline(x, y, w, c);
    Graphics::draw_hline(x, y+h-1, w, c);
    Graphics::draw_vline(x, y, h, c);
    Graphics::draw_vline(x+w-1, y, h, c);
}

// ============================================================
//  SPRITE: ENEMIGO
//  3 tipos: 0=basic (rojo), 1=fast (amarillo), 2=tank (magenta)
// ============================================================
void Graphics::draw_enemy(int16_t x, int16_t y, uint8_t type, uint8_t animFrame) {
    draw_enemy_scaled(x + ENEMY_W/2, y + ENEMY_H/2, type, animFrame, 1);
}

void Graphics::draw_enemy_scaled(int16_t cx, int16_t cy, uint8_t type,
                                  uint8_t animFrame, uint8_t sc) {
    (void)animFrame;
    int16_t hw = (ENEMY_W/2) * sc;
    int16_t hh = (ENEMY_H/2) * sc;

    fill_rect(cx - hw - sc, cy - hh - sc, 2*hw + 2*sc + 1, 2*hh + 2*sc + 1, BLACK);

    if(type == 0) {
        // Tipo BASIC: forma de diamante / ovni clásico - ROJO
        uint16_t bodyColor = RED;
        // Cúpula superior (semicírculo)
        for(int16_t row = -hh; row <= 0; row++) {
            int16_t dx = isqrt((int32_t)hh*hh - (int32_t)row*row);
            fill_rect(cx - dx, cy + row, 2*dx + 1, 1, bodyColor);
        }
        // Base rectangular
        fill_rect(cx - hw, cy, 2*hw + 1, hh/2 + 1, bodyColor);
        // Ventana
        fill_circle(cx, cy - hh/3, sc + 1, YELLOW);
        // Luz parpadeante
        if(animFrame < 4) draw_pixel(cx, cy - hh + 1, WHITE);

    } else if(type == 1) {
        // Tipo FAST: nave pequeña hacia abajo - AMARILLO
        for(int16_t row = -hh; row <= hh; row++) {
            int16_t t = abs(row);
            int16_t halfW = hw - (t * hw * 3) / (2*hh + 2);
            if(halfW < 1) halfW = 1;
            fill_rect(cx - halfW, cy + row, 2*halfW + 1, 1, YELLOW);
        }
        // Ala izquierda
        fill_rect(cx - hw - sc, cy - hh/3, sc + hw/3, hh/2, YELLOW);
        // Ala derecha
        fill_rect(cx + hw - sc, cy - hh/3, sc + hw/3, hh/2, YELLOW);
        // Ojo rojo
        fill_circle(cx, cy + hh/4, sc, RED);

    } else {
        // Tipo TANK: forma cuadrada con detalles - MAGENTA
        fill_rect(cx - hw, cy - hh, 2*hw + 1, 2*hh + 1, MAGENTA);
        // Borde más oscuro
        draw_rect(cx - hw, cy - hh, 2*hw + 1, 2*hh + 1, COLOR565(100, 0, 100));
        // Escudo / blindaje
        fill_rect(cx - hw/2, cy - hh/2, hw + 1, hh + 1, COLOR565(200, 0, 150));
        // Ojos gemelos
        fill_circle(cx - hw/3, cy - hh/3, sc, WHITE);
        fill_circle(cx + hw/3, cy - hh/3, sc, WHITE);
        fill_circle(cx - hw/3, cy - hh/3, sc/2, BLACK);
        fill_circle(cx + hw/3, cy - hh/3, sc/2, BLACK);
    }
}

// ============================================================
//  SPRITE: BALA
//  friendly=true  → láser azul/cian (jugador)
//  friendly=false → proyectil rojo (enemigo)
// ============================================================
void Graphics::draw_bullet(int16_t x, int16_t y, bool friendly) {
    if(friendly) {
        // Láser brillante del jugador
        fill_rect(x, y, BULLET_W, BULLET_H, CYAN);
        fill_rect(x+1, y+1, BULLET_W-2, BULLET_H-2, WHITE);
    } else {
        // Proyectil enemigo
        fill_circle(x + BULLET_W/2, y + BULLET_H/2, BULLET_W, RED);
        fill_circle(x + BULLET_W/2, y + BULLET_H/2, BULLET_W-1, YELLOW);
    }
}

// ============================================================
//  SPRITE: EXPLOSIÓN
//  8 frames de animación (0..7)
//  frame 0 = pequeño brillo blanco
//  frame 7 = último destello antes de desaparecer
// ============================================================
void Graphics::draw_explosion(int16_t x, int16_t y, uint8_t frame) {
    static const uint16_t expColors[8] = {
        WHITE, COLOR565(255,255,200), YELLOW, ORANGE,
        ORANGE, RED, COLOR565(150,0,0), COLOR565(80,0,0)
    };

    int16_t r = 1 + frame * 2;  // radio expandiéndose

    fill_circle(x, y, r, expColors[frame]);

    // Partículas aleatorias en frames medios
    if(frame >= 2 && frame <= 5) {
        for(int p = 0; p < 3; p++) {
            int16_t px = x + (rand() % (2*r)) - r;
            int16_t py = y + (rand() % (2*r)) - r;
            draw_pixel(px, py, YELLOW);
        }
    }
}
