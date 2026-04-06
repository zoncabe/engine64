#ifndef PLAYER_H
#define PLAYER_H

#include "../entity/entity.h"

#define PLAYER_COUNT 1

typedef struct {
    float hp;
    float stamina;
} PlayerStats;

typedef struct Player {
    Entity      *entity;
    PlayerStats  stats;
} Player;

Player **player_get(void);
Player  *player_create(const char *model_path, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings, const AnimDef *anim_def);
void     player_destroy(Player *p);

void player_update(uint8_t fb_index);

#endif
