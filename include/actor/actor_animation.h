#ifndef ACTOR_ANIMATION_H
#define ACTOR_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>


typedef struct Entity Entity;
typedef struct Actor Actor;
typedef struct ActorAnimation ActorAnimation;

#define LAND_ANIM_STARTING_HEIGHT 130
#define ANIM_MAX_LAYERS 16

// --- Anim graph types ---

#define ANIM_SLOT_MAIN          0xFF
#define ANIM_TURN_AVG_COUNT  5

typedef enum {

    ANIM_PARAM_WALK,
    ANIM_PARAM_RUN,
    ANIM_PARAM_SPRINT,
    ANIM_PARAM_IDLE_RIGHT,
    ANIM_PARAM_TURNING,
    ANIM_PARAM_JUMP_L,
    ANIM_PARAM_JUMP_R,
    ANIM_PARAM_LAND_L,
    ANIM_PARAM_LAND_R,
    ANIM_PARAM_ROLL_RUN,
    ANIM_PARAM_ROLL_STAND,
    ANIM_PARAM_ROLL_DIR,
    ANIM_PARAM_STAND_ROLL_DIR,
    ANIM_PARAM_TURN_WALK,
    ANIM_PARAM_TURN_RUN,
    ANIM_PARAM_COUNT

} AnimParam;

typedef enum {

    ANIM_NODE_CLIP,
    ANIM_NODE_SELECT,
    ANIM_NODE_SEQUENCE,
    ANIM_NODE_BLEND,
    ANIM_NODE_LAYER,

} AnimNodeType;

typedef struct {

    const char *name;
    uint8_t buffer;
    bool looping;
    
} AnimClipDef;

typedef struct {

    AnimNodeType type;
    uint8_t anim;
    uint8_t anim2;
    uint8_t buffer;
    uint8_t param;

} AnimNode;

typedef struct {

    const AnimClipDef *clip;
    const AnimNode *node;

    uint8_t clip_count;
    uint8_t node_count;
    uint8_t buffer_count;
    uint8_t walk_anim;
    uint8_t run_anim;
    uint8_t sprint_anim;
    uint8_t turn_walk_anim;
    uint8_t turn_run_anim;
    uint8_t jump_anim;
    uint8_t fall_anim;
    uint8_t land_anim;
    uint8_t roll_anim;
    uint8_t stand_roll_anim;

} AnimDef;

typedef struct {

    float action_idle_max_blending_ratio;
    float run_to_walk_ratio;
    float walk_to_run_ratio;
    float sprint_to_run_ratio;
    float run_to_sprint_ratio;
    float sprint_to_walk_ratio;
    float walk_to_sprint_ratio;
    float walking_anim_length_half;
    float walking_anim_length;
    float running_anim_length_half;
    float running_anim_length;
    float sprinting_anim_length_half;
    float sprinting_anim_length;

} ActorAnimationLocomotionSettings;

typedef struct {

    float jump_max_blending_ratio;
    float jump_anim_length;
    float jump_anim_crouch;
    float jump_anim_air;
    float jump_footing_speed;
    float land_anim_length;
    float land_anim_ground;
    float land_anim_crouch;
    float land_anim_stand;

} ActorAnimationJumpSettings;

typedef struct {

    float run_to_rolling_anim_ground;
    float run_to_rolling_anim_grip;
    float run_to_rolling_anim_stand;
    float run_to_rolling_anim_length;
    float stand_to_rolling_anim_ground;
    float stand_to_rolling_anim_grip;
    float stand_to_rolling_anim_stand;
    float stand_to_rolling_anim_length;

} ActorAnimationRollSettings;

typedef struct {

    ActorAnimationLocomotionSettings standing_locomotion;
    ActorAnimationJumpSettings       jump;
    ActorAnimationRollSettings       roll;

} ActorAnimationSettings;

typedef struct {

    float delta;

    Entity *entity;
    Actor *actor;

    ActorAnimation *anim;
    const AnimDef *def;
    const ActorAnimationSettings *settings;

    uint8_t speed_state;
    float locomotion_param;
    float locomotion_phase;
    float turning;

} AnimParamCtx;


typedef struct {

    const T3DSkeleton *layer[ANIM_MAX_LAYERS];
    float weight[ANIM_MAX_LAYERS];
    uint8_t count;

} ActorAnimationBuffer;

typedef struct ActorAnimation {

    T3DSkeleton  main;
    T3DSkeleton *buffer;
    T3DAnim     *animation;
    uint8_t     *node_state;
    bool        *node_active;
    float        param[ANIM_PARAM_COUNT];
    uint8_t      prev_speed_state;
    uint8_t      action_state;
    float        turn_avg[ANIM_TURN_AVG_COUNT];
    uint8_t      turn_avg_idx;
    ActorAnimationSettings settings;

} ActorAnimation;


void actorAnimation_addLayer(ActorAnimationBuffer *buffer, const T3DSkeleton *skel, float weight);
void actorAnimation_blendLayers(const T3DSkeleton *main, const ActorAnimationBuffer *buffer);
void actorAnimation_initGraph(Entity *entity, const AnimDef *def);
void actorAnimation_setParams(Entity *entity, const AnimDef *def);
void actorAnimation_evaluateGraph(const AnimDef *def, const ActorAnimationSettings *settings, ActorAnimation *anim, float delta);
void actor_setAnimation(Entity *entity);

#endif
