#ifndef PLAYER_H
#define PLAYER_H

#include "../entity/entity.h"
#include "../control/control.h"

#define PLAYER_COUNT 1

typedef struct {
    float hp;
    float stamina;
} PlayerStats;

typedef struct Player {
    ControllerData control;
    Entity        *entity;
    PlayerStats    stats;
} Player;

Player **player_get(void);

void player_update(uint8_t fb_index);

#endif
