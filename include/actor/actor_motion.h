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
//   angles        : degrees (yaw stored on transform.rotation.z)
//   time / dt     : seconds
//
// Per-character tunables live in ActorMotionSettings and are loaded from the
// asset file (e.g. male_muscled.c). The constants below are engine-wide
// invariants, not per-character knobs.
// ----------------------------------------------------------------------------

#define LOCOMOTION_MIN_SPEED 5

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

#define ACTOR_GRAVITY -1800.0f
#define ACTOR_FALL_MAX_SPEED -1500.0f


typedef enum {
	LOCOMOTION_IDLE,
	LOCOMOTION_WALK,
	LOCOMOTION_RUN,
	LOCOMOTION_SPRINT,
	LOCOMOTION_COUNT
} LocomotionType;

typedef struct {
	float target_speed;
	float response_rate;
	float rotation_response_rate;
} LocomotionSettings;

typedef struct {
	float launch_response_rate;
	float spin_response_rate;
	float grip_response_rate;
	float ground_time;
	float grip_time;
	float timer_max;
	float target_speed[LOCOMOTION_COUNT];
} RollSettings;

typedef struct {
	float response_rate;
	float force_multiplier;
	float minimum_speed;
	float timer_max;
} JumpSettings;

typedef struct {
	LocomotionSettings locomotion[LOCOMOTION_COUNT];
	RollSettings       roll;
	JumpSettings       jump;
} ActorMotionSettings;

typedef struct {
	float previous_yaw;
	float horizontal_speed;
	float roll_timer;
	Vector3 jump_initial_velocity;
	float jump_force;
	float jump_timer;
	bool is_grounded;
	float grounding_height;
	uint8_t rotation_mode;
} ActorMotionData;

typedef struct {
	float target_yaw;
	bool roll_triggered;
	bool jump_held;
	bool jump_triggered;
} MotionCommand;

typedef struct ActorMotion {
	ActorMotionSettings settings;
	ActorMotionData data;
} ActorMotion;

void actor_updateMotion(Entity *entity, MotionCommand *cmd, float dt);

#endif
