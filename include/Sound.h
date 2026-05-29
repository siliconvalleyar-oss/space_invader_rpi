#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

void sound_init(void);
void sound_beep(uint16_t freq_hz, uint16_t duration_ms);
void sound_start(void);
void sound_eat(void);
void sound_ghost(void);
void sound_death(void);

#endif
