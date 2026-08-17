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


typedef enum {
	MOVEMENT_STATE_IDLE,
	MOVEMENT_STATE_WALKING,
	MOVEMENT_STATE_ROLLING,
	MOVEMENT_STATE_JUMPING,
	MOVEMENT_STATE_FALLING,
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
