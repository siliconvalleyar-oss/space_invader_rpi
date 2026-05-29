#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

// ============================================================
//  HardwareProfile.h — Raspberry Pi Zero 2W + GMT130 v1.0
//                       Pantalla ST7789 240x240
//
//  Driver: usa /dev/spidev0.0  (kernel SPI driver)
//          usa /dev/gpiochip0  (kernel GPIO via ioctl)
//  NO requiere bare-metal ni bcm2835 lib.
//  Compilar: g++ -O2 -o pacman *.cpp -lpthread
//  Ejecutar: sudo ./pacman
//
//  Conexión física GMT130 v1.0 → RPi Zero 2W (header de 40 pines):
//
//    GMT130   BCM GPIO   Nº Pin físico
//    ------   --------   -------------
//    SCK      GPIO 11    Pin 23  (SPI0 SCLK)
//    SDATA    GPIO 10    Pin 19  (SPI0 MOSI)
//    DC       GPIO 25    Pin 22
//    RST      GPIO 24    Pin 18
//    BL       GPIO 23    Pin 16
//    VCC      3.3V       Pin 17
//    GND      GND        Pin 20
//
//  CS: la pantalla GMT130 no expone pin CS — está conectado
//      internamente a GND (siempre seleccionado). Por eso
//      usamos spidev con CS_HIGH desactivado y manejamos
//      la transacción manualmente. Si el módulo tiene CS,
//      conectar al Pin 24 (GPIO 8, CE0).
//
//  SPI:
//    Modo 3 (CPOL=1 CPHA=1) — igual que PIC32 (CKP=1 CKE=0)
//    Velocidad: 40 MHz — igual que PIC32 (SPI4BRG=0 @ 80MHz Fpb)
//
//  Sonido: GPIO 12 Pin 32 — buzzer pasivo (bit-bang PWM)
// ============================================================

#include <stdint.h>
#include <stdlib.h>

// ==================== PINES BCM ====================
#define PIN_MOSI   10   // SPI0 MOSI  (pin físico 19)
#define PIN_SCLK   11   // SPI0 SCLK  (pin físico 23)
#define PIN_DC     21//25   // D/C        (pin físico 22)
#define PIN_RST    16//24   // RESET      (pin físico 18)
#define PIN_BL     20//23   // Backlight  (pin físico 16)
#define PIN_SOUND  13//12   // Buzzer     (pin físico 32)

// Botones (opcional)
#define USE_BUTTONS

#ifdef USE_BUTTONS
#define BTN_UP_PIN    27//17//5
#define BTN_DOWN_PIN  27//6
#define BTN_LEFT_PIN  22//13
#define BTN_RIGHT_PIN 17//19
#endif

// ==================== SPI ====================
#define SPI_DEVICE   "/dev/spidev0.0"
#define SPI_SPEED_HZ  40000000    // 40 MHz = mismo que PIC32

// ==================== GPIO ====================
#define GPIO_CHIP    "/dev/gpiochip0"

// ==================== PANTALLA ====================
#define TFT_W       240
#define TFT_H       240

#define MAZE_COLS   9
#define MAZE_ROWS   9
#define MAZE_W      MAZE_COLS
#define MAZE_H      MAZE_ROWS

#define CELL_W      24
#define CELL_H      24
#define MAZE_OX     12
#define MAZE_OY     12

#define WALL_THICK  4
#define PACMAN_R    9
#define PACMAN_SIZE 18
#define GHOST_SIZE  18

#define HUD_Y       (MAZE_OY + MAZE_ROWS*CELL_H + 4)
#define HUD_H       (TFT_H - HUD_Y)

// ==================== COLORES RGB565 ====================
#define BLACK       0x0000
#define WHITE       0xFFFF
#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define CYAN        0x07FF
#define YELLOW      0xFFE0
#define ORANGE      0xFC00
#define MAGENTA     0xF81F
#define GRAY        0x8410
#define DARK_BLUE   0x000C
#define PINK        0xF81F
#define COLOR565(r,g,b) ((uint16_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))

#define WALL_COLOR   COLOR565(0,0,140)
#define WALL_EDGE    COLOR565(30,80,255)
#define FLOOR_COLOR  BLACK
#define DOT_COLOR    COLOR565(255,220,100)
#define POWER_COLOR  WHITE

// ==================== MACROS GPIO ====================
// Implementadas en main.cpp; usan fd global
void gpio_write(int pin, int val);
int  gpio_read (int pin);

#define DC_HIGH()    gpio_write(PIN_DC,  1)
#define DC_LOW()     gpio_write(PIN_DC,  0)
#define RST_HIGH()   gpio_write(PIN_RST, 1)
#define RST_LOW()    gpio_write(PIN_RST, 0)
#define BL_HIGH()    gpio_write(PIN_BL,  1)
#define BL_LOW()     gpio_write(PIN_BL,  0)
// SOUND_ON/OFF: el sonido usa PWM hardware via /sys/class/pwm.
// Sound.cpp gestiona el PWM directamente — estas macros ya no se usan
// pero se mantienen como no-op para que el codigo compile sin cambios.
#define SOUND_ON()   ((void)0)
#define SOUND_OFF()  ((void)0)

#ifdef USE_BUTTONS
#define BTN_UP    (!gpio_read(BTN_UP_PIN))
#define BTN_DOWN  (!gpio_read(BTN_DOWN_PIN))
#define BTN_LEFT  (!gpio_read(BTN_LEFT_PIN))
#define BTN_RIGHT (!gpio_read(BTN_RIGHT_PIN))
#endif

// ==================== PROTOTIPOS ====================
int  hw_init(void);
void hw_close(void);

void delay_ms(uint32_t ms);
void delay_us(uint32_t us);

void spi_write_byte(uint8_t d);
void spi_write_buf (const uint8_t *buf, uint32_t len);

void write_cmd  (uint8_t c);
void write_data (uint8_t d);
void push_color (uint16_t color);
void push_color_n(uint16_t color, uint32_t n);

void reset_display(void);
void init_display (void);
void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

#endif
