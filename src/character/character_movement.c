#include <assert.h>
#include <math.h>
#include <fmath.h>

#include "character/character.h"
#include "physics/math/math_common.h"


static const bool movement_updates_locomotion[MOVEMENT_STATE_COUNT] = {
	[MOVEMENT_STATE_IDLE]    = true,
	[MOVEMENT_STATE_WALKING] = true,
	[MOVEMENT_STATE_ROLLING] = false,
	[MOVEMENT_STATE_JUMPING] = false,
	[MOVEMENT_STATE_FALLING] = false,
};

void characterMovement_setMode(CharacterMovement *movement, uint8_t new_mode)
{
	if (movement->current == new_mode) return;
	movement->current = new_mode;
	if (movement_updates_locomotion[new_mode]) movement->locomotion = new_mode;
}

bool characterMovement_isLocomotion(uint8_t mode)
{
	return movement_updates_locomotion[mode];
}

static void characterMovement_evaluateTransitions(Character *character)
{
	CharacterMovement *movement = &character->movement;
	if (movement->next == MOVEMENT_STATE_NONE) return;
	characterMovement_setMode(movement, movement->next);
	movement->next = MOVEMENT_STATE_NONE;
}


static const CharacterGaitSettings *characterMovement_gait(const Character *character)
{
	const CharacterMovementSettings *settings = character->movement.settings;
	uint8_t gait = character->movement.data.gait;
	if (gait >= settings->gait_count) gait = settings->gait_count - 1;
	return &settings->gait[gait];
}

static float characterMovement_targetSpeed(const Character *character, uint8_t state)
{
	if (state == MOVEMENT_STATE_WALKING) return characterMovement_gait(character)->target_speed;
	return character->movement.settings->idle_target_speed;
}

static float characterMovement_accelerationRate(const Character *character, uint8_t state)
{
	if (state == MOVEMENT_STATE_WALKING) return characterMovement_gait(character)->response_rate;
	return character->movement.settings->idle_response_rate;
}

static float characterMovement_rotationAccelerationRate(const Character *character, uint8_t state)
{
	if (state == MOVEMENT_STATE_WALKING) return characterMovement_gait(character)->rotation_response_rate;
	return character->movement.settings->idle_rotation_response_rate;
}

static void characterMovement_setHorizontalVelocity(Character *character, float yaw, float target_speed, float response_rate, float dt)
{
	KinematicBody *body = &character->body;

	float target_vx = target_speed *  fm_sinf(deg_to_rad(yaw));
	float target_vy = target_speed * -fm_cosf(deg_to_rad(yaw));

	float factor = fm_expf(-response_rate * dt);
	body->velocity.x = body->velocity.x * factor + target_vx * (1.0f - factor);
	body->velocity.y = body->velocity.y * factor + target_vy * (1.0f - factor);
}

/*
static void characterMovement_setHorizontalVelocity(Character *character, float yaw, float target_speed, float response_rate, float dt)
{
	KinematicBody *body = &character->body;

	float target_vx = target_speed *  fm_sinf(deg_to_rad(yaw));
	float target_vy = target_speed * -fm_cosf(deg_to_rad(yaw));

	body->acceleration.x = response_rate * (target_vx - body->velocity.x);
	body->acceleration.y = response_rate * (target_vy - body->velocity.y);

	body->velocity.x += body->acceleration.x * dt;
	body->velocity.y += body->acceleration.y * dt;
}
*/

static void characterMovement_setRotation(Character *character, float dt)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;

	bool moving = body->velocity.x != 0 || body->velocity.y != 0;

	if (moving) {
		Vector2 horizontal_velocity = {body->velocity.x, body->velocity.y};
		data->horizontal_speed = vector2_magnitude(&horizontal_velocity);
	}

	float facing_yaw;
	if (data->strafe) {
		if (!moving) return;
		facing_yaw = data->strafe_yaw;
	} else {
		if (!moving) return;
		facing_yaw = rad_to_deg(fm_atan2f(-body->velocity.x, -body->velocity.y));

		if (data->rotation_mode == CHARACTER_ROTATION_MODE_SNAP) {
			body->rotation.z = angle_wrap(facing_yaw);
			return;
		}
	}

	const float current_yaw = body->rotation.z;
	const float target_yaw = angle_wrap_relative(facing_yaw, current_yaw);

	if (fabsf(target_yaw - current_yaw) <= CHARACTER_ROTATION_SNAP_THRESHOLD) {
		body->rotation.z = target_yaw;
		return;
	}

	uint8_t state = character->movement.current;
	if (state == MOVEMENT_STATE_ROLLING || state == MOVEMENT_STATE_JUMPING || state == MOVEMENT_STATE_FALLING)
		state = character->movement.locomotion;
	float response_rate = characterMovement_rotationAccelerationRate(character, state);
	float factor = fm_expf(-response_rate * dt);
	body->rotation.z = angle_wrap(current_yaw * factor + target_yaw * (1.0f - factor));
}

