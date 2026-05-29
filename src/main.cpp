// ============================================================
//  main.cpp — Space Shooter RPi Zero 2W + GMT130 ST7789 240x240
//
//  Usa /dev/spidev0.0 (ioctl SPI_IOC_MESSAGE)
//  Usa /dev/gpiochip0 (ioctl GPIO_V1_GET_LINEHANDLE_IOCTL)
//  Sin librerías externas, sin bare-metal /dev/mem.
//
//  PRE-REQUISITOS en /boot/config.txt (o /boot/firmware/config.txt):
//    dtparam=spi=on          ← habilitar SPI
//    # NO cargar framebuffer de la pantalla (si está en uso)
//
//  Compilar:
//    g++ -O2 -std=c++11 -o pacman main.cpp Graphics.cpp \
//        GameEngine.cpp Sound.cpp -lpthread
//  Ejecutar:
//    sudo ./pacman
// ============================================================

#include "../include/HardwareProfile.h"
#include "../include/Graphics.h"
#include "../include/GameEngine.h"
#include "../include/Sound.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <linux/gpio.h>

// ============================================================
//  ESTADO GLOBAL
// ============================================================
static int spi_fd   = -1;
static int gpio_fd  = -1;   // fd de /dev/gpiochip0

// Handles de líneas GPIO (una por pin de salida)
// Agrupamos DC, RST, BL en un handle de 3 lineas.
// PIN_SOUND va por PWM hardware, no por GPIO.
static int gpio_out_fd = -1;   // handle para pines de salida
static int gpio_in_fd  = -1;   // handle para pines de entrada (botones)

// Índices dentro del array de offsets del handle de salida
// DC, RST, BL — los 3 pines GPIO de control de la pantalla
// PIN_SOUND ya NO se incluye aqui: el sonido usa PWM hardware
// (Sound.cpp gestiona /sys/class/pwm directamente)
#define IDX_DC     0
#define IDX_RST    1
#define IDX_BL     2
#define N_OUT_PINS 3

static const uint32_t out_pins[N_OUT_PINS] = {
    PIN_DC, PIN_RST, PIN_BL
};

// Cache del estado de los pines de salida
static uint8_t pin_state[N_OUT_PINS] = {0, 1, 0};

// ============================================================
//  GPIO
// ============================================================

// Abrir /dev/gpiochip0 y solicitar líneas de salida
static int gpio_init(void) {
    gpio_fd = open(GPIO_CHIP, O_RDONLY);
    if(gpio_fd < 0) {
        perror("open /dev/gpiochip0");
        return -1;
    }

    // --- Pines de salida (DC, RST, BL) ---
    struct gpiohandle_request req_out;
    memset(&req_out, 0, sizeof(req_out));
    req_out.flags = GPIOHANDLE_REQUEST_OUTPUT;
    req_out.lines = N_OUT_PINS;
    for(int i = 0; i < N_OUT_PINS; i++) {
        req_out.lineoffsets[i]    = out_pins[i];
        req_out.default_values[i] = pin_state[i];
    }
    strncpy(req_out.consumer_label, "pacman_out", 15);

    if(ioctl(gpio_fd, GPIO_GET_LINEHANDLE_IOCTL, &req_out) < 0) {
        perror("GPIO_GET_LINEHANDLE_IOCTL (salidas)");
        close(gpio_fd);
        return -2;
    }
    gpio_out_fd = req_out.fd;

#ifdef USE_BUTTONS
    const uint32_t in_pins[] = {
        BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN
    };
    struct gpiohandle_request req_in;
    memset(&req_in, 0, sizeof(req_in));
    req_in.flags = GPIOHANDLE_REQUEST_INPUT;
    req_in.lines = 4;
    for(int i = 0; i < 4; i++) req_in.lineoffsets[i] = in_pins[i];
    strncpy(req_in.consumer_label, "pacman_btn", 15);
    if(ioctl(gpio_fd, GPIO_GET_LINEHANDLE_IOCTL, &req_in) >= 0)
        gpio_in_fd = req_in.fd;
#endif

    return 0;
}

