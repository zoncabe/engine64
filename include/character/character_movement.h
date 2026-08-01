#ifndef CHARACTER_MOVEMENT_H
#define CHARACTER_MOVEMENT_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dmath.h>

typedef struct Character Character;


#define LOCOMOTION_MIN_SPEED 0.05f

#define CHARACTER_ROTATION_SNAP_THRESHOLD 1.0f

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
	MOVEMENT_STATE_RUNNING,
	MOVEMENT_STATE_SPRINTING,
	MOVEMENT_STATE_ROLLING,
	MOVEMENT_STATE_JUMPING,
	MOVEMENT_STATE_FALLING,
	MOVEMENT_STATE_COUNT,
	MOVEMENT_STATE_NONE
} MovementState;

typedef struct {

	float idle_target_speed;
	float idle_acceleration_rate;
	float idle_rotation_acceleration_rate;

	float walk_target_speed;
	float walk_acceleration_rate;
	float walk_rotation_acceleration_rate;

	float run_target_speed;
	float run_acceleration_rate;
	float run_rotation_acceleration_rate;

	float sprint_target_speed;
	float sprint_acceleration_rate;
	float sprint_rotation_acceleration_rate;

	float roll_launch_acceleration_rate;
	float roll_spin_acceleration_rate;
	float roll_grip_acceleration_rate;
	float roll_ground_time;
	float roll_grip_time;
	float roll_timer_max;
	float roll_target_speed_walk;
	float roll_target_speed_run;
	float roll_target_speed_sprint;

	float jump_acceleration_rate;
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
	bool is_grounded;
	uint8_t rotation_mode;
} CharacterMovementData;

typedef struct {
	float target_yaw;
	bool roll_triggered;
	bool jump_held;
	bool jump_triggered;
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
