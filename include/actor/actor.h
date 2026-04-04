#ifndef ACTOR_H
#define ACTOR_H

#include <stdbool.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "../physics/physics.h"
#include "../render/render_mesh.h"


// --- Animation slots ---

typedef enum {
    ANIM_SLOT_WALK,
    ANIM_SLOT_IDLE_R,
    ANIM_SLOT_JUMP_L,
    ANIM_SLOT_JUMP_R,
    ANIM_SLOT_LAND_L,
    ANIM_SLOT_LAND_R,
    ANIM_SLOT_ROLL_L,
    ANIM_SLOT_ROLL_R,
    ANIM_SLOT_TURN_WALK,
    ANIM_SLOT_TURN_RUN,
    ANIM_SLOT_COUNT
} AnimSlot;


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


// --- Armature & Animation ---

typedef struct ActorArmature {
    T3DSkeleton main;
    T3DSkeleton buffer[ANIM_SLOT_COUNT];
} ActorArmature;

typedef struct {
    T3DAnim breathing_idle;
    T3DAnim transition_left;
    T3DAnim transition_right;
    T3DAnim standing_idle_left;
    T3DAnim standing_idle_right;
    T3DAnim walking;
    T3DAnim running;
    T3DAnim sprinting;
    T3DAnim walking_turn_left;
    T3DAnim walking_turn_right;
    T3DAnim running_turn_left;
    T3DAnim running_turn_right;
} ActorAnimationStandingLocomotionSet;

typedef struct {
    T3DAnim jump_left;
    T3DAnim jump_right;
    T3DAnim falling_left;
    T3DAnim falling_right;
    T3DAnim land_left;
    T3DAnim land_right;
} ActorAnimationJumpSet;

typedef struct {
    T3DAnim running_to_roll_left;
    T3DAnim running_to_roll_right;
    T3DAnim standing_to_roll_left;
    T3DAnim standing_to_roll_right;
} ActorAnimationRollSet;

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
    uint8_t current;
    uint8_t previous;
    float locomotion_blending_ratio;
    float turning_blending_ratio;
    float jump_blending_ratio;
    float land_blending_ratio;
    float roll_blending_ratio;
    float footing_phase;
    float speed;
} ActorAnimationData;

typedef struct ActorAnimation {
    ActorAnimationStandingLocomotionSet standing_locomotion;
    ActorAnimationJumpSet               jump;
    ActorAnimationRollSet               roll;
    ActorAnimationSettings              settings;
    ActorAnimationData                  data;
} ActorAnimation;


#endif
