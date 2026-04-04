#ifndef MEMORY_H
#define MEMORY_H

#include "../entity/entity.h"
#include "../player/player.h"

Player *player_create(const char *model_path, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings);

#endif
