#ifndef ACTOR_H
#define ACTOR_H

#include <stdbool.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "../physics/physics.h"
#include "../graphics/mesh.h"


// --- Anim graph types ---

#define ANIM_SLOT_MAIN 0xFF

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
    ANIM_PARAM_ROLL,
    ANIM_PARAM_ROLL_DIR,
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
    AnimNodeType type;
    uint8_t      anim;
    uint8_t      anim2;
    uint8_t      buffer;
    uint8_t      param;
} AnimNode;

typedef struct {
    const char *name;
    uint8_t     buffer;
    bool        looping;
} AnimClipDef;

typedef struct {
    const AnimClipDef *clip;
    const AnimNode    *node;
    uint8_t            clip_count;
    uint8_t            node_count;
    uint8_t            buffer_count;
    uint8_t            walk_anim;
    uint8_t            run_anim;
    uint8_t            sprint_anim;
    uint8_t            turn_walk_anim;
    uint8_t            turn_run_anim;
    uint8_t            jump_anim;
    uint8_t            land_anim;
    uint8_t            roll_anim;
} AnimDef;


// --- State ---

typedef struct ActorStateData {
    uint8_t current;
    uint8_t previous;
    uint8_t locomotion;
} ActorStateData;


// --- Motion ---

typedef struct {
    float idle_acceleration_rate;
    float walk_acceleration_rate;
    float run_acceleration_rate;
    float sprint_acceleration_rate;
    float roll_acceleration_grip_rate;

    float walk_target_speed;
    float run_target_speed;
    float sprint_target_speed;

    float roll_change_grip_time;
    float roll_timer_max;

    float aerial_control_rate;
    float jump_acceleration_rate;
    float jump_force_multiplier;
    float jump_minimum_speed;
    float jump_timer_max;

    float gravity;
    float fall_max_speed;
} ActorMotionSettings;

typedef struct {
    float previous_yaw;
    Vector3 target_velocity;
    float horizontal_speed;
    float roll_timer;
    Vector3 jump_initial_velocity;
    float jump_force;
    float jump_timer;
    bool grounded;
    float grounding_height;
} ActorMotionData;

typedef struct {
    float target_yaw;
    bool jump_hold;
    bool jump_triggered;
} ActorMotionInput;

typedef struct ActorMotion {
    ActorMotionSettings settings;
    ActorMotionInput    input;
    ActorMotionData     data;
} ActorMotion;


// --- Animation settings ---

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

typedef struct ActorAnimation {
    T3DSkeleton  main;
    T3DSkeleton *buffer;
    T3DAnim     *animation;
    uint8_t     *node_state;
    float        param[ANIM_PARAM_COUNT];
    uint8_t      prev_speed_state;
    uint8_t      action_state;
    ActorAnimationSettings settings;
} ActorAnimation;


typedef struct Actor {
    RigidBody       body;
    ActorMotion     motion;
    ActorAnimation  animation;
    ActorStateData  state;
    const AnimDef  *anim_def;
} Actor;


#endif
