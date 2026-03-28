#ifndef ACTOR_ANIMATION_H
#define ACTOR_ANIMATION_H


#define LAND_ANIM_STARTING_HEIGHT 130 // workaround until definitive solution including collision

/*
typedef struct {
    T3DSkeleton** skeleton;
    float* weight;
    u_int8_t count;
} ActorAnimationLayers;
*/

typedef struct {

    const T3DSkeleton* layer[ANIMATION_BUFFER_COUNT];

    float weight[ANIMATION_BUFFER_COUNT];

    u_int8_t count;

}ActorAnimationBuffer;

void actorAnimation_initArmature(Actor* actor);
void actorAnimation_initStandingLocomotionSet(Actor* actor);
void actorAnimation_initJumpSet(Actor* actor);
void actorAnimation_initRollSet(Actor* actor);

void actorAnimation_set(Actor* actor);

#endif