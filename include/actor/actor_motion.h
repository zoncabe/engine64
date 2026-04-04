#ifndef ACTOR_MOTION_H
#define ACTOR_MOTION_H

#include "../entity/entity.h"

#define ACTOR_GRAVITY -1800
#define LOCOMOTION_MIN_SPEED 5

#define JUMP_MINIMUM_SPEED 450
#define FALL_MAX_SPEED 1000

#define ACTOR_ROTATION_LERP_FACTOR      0.8f
#define ACTOR_ROTATION_SNAP_THRESHOLD   1.0f
#define ACTOR_GROUNDING_SNAP_ZONE       10
#define ACTOR_JUMP_HOLD_VELOCITY_SCALE  0.96f
#define ACTOR_JUMP_LAUNCH_VELOCITY_SCALE 0.8f

void actorMotion_update(Entity *entity);

#endif
