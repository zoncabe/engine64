#ifndef CHARACTER_MOVEMENT_H
#define CHARACTER_MOVEMENT_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dmath.h>

typedef struct Character Character;


#define LOCOMOTION_MIN_SPEED 0.05f

#define CHARACTER_ROTATION_SNAP_THRESHOLD 1.0f

#define CHARACTER_STRAFE_YAW_OFFSET 0.0f

#define CHARACTER_ROTATION_MODE_LERP 0
#define CHARACTER_ROTATION_MODE_SNAP 1

#define CHARACTER_ROLL_PHASE_LAUNCH 0
#define CHARACTER_ROLL_PHASE_SPIN    1
#define CHARACTER_ROLL_PHASE_GRIP   2
#define CHARACTER_ROLL_PHASE_DONE   3

#define CHARACTER_JUMP_PHASE_CHARGING 0
#define CHARACTER_JUMP_PHASE_LAUNCH   1
#define CHARACTER_JUMP_PHASE_RISING   2
#define CHARACTER_JUMP_PHASE_DONE     3

#define CHARACTER_JUMP_HOLD_VELOCITY_SCALE 0.96f
#define CHARACTER_JUMP_LAUNCH_VELOCITY_SCALE 0.8f

#define CHARACTER_GRAVITY -18.0f
#define CHARACTER_FALL_MAX_SPEED -15.0f

/* Fake buoyancy: the capsule seeks the depth where the scaled gravity flips
   sign. The equilibrium follows the pose the clips are authored at: treading
   water holds the head at the capsule top, stroking holds the body at its
   middle, so speed slides the target between the two and the swim rides up
   to the surface on the same spring. */
#define CHARACTER_WATER_EQUILIBRIUM_IDLE 0.85f
#define CHARACTER_WATER_EQUILIBRIUM_SWIM 0.55f
#define CHARACTER_WATER_DRAG             4.0f    /* vertical, per second, at full submersion */
#define CHARACTER_WATER_SINK_MAX_SPEED  -2.0f    /* fully reached at the fraction below */
#define CHARACTER_WATER_SINK_LIMIT_FULL  0.8f    /* submersion where the sink limit saturates */

/* Swim state thresholds on the submerged fraction, with hysteresis so the
   waves cannot flicker the state at the boundary. Exit also needs footing. */
#define CHARACTER_WATER_SWIM_ENTER     0.6f
#define CHARACTER_WATER_SWIM_EXIT      0.45f

enum {
	CHARACTER_SWIM_GAIT_IDLE = 0,
	CHARACTER_SWIM_GAIT_SLOW = 1,
	CHARACTER_SWIM_GAIT_FAST = 2,
};


typedef enum {
	MOVEMENT_STATE_IDLE,
	MOVEMENT_STATE_WALKING,
	MOVEMENT_STATE_ROLLING,
	MOVEMENT_STATE_JUMPING,
	MOVEMENT_STATE_FALLING,
	MOVEMENT_STATE_SWIMMING,
	MOVEMENT_STATE_COUNT,
	MOVEMENT_STATE_NONE
} MovementState;

/* One gait phase of the WALKING state. How many and their values are up to
   the caller; the order runs from lowest to highest target_speed. */
typedef struct {
	float target_speed;
	float response_rate;
	float rotation_response_rate;
} CharacterGaitSettings;

typedef struct {

	float idle_target_speed;
	float idle_response_rate;
	float idle_rotation_response_rate;

	const CharacterGaitSettings *gait;
	uint8_t gait_count;

	/* Stamina is normalized 0..1; rates are per second. Tired caps the
	   reachable speed at this fraction of the top gait. */
	float stamina_drain_rate;
	float stamina_regen_rate;
	float tired_speed_scale;

	float roll_target_speed;
	float roll_launch_response_rate;
	float roll_spin_response_rate;
	float roll_grip_response_rate;
	float roll_ground_time;
	float roll_grip_time;
	float roll_timer_max;

	float jump_response_rate;
	float jump_force_multiplier;
	float jump_minimum_speed;
	float jump_timer_max;

	float swim_slow_speed;
	float swim_fast_speed;
	float swim_response_rate;

} CharacterMovementSettings;

typedef struct {
	float previous_yaw;
	float horizontal_speed;
	float roll_timer;
	Vector3 jump_initial_velocity;
	float jump_force;
	float jump_timer;
	float roll_yaw;
	bool is_grounded;
	bool in_water;
	float submerged_fraction;   /* 0..1 of the capsule under the surface */
	uint8_t rotation_mode;
	bool strafe;
	bool strafe_locked;
	float strafe_yaw;
	uint8_t gait;
} CharacterMovementData;

typedef struct {
	float target_yaw;
	bool roll_triggered;
	bool jump_held;
	bool jump_triggered;
	bool strafe;
	bool strafe_locked;
	float strafe_yaw;
	uint8_t gait;
	uint8_t swim_gait;   /* CHARACTER_SWIM_GAIT_*, from the stick while swimming */
	float speed_scale;   /* 1.0 normal, tired_speed_scale while tired */
} MovementCommand;

typedef struct CharacterMovement {
	const CharacterMovementSettings *settings;
	CharacterMovementData data;
	uint8_t current;
	uint8_t locomotion;
	uint8_t next;
} CharacterMovement;

void character_updateMovement(Character *character, MovementCommand *cmd, float dt);
void characterMovement_setMode(CharacterMovement *movement, uint8_t new_mode);
bool characterMovement_isLocomotion(uint8_t mode);

#endif
