#ifndef STAMINA_WHEEL_H
#define STAMINA_WHEEL_H

#include <stdint.h>

typedef struct Player Player;

void stamina_wheel_init(void);

/* On leaving gameplay: drops every slot's character reference before the
   unload frees the characters, so a re-entry cannot read a dead body. */
void stamina_wheel_reset(void);

float stamina_wheel_getProgress(void);
void stamina_wheel_update(const Player *player, uint8_t fb_index);
void stamina_wheel_draw(void);

#endif
