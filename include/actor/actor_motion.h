#ifndef ACTOR_MOTION_H
#define ACTOR_MOTION_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dmath.h>

typedef struct Entity Entity;

// ----------------------------------------------------------------------------
// Units
//   distance      : world units
//   linear speed  : world units / second
//   acceleration  : world units / second^2
//   angles        : degrees (yaw stored on transform.rotation.z)
//   time / dt     : seconds
//
// Per-character tunables (gravity, fall_max_speed, jump_minimum_speed, target
// speeds, acceleration rates, roll/jump timings) live in ActorMotionSettings
// and are loaded from the asset file (e.g. male_muscled.c). The constants
// below are engine-wide invariants, not per-character knobs.
// ----------------------------------------------------------------------------

#define LOCOMOTION_MIN_SPEED 5         // horizontal speed snapped to 0 below this

#define ACTOR_ROTATION_LERP_FACTOR 0.8f
#define ACTOR_ROTATION_SNAP_THRESHOLD 1.0f

#define ACTOR_ROTATION_MODE_LERP 0
#define ACTOR_ROTATION_MODE_SNAP 1

#define ACTOR_ROLL_PHASE_LAUNCH 0
#define ACTOR_ROLL_PHASE_SPIN    1
#define ACTOR_ROLL_PHASE_GRIP   2
#define ACTOR_ROLL_PHASE_DONE   3

#define ACTOR_JUMP_PHASE_CHARGING 0
#define ACTOR_JUMP_PHASE_LAUNCH   1
#define ACTOR_JUMP_PHASE_RISING   2
#define ACTOR_JUMP_PHASE_DONE     3
#define ACTOR_GROUNDING_SNAP_ZONE 10

#define ACTOR_JUMP_HOLD_VELOCITY_SCALE 0.96f
#define ACTOR_JUMP_LAUNCH_VELOCITY_SCALE 0.8f


typedef struct {

    float idle_acceleration_rate;
    float walk_acceleration_rate;
    float run_acceleration_rate;
    float sprint_acceleration_rate;
    float roll_acceleration_grip_rate;

    float stand_roll_target_speed;
    float stand_roll_launch_rate;
    float walk_roll_target_speed;
    float walk_roll_launch_rate;
    float run_roll_target_speed;
    float run_roll_launch_rate;
    float sprint_roll_target_speed;
    float sprint_roll_launch_rate;

    float walk_target_speed;
    float run_target_speed;
    float sprint_target_speed;

    float roll_ground_time;
    float roll_change_grip_time;
    float roll_timer_max;

    float stand_roll_ground_time;
    float stand_roll_change_grip_time;
    float stand_roll_timer_max;

    float aerial_control_rate;
    float jump_force_multiplier;
    float jump_minimum_speed;
    float jump_timer_max;

    float gravity;
    float fall_max_speed;

} ActorMotionSettings;

typedef struct {

    float ground_time;
    float grip_time;
    float timer_max;
    float target_speed;
    float launch_rate;

} RollParams;

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
    uint8_t rotation_mode;

} ActorMotionData;

typedef struct {

    float target_yaw;
    bool roll_triggered;
    bool jump_hold;
    bool jump_triggered;

} MotionCommand;

typedef struct ActorMotion {

    ActorMotionSettings settings;
    ActorMotionData data;

} ActorMotion;

void actor_updateMotion(Entity *entity, MotionCommand *cmd, float dt);

#endif
