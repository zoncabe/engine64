#ifndef ACTOR_STATES_H
#define ACTOR_STATES_H

#include "../entity/entity.h"

#define STANDING_IDLE 0
#define WALKING       1
#define RUNNING       2
#define SPRINTING     3
#define ROLLING       4
#define JUMPING       5
#define FALLING       6

void actor_setState(ActorStateData *state, uint8_t new_state);

#endif
