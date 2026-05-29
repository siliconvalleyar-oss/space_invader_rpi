#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "HardwareProfile.h"
#include "Graphics.h"

// ============================================================
//  Constantes del juego
// ============================================================
#define MAX_BULLETS    8
#define MAX_ENEMIES    15
#define MAX_EXPLOSIONS 8
#define NUM_STARS      40

#define PLAYER_W       14
#define PLAYER_H       14
#define ENEMY_W        12
#define ENEMY_H        12
#define BULLET_W        3
#define BULLET_H        7

namespace GameEngine {

    // ---- Estados del juego ----
    enum GameState {
        STATE_TITLE,
        STATE_PLAYING,
        STATE_GAME_OVER
    };

    // ---- Proyectiles ----
    struct Bullet {
        int16_t x, y;
        bool active;
    };

    // ---- Enemigos ----
    struct Enemy {
        int16_t x, y;
        uint8_t type;      // 0=basic, 1=fast, 2=tank
        uint8_t hp;
        int8_t  dirX;      // movimiento horizontal
        bool    active;
        uint8_t animFrame
    };

    // ---- Explosiones ----
    struct Explosion {
        int16_t x, y;
        uint8_t frame;     // 0..7
        bool active;
    };

    // ---- Estrellas de fondo ----
    struct Star {
        int16_t x, y;
        uint8_t speed;     // 1-3
        uint8_t brightness;// 1-3 (tamaño)
    };

    // ---- Estado global ----
    extern int16_t playerX, playerY;
    extern uint8_t playerDir;        // 0=up,1=down,2=left,3=right
    extern int8_t  playerMoveX, playerMoveY;

    extern Bullet   bullets[MAX_BULLETS];
    extern Enemy    enemies[MAX_ENEMIES];
    extern Explosion explosions[MAX_EXPLOSIONS];
    extern Star     stars[NUM_STARS];

    extern uint16_t score;
    extern uint16_t highScore;
    extern uint8_t  lives;
    extern uint8_t  wave;
    extern bool     gameOver;
    extern GameState gameState;

    extern uint8_t  fireCooldown;
    extern uint8_t  enemySpawnTimer;
    extern uint8_t  enemiesAlive;
    extern uint8_t  enemiesPerWave;
    extern uint16_t waveDelay;

    // ---- API ----
    void init_game(void);
    void init_stars(void);
    void update_stars(void);
    void draw_stars(void);

    void spawn_enemy(void);
    void update_player(void);
    void update_bullets(void);
    void update_enemies(void);
    void update_explosions(void);
    void check_collisions(void);
    void draw_hud(void);

    void game_loop(void);
    void show_title(void);
    void show_game_over(void);
    void show_wave_clear(void);
}

#endif
