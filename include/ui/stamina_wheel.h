#ifndef STAMINA_WHEEL_H
#define STAMINA_WHEEL_H

#include <stdint.h>

typedef struct Player Player;

void stamina_wheel_init(void);
float stamina_wheel_getProgress(void);
void stamina_wheel_update(const Player *player, uint8_t fb_index);
void stamina_wheel_draw(void);

#endif