// Escribir un pin de salida
void gpio_write(int pin, int val) {
    // Encontrar índice
    int idx = -1;
    for(int i = 0; i < N_OUT_PINS; i++)
        if((int)out_pins[i] == pin) { idx = i; break; }
    if(idx < 0) return;

    pin_state[idx] = val ? 1 : 0;

    // gpiohandle_data.values es uint8_t[GPIOHANDLES_MAX=64]
    // Inicializar a 0 completo para evitar basura en bytes no usados
    struct gpiohandle_data data;
    memset(&data, 0, sizeof(data));
    for(int i = 0; i < N_OUT_PINS; i++)
        data.values[i] = pin_state[i];
    ioctl(gpio_out_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
}

// Leer un pin de entrada (botones)
int gpio_read(int pin) {
    (void)pin;
#ifdef USE_BUTTONS
    if(gpio_in_fd < 0) return 0;
    const uint32_t in_pins[] = {
        BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN
    };
    int idx = -1;
    for(int i = 0; i < 4; i++)
        if((int)in_pins[i] == pin) { idx = i; break; }
    if(idx < 0) return 0;
    struct gpiohandle_data data;
    ioctl(gpio_in_fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
    return data.values[idx];
#else
    return 0;
#endif
}

// ============================================================
//  SPI
// ============================================================
static int spi_init_dev(void) {
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if(spi_fd < 0) {
        perror("open /dev/spidev0.0");
        fprintf(stderr, "Asegurate de que dtparam=spi=on en /boot/config.txt\n");
        return -1;
    }

    // Modo 3: CPOL=1 CPHA=1 (igual que PIC32: CKP=1 CKE=0)
    uint8_t mode = SPI_MODE_3;
    if(ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("SPI_IOC_WR_MODE");
        return -2;
    }

    // 8 bits por palabra
    uint8_t bits = 8;
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);

    // Velocidad: 40 MHz (igual que PIC32: SPI4BRG=0 → 40 MHz)
    uint32_t speed = SPI_SPEED_HZ;
    if(ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("SPI_IOC_WR_MAX_SPEED_HZ");
        // Intentar velocidad menor si 40MHz falla (algunos kernels limitan)
        speed = 32000000;
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
        fprintf(stderr, "SPI: usando 32 MHz como fallback\n");
    }

    fprintf(stderr, "SPI: modo 3, %u MHz\n", speed / 1000000);
    return 0;
}

// Enviar un byte
void spi_write_byte(uint8_t d) {
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf        = (unsigned long)&d;
    tr.rx_buf        = 0;
    tr.len           = 1;
    tr.speed_hz      = SPI_SPEED_HZ;
    tr.bits_per_word = 8;
    tr.cs_change     = 0;
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

// Enviar buffer completo en una sola transferencia SPI
void spi_write_buf(const uint8_t *buf, uint32_t len) {
    if(!len) return;

    // El kernel acepta transferencias grandes en un solo ioctl
    // Dividimos en chunks de 4096 bytes por compatibilidad con
    // algunos drivers que tienen ese límite en bufsiz
    const uint32_t CHUNK = 4096;
    uint32_t offset = 0;
    while(offset < len) {
        uint32_t chunk = (len - offset < CHUNK) ? (len - offset) : CHUNK;
        struct spi_ioc_transfer tr;
        memset(&tr, 0, sizeof(tr));
        tr.tx_buf        = (unsigned long)(buf + offset);
        tr.rx_buf        = 0;
        tr.len           = chunk;
        tr.speed_hz      = SPI_SPEED_HZ;
        tr.bits_per_word = 8;
        tr.cs_change     = 0;
        ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
        offset += chunk;
    }
}

// ============================================================
//  DELAY
// ============================================================
void delay_ms(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, nullptr);
}

void delay_us(uint32_t us) {
    struct timespec ts;
    ts.tv_sec  = 0;
    ts.tv_nsec = (long)us * 1000L;
    nanosleep(&ts, nullptr);
}

// ============================================================
//  PROTOCOLO ST7789
// ============================================================
void write_cmd(uint8_t c) {
    DC_LOW();
    spi_write_byte(c);
}

void write_data(uint8_t d) {
    DC_HIGH();
    spi_write_byte(d);
}

void push_color(uint16_t color) {
    DC_HIGH();
    uint8_t buf[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    spi_write_buf(buf, 2);
}

// Enviar N píxeles del mismo color en bloque — mucho más rápido
void push_color_n(uint16_t color, uint32_t n) {
    if(!n) return;
    DC_HIGH();

    // Pre-llenar un buffer de 512 píxeles = 1024 bytes
    const uint32_t BUF_PX = 512;
    uint8_t buf[BUF_PX * 2];
    uint8_t hi = color >> 8, lo = color & 0xFF;
    uint32_t fill = (n < BUF_PX) ? n : BUF_PX;
    for(uint32_t i = 0; i < fill; i++) { buf[2*i] = hi; buf[2*i+1] = lo; }

    uint32_t left = n;
    while(left > 0) {
        uint32_t chunk = (left < BUF_PX) ? left : BUF_PX;
        spi_write_buf(buf, chunk * 2);
        left -= chunk;
    }
}

void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    write_cmd(0x2A);
    write_data(x0 >> 8); write_data(x0 & 0xFF);
    write_data(x1 >> 8); write_data(x1 & 0xFF);
    write_cmd(0x2B);
    write_data(y0 >> 8); write_data(y0 & 0xFF);
    write_data(y1 >> 8); write_data(y1 & 0xFF);
    write_cmd(0x2C);
    DC_HIGH();
}

// ============================================================
//  RESET + INIT DISPLAY ST7789
//  Secuencia idéntica al PIC32 que funcionó.
// ============================================================
void reset_display(void) {
    RST_HIGH(); delay_ms(10);
    RST_LOW();  delay_ms(20);
    RST_HIGH(); delay_ms(150);
}

void init_display(void) {
    reset_display();

    write_cmd(0x11); delay_ms(120);       // Sleep out

    write_cmd(0x36); write_data(0x00);    // MADCTL: sin rotación
    write_cmd(0x3A); write_data(0x05);    // COLMOD: 16bpp RGB565
    write_cmd(0x21);                      // Invert ON  (ST7789 lo necesita)
    write_cmd(0x13);                      // Normal display mode

    write_cmd(0xB2);                      // Porch control
    write_data(0x0C); write_data(0x0C);
    write_data(0x00); write_data(0x33); write_data(0x33);

    write_cmd(0xB7); write_data(0x35);    // Gate control
    write_cmd(0xBB); write_data(0x37);    // VCOM
    write_cmd(0xC0); write_data(0x2C);    // LCM control
    write_cmd(0xC2); write_data(0x01);    // VDV/VRH
    write_cmd(0xC3); write_data(0x12);    // VRH
    write_cmd(0xC4); write_data(0x20);    // VDV
    write_cmd(0xC6); write_data(0x0F);    // Frame rate 60Hz

    write_cmd(0xD0);                      // Power control 1
    write_data(0xA4); write_data(0xA1);

    write_cmd(0xE0);                      // Gamma positivo
    { const uint8_t g[]={0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,
                          0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23};
      for(int i=0;i<14;i++) write_data(g[i]); }

    write_cmd(0xE1);                      // Gamma negativo
    { const uint8_t g[]={0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,
                          0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23};
      for(int i=0;i<14;i++) write_data(g[i]); }

    write_cmd(0x29); delay_ms(120);       // Display ON

    fprintf(stderr, "Display ST7789 inicializado OK\n");
}

// ============================================================
//  LIMPIEZA
// ============================================================
static void cleanup(void) {
    BL_LOW();
    // SOUND: PWM se apaga via sound_beep (enable=0)
    if(gpio_out_fd >= 0) { close(gpio_out_fd); gpio_out_fd = -1; }
    if(gpio_in_fd  >= 0) { close(gpio_in_fd);  gpio_in_fd  = -1; }
    if(gpio_fd     >= 0) { close(gpio_fd);      gpio_fd     = -1; }
    if(spi_fd      >= 0) { close(spi_fd);        spi_fd      = -1; }
    fprintf(stderr, "cleanup OK\n");
}

void hw_close(void) { cleanup(); }

static void sig_handler(int s) { (void)s; cleanup(); _exit(0); }

// ============================================================
//  hw_init
// ============================================================
int hw_init(void) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    // 1) GPIO primero (necesitamos RST y BL antes de SPI)
    if(gpio_init() < 0) return -1;

    // 2) SPI
    if(spi_init_dev() < 0) { cleanup(); return -2; }

    // Estado inicial seguro
    RST_HIGH();
    DC_LOW();
    BL_LOW();
    // Sonido: Sound.cpp::sound_init() gestiona PWM

    return 0;
}

// ============================================================
//  MAIN
// ============================================================
int main(void) {
    fprintf(stderr, "=== SPACE SHOOTER RPi Zero 2W ===\n");

    if(hw_init() < 0) {
        fprintf(stderr, "ERROR: hw_init fallido.\n"
                        "Verificar:\n"
                        "  1) sudo ./pacman\n"
                        "  2) dtparam=spi=on en /boot/config.txt\n"
                        "  3) reiniciar tras editar config.txt\n");
        return 1;
    }

    // Init display
    init_display();

    // Encender backlight DESPUÉS de init (evita pantalla blanca al arrancar)
    delay_ms(50);
    BL_HIGH();
    fprintf(stderr, "Backlight ON\n");

    // Semilla aleatoria
    srand((unsigned)time(nullptr));

    // Sonido
    sound_init();

    // Lanzar juego (pantalla de título incluida)
    GameEngine::game_loop();

    hw_close();
    return 0;
}