static void characterMovement_updateBody(Character *character, float dt)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;

	data->previous_yaw = body->rotation.z;

	if (body->acceleration.z)
		body->velocity.z += body->acceleration.z * dt;

	if (fabsf(body->velocity.x) < LOCOMOTION_MIN_SPEED && fabsf(body->velocity.y) < LOCOMOTION_MIN_SPEED && body->velocity.z == 0) {
		body->velocity.x = 0;
		body->velocity.y = 0;
		data->horizontal_speed = 0;
	}

	if (body->velocity.x != 0 || body->velocity.y != 0 || body->velocity.z != 0)
		vector3_addScaledVector(&body->position, &body->velocity, dt);

	characterMovement_setRotation(character, dt);
}

static void characterMovement_setLocomotion(Character *character, MovementCommand *cmd, float dt)
{
	uint8_t state = character->movement.current;
	characterMovement_setHorizontalVelocity(character, cmd->target_yaw, characterMovement_targetSpeed(character, state) * cmd->speed_scale, characterMovement_accelerationRate(character, state), dt);
}

static uint8_t characterMovement_rollPhase(const CharacterMovementData *data, const MovementCommand *cmd, const CharacterMovementSettings *settings)
{
	if (cmd->roll_triggered)                         return CHARACTER_ROLL_PHASE_LAUNCH;
	if (data->roll_timer < settings->roll_grip_time) return CHARACTER_ROLL_PHASE_SPIN;
	if (data->roll_timer < settings->roll_timer_max) return CHARACTER_ROLL_PHASE_GRIP;
	return CHARACTER_ROLL_PHASE_DONE;
}

static void characterMovement_rollLaunch(Character *character, MovementCommand *cmd, const CharacterMovementSettings *settings, uint8_t locomotion, float dt)
{
	(void)locomotion;
	CharacterMovementData *data = &character->movement.data;
	characterMovement_setHorizontalVelocity(character, data->roll_yaw, settings->roll_target_speed, settings->roll_launch_response_rate, dt);
	data->roll_timer += dt;
	if (data->roll_timer >= settings->roll_ground_time) cmd->roll_triggered = false;
}

static void characterMovement_rollSpin(Character *character, const CharacterMovementSettings *settings, float dt)
{
	CharacterMovementData *data = &character->movement.data;
	characterMovement_setHorizontalVelocity(character, -character->body.rotation.z, data->horizontal_speed, settings->roll_spin_response_rate, dt);
	data->roll_timer += dt;
}

static void characterMovement_rollGrip(Character *character, MovementCommand *cmd, const CharacterMovementSettings *settings, float dt)
{
	CharacterMovementData *data = &character->movement.data;
	characterMovement_setHorizontalVelocity(character, cmd->target_yaw, data->horizontal_speed, settings->roll_grip_response_rate, dt);
	data->roll_timer += dt;
}

static void characterMovement_rollDone(Character *character)
{
	character->movement.next = character->movement.locomotion;
	character->movement.data.roll_timer = 0;
}

static void characterMovement_setRolling(Character *character, MovementCommand *cmd, float dt)
{
	CharacterMovementData *data = &character->movement.data;
	const CharacterMovementSettings *settings = character->movement.settings;
	uint8_t locomotion = character->movement.locomotion;

	/* the stick yaw is taken once on entry and held until grip */
	if (data->roll_timer == 0.0f) data->roll_yaw = cmd->target_yaw;

	switch (characterMovement_rollPhase(data, cmd, settings)) {
		case CHARACTER_ROLL_PHASE_LAUNCH: characterMovement_rollLaunch(character, cmd, settings, locomotion, dt); break;
		case CHARACTER_ROLL_PHASE_SPIN:   characterMovement_rollSpin(character, settings, dt);                    break;
		case CHARACTER_ROLL_PHASE_GRIP:   characterMovement_rollGrip(character, cmd, settings, dt);               break;
		case CHARACTER_ROLL_PHASE_DONE:   characterMovement_rollDone(character);                                  break;
	}
}

static uint8_t characterMovement_jumpPhase(const CharacterMovementData *data, const KinematicBody *body, const CharacterMovementSettings *settings)
{
	if (data->jump_timer < settings->jump_timer_max) return CHARACTER_JUMP_PHASE_CHARGING;
	if (data->jump_force > 0)               return CHARACTER_JUMP_PHASE_LAUNCH;
	if (body->velocity.z > 0)               return CHARACTER_JUMP_PHASE_RISING;
	return CHARACTER_JUMP_PHASE_DONE;
}

