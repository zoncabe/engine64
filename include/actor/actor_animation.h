#ifndef ACTOR_ANIMATION_H
#define ACTOR_ANIMATION_H

#include "../entity/entity.h"

#define LAND_ANIM_STARTING_HEIGHT 130

typedef struct {
    const T3DSkeleton *layer[16];
    float              weight[16];
    uint8_t            count;
} ActorAnimationBuffer;

typedef struct {
    Entity                       *entity;
    Actor                        *actor;
    ActorAnimation               *anim;
    const AnimDef                *def;
    const ActorAnimationSettings *settings;
    float                         delta;

    uint8_t  speed_state;
    float    locomotion_param;
    float    locomotion_phase;
    float    turning;
} AnimParamCtx;

void actorAnimation_addLayer(ActorAnimationBuffer *buffer, const T3DSkeleton *skel, float weight);
void actorAnimation_blendLayers(const T3DSkeleton *main, const ActorAnimationBuffer *buffer);
void actorAnimation_initGraph(Entity *entity, const AnimDef *def);
void actorAnimation_setParams(Entity *entity, const AnimDef *def);
void actorAnimation_evaluateGraph(const AnimDef *def, const ActorAnimationSettings *settings, ActorAnimation *anim, float delta);

#endif
