// Sound.cpp - Versión original que funcionaba con GPIO bit-bang
#include "Sound.h"
#include "HardwareProfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

static int gpio_fd = -1;

// Inicializar GPIO para sonido (bit-bang simple)
void sound_init(void) {
    int fd = open("/dev/gpiochip0", O_RDONLY);
    if(fd < 0) {
        fprintf(stderr, "Sound: No se pudo abrir gpiochip0\n");
        return;
    }
    
    struct gpiohandle_request req;
    memset(&req, 0, sizeof(req));
    req.lineoffsets[0] = PIN_SOUND;
    req.lines = 1;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    strncpy(req.consumer_label, "pacman_sound", 15);
    
    if(ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req) == 0) {
        gpio_fd = req.fd;
        fprintf(stderr, "Sound: GPIO %d inicializado para sonido\n", PIN_SOUND);
    } else {
        fprintf(stderr, "Sound: Error al configurar GPIO %d\n", PIN_SOUND);
    }
    close(fd);
}

// Generar tono con bit-bang usando busy-wait preciso
static void gpio_beep(uint16_t freq_hz, uint16_t duration_ms) {
    if(gpio_fd < 0) {
        if(duration_ms) usleep(duration_ms * 1000);
        return;
    }
    
    if(freq_hz == 0) {
        if(duration_ms) usleep(duration_ms * 1000);
        return;
    }
    
    uint32_t half_period_us = 500000 / freq_hz;  // microsegundos
    uint32_t cycles = (duration_ms * 1000) / (half_period_us * 2);
    if(cycles < 1) cycles = 1;
    
    struct gpiohandle_data data;
    memset(&data, 0, sizeof(data));
    
    for(uint32_t i = 0; i < cycles; i++) {
        data.values[0] = 1;
        ioctl(gpio_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
        delay_us(half_period_us);
        
        data.values[0] = 0;
        ioctl(gpio_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
        delay_us(half_period_us);
    }
}

void sound_beep(uint16_t freq_hz, uint16_t duration_ms) {
    gpio_beep(freq_hz, duration_ms);
}

void sound_start(void) {
    gpio_beep(440, 100);
    delay_ms(40);
    gpio_beep(880, 100);
    delay_ms(40);
    gpio_beep(1320, 200);
}

void sound_eat(void) {
    gpio_beep(1200, 30);
}

void sound_ghost(void) {
    gpio_beep(800, 70);
    delay_ms(20);
    gpio_beep(500, 70);
}

void sound_death(void) {
    gpio_beep(440, 200);
    delay_ms(60);
    gpio_beep(350, 200);
    delay_ms(60);
    gpio_beep(260, 350);
}