static void characterMovement_jumpCharging(Character *character, MovementCommand *cmd, float dt)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;

	data->jump_timer += dt;
	if (cmd->jump_held) {
		data->jump_force += dt;
		vector3_scale(&body->velocity, CHARACTER_JUMP_HOLD_VELOCITY_SCALE);
	}
}

static void characterMovement_jumpLaunch(Character *character, float dt)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;
	const CharacterMovementSettings *settings = character->movement.settings;

	data->jump_timer += dt;
	body->velocity = data->jump_initial_velocity;
	vector3_scale(&body->velocity, CHARACTER_JUMP_LAUNCH_VELOCITY_SCALE);
	body->velocity.z = data->jump_force * settings->jump_force_multiplier;
	if (body->velocity.z < settings->jump_minimum_speed)
		body->velocity.z = settings->jump_minimum_speed;
	data->jump_force = 0;
}

static void characterMovement_jumpRising(Character *character, float dt)
{
	CharacterMovementData *data = &character->movement.data;

	data->jump_timer += dt;
	character->body.acceleration.z = CHARACTER_GRAVITY;
}

static void characterMovement_jumpDone(Character *character)
{
	character->body.acceleration.z = CHARACTER_GRAVITY;
	character->movement.data.jump_timer = 0;
	character->movement.next = MOVEMENT_STATE_FALLING;
}

static void characterMovement_setJump(Character *character, MovementCommand *cmd, float dt)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;
	const CharacterMovementSettings *settings = character->movement.settings;

	if (cmd->jump_triggered) {
		data->jump_initial_velocity = body->velocity;
		data->jump_timer = 0;
		data->jump_force = 0;
		cmd->jump_triggered = false;
	}

	characterMovement_setHorizontalVelocity(character, cmd->target_yaw, data->horizontal_speed, settings->jump_response_rate, dt);

	switch (characterMovement_jumpPhase(data, body, settings)) {
		case CHARACTER_JUMP_PHASE_CHARGING: characterMovement_jumpCharging(character, cmd, dt); break;
		case CHARACTER_JUMP_PHASE_LAUNCH:   characterMovement_jumpLaunch(character, dt);        break;
		case CHARACTER_JUMP_PHASE_RISING:   characterMovement_jumpRising(character, dt);        break;
		case CHARACTER_JUMP_PHASE_DONE:     characterMovement_jumpDone(character);              break;
	}
}

static void characterMovement_setFalling(Character *character, MovementCommand *cmd, float dt)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;
	const CharacterMovementSettings *settings = character->movement.settings;

	data->is_grounded = 0;
	characterMovement_setHorizontalVelocity(character, cmd->target_yaw, data->horizontal_speed, settings->jump_response_rate, dt);
	body->acceleration.z = CHARACTER_GRAVITY;
	if (body->velocity.z < CHARACTER_FALL_MAX_SPEED)
		body->velocity.z = CHARACTER_FALL_MAX_SPEED;
}

static void (*characterMovement_handler[MOVEMENT_STATE_COUNT])(Character *, MovementCommand *, float) = {
	[MOVEMENT_STATE_IDLE]    = characterMovement_setLocomotion,
	[MOVEMENT_STATE_WALKING] = characterMovement_setLocomotion,
	[MOVEMENT_STATE_ROLLING] = characterMovement_setRolling,
	[MOVEMENT_STATE_JUMPING] = characterMovement_setJump,
	[MOVEMENT_STATE_FALLING] = characterMovement_setFalling,
};

_Static_assert(sizeof(characterMovement_handler) / sizeof(characterMovement_handler[0]) == MOVEMENT_STATE_COUNT, "characterMovement_handler must have one entry per character state");

void character_updateMovement(Character *character, MovementCommand *cmd, float dt)
{
	assert(character);
	assert(character);
	assert(cmd);


	assert(character->movement.current < MOVEMENT_STATE_COUNT);
	assert(characterMovement_handler[character->movement.current] != NULL);

	character->movement.data.rotation_mode = CHARACTER_ROTATION_MODE_LERP;
	character->movement.data.strafe        = cmd->strafe;
	character->movement.data.strafe_locked = cmd->strafe_locked;
	character->movement.data.strafe_yaw    = cmd->strafe_yaw;

	/* A scaled-down command locks the top gait away: the character stays on
	   the previous one until the scale is back at full. */
	uint8_t gait = cmd->gait;
	if (cmd->speed_scale < 1.0f && gait == character->movement.settings->gait_count - 1)
		gait = character->movement.settings->gait_count - 2;
	character->movement.data.gait = gait;
	character->movement.next = MOVEMENT_STATE_NONE;

	characterMovement_handler[character->movement.current](character, cmd, dt);
	characterMovement_updateBody(character, dt);
	characterMovement_evaluateTransitions(character);
}
