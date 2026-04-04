#ifndef ACTOR_ANIMATION_H
#define ACTOR_ANIMATION_H

#include "../entity/entity.h"

#define LAND_ANIM_STARTING_HEIGHT 130

typedef struct {
    const T3DSkeleton *layer[ANIM_SLOT_COUNT];
    float              weight[ANIM_SLOT_COUNT];
    uint8_t            count;
} ActorAnimationBuffer;

void actorAnimation_initArmature(Entity *entity);
void actorAnimation_initStandingLocomotionSet(Entity *entity);
void actorAnimation_initJumpSet(Entity *entity);
void actorAnimation_initRollSet(Entity *entity);

void actorAnimation_set(Entity *entity);

#endif
