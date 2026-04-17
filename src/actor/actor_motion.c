#include <assert.h>
#include <math.h>
#include <fmath.h>

#include "entity/entity.h"


static LocomotionType actorMotion_getLocomotionType(uint8_t state)
{
	switch (state) {
		case ACTOR_STATE_WALKING:   return LOCOMOTION_WALK;
		case ACTOR_STATE_RUNNING:   return LOCOMOTION_RUN;
		case ACTOR_STATE_SPRINTING: return LOCOMOTION_SPRINT;
		default:        return LOCOMOTION_IDLE;
	}
}

static void actorMotion_setHorizontalVelocity(Entity *entity, float yaw, float target_speed, float response_rate, float dt)
{
	RigidBody *body = &entity->actor->body;

	float target_vx = target_speed *  fm_sinf(rad(yaw));
	float target_vy = target_speed * -fm_cosf(rad(yaw));

	float factor = fm_exp(-response_rate * dt);
	body->velocity.x = body->velocity.x * factor + target_vx * (1.0f - factor);
	body->velocity.y = body->velocity.y * factor + target_vy * (1.0f - factor);
}

static void actorMotion_setRotation(Entity *entity, float dt)
{
	Actor *actor = entity->actor;
	RigidBody *body = &actor->body;
	ActorMotionData *data = &actor->motion.data;
	const ActorMotionSettings *settings = &actor->motion.settings;

	if (body->velocity.x == 0 && body->velocity.y == 0) return;

	Vector2 horizontal_velocity = {body->velocity.x, body->velocity.y};
	data->horizontal_speed = vector2_magnitude(&horizontal_velocity);

	const float velocity_yaw = deg(fm_atan2f(-body->velocity.x, -body->velocity.y));

	if (data->rotation_mode == ACTOR_ROTATION_MODE_SNAP) {
		entity->transform.rotation.z = angle_wrap(velocity_yaw);
		return;
	}

	const float current_yaw = entity->transform.rotation.z;
	const float target_yaw = angle_wrap_relative(velocity_yaw, current_yaw);

	if (fabsf(target_yaw - current_yaw) <= ACTOR_ROTATION_SNAP_THRESHOLD) {
		entity->transform.rotation.z = target_yaw;
		return;
	}

	uint8_t state = actor->state.current;
	if (state == ACTOR_STATE_ROLLING || state == ACTOR_STATE_JUMPING || state == ACTOR_STATE_FALLING)
		state = actor->state.locomotion;
	LocomotionType locomotion = actorMotion_getLocomotionType(state);
	float response_rate = settings->locomotion[locomotion].rotation_response_rate;
	float factor = fm_exp(-response_rate * dt);
	entity->transform.rotation.z = angle_wrap(current_yaw * factor + target_yaw * (1.0f - factor));
}

static void actorMotion_updateBody(Entity *entity, float dt)
{
	Actor *actor = entity->actor;
	RigidBody *body = &actor->body;
	ActorMotionData *data = &actor->motion.data;

	data->previous_yaw = entity->transform.rotation.z;

	if (body->acceleration.z)
		body->velocity.z += body->acceleration.z * dt;

	if (fabsf(body->velocity.x) < LOCOMOTION_MIN_SPEED && fabsf(body->velocity.y) < LOCOMOTION_MIN_SPEED && body->velocity.z == 0) {
		body->velocity.x = 0;
		body->velocity.y = 0;
		data->horizontal_speed = 0;
	}

	if (body->velocity.x != 0 || body->velocity.y != 0 || body->velocity.z != 0)
		vector3_addScaledVector(&entity->transform.position, &body->velocity, dt);

	actorMotion_setRotation(entity, dt);
}

static void actorMotion_setLocomotion(Entity *entity, MotionCommand *cmd, float dt)
{
	const ActorMotionSettings *settings = &entity->actor->motion.settings;
	LocomotionType locomotion = actorMotion_getLocomotionType(entity->actor->state.current);
	actorMotion_setHorizontalVelocity(entity, cmd->target_yaw, settings->locomotion[locomotion].target_speed, settings->locomotion[locomotion].response_rate, dt);
}

static uint8_t actorMotion_rollPhase(const ActorMotionData *data, const MotionCommand *cmd, const RollSettings *roll)
{
	if (cmd->roll_triggered)                return ACTOR_ROLL_PHASE_LAUNCH;
	if (data->roll_timer < roll->grip_time) return ACTOR_ROLL_PHASE_SPIN;
	if (data->roll_timer < roll->timer_max) return ACTOR_ROLL_PHASE_GRIP;
	return ACTOR_ROLL_PHASE_DONE;
}

static void actorMotion_rollLaunch(Entity *entity, MotionCommand *cmd, const RollSettings *roll, LocomotionType locomotion, float dt)
{
	ActorMotionData *data = &entity->actor->motion.data;
	actorMotion_setHorizontalVelocity(entity, cmd->target_yaw, roll->target_speed[locomotion], roll->launch_response_rate, dt);
	data->roll_timer += dt;
	if (data->roll_timer >= roll->ground_time) cmd->roll_triggered = false;
}

static void actorMotion_rollSpin(Entity *entity, const RollSettings *roll, float dt)
{
	ActorMotionData *data = &entity->actor->motion.data;
	actorMotion_setHorizontalVelocity(entity, -entity->transform.rotation.z, data->horizontal_speed, roll->spin_response_rate, dt);
	data->roll_timer += dt;
}

static void actorMotion_rollGrip(Entity *entity, MotionCommand *cmd, const RollSettings *roll, float dt)
{
	ActorMotionData *data = &entity->actor->motion.data;
	actorMotion_setHorizontalVelocity(entity, cmd->target_yaw, data->horizontal_speed, roll->grip_response_rate, dt);
	data->roll_timer += dt;
}

static void actorMotion_rollDone(Entity *entity)
{
	Actor *actor = entity->actor;
	actor->state.next = actor->state.locomotion;
	actor->motion.data.roll_timer = 0;
}

static void actorMotion_setRolling(Entity *entity, MotionCommand *cmd, float dt)
{
	Actor *actor = entity->actor;
	ActorMotionData *data = &actor->motion.data;
	const RollSettings *roll = &actor->motion.settings.roll;
	LocomotionType locomotion = actorMotion_getLocomotionType(actor->state.locomotion);

	data->rotation_mode = ACTOR_ROTATION_MODE_SNAP;

	switch (actorMotion_rollPhase(data, cmd, roll)) {
		case ACTOR_ROLL_PHASE_LAUNCH: actorMotion_rollLaunch(entity, cmd, roll, locomotion, dt); break;
		case ACTOR_ROLL_PHASE_SPIN:   actorMotion_rollSpin(entity, roll, dt);                    break;
		case ACTOR_ROLL_PHASE_GRIP:   actorMotion_rollGrip(entity, cmd, roll, dt);               break;
		case ACTOR_ROLL_PHASE_DONE:   actorMotion_rollDone(entity);                              break;
	}
}

static uint8_t actorMotion_jumpPhase(const ActorMotionData *data, const RigidBody *body, const JumpSettings *jump)
{
	if (data->jump_timer < jump->timer_max) return ACTOR_JUMP_PHASE_CHARGING;
	if (data->jump_force > 0)               return ACTOR_JUMP_PHASE_LAUNCH;
	if (body->velocity.z > 0)               return ACTOR_JUMP_PHASE_RISING;
	return ACTOR_JUMP_PHASE_DONE;
}

static void actorMotion_jumpCharging(Entity *entity, MotionCommand *cmd, float dt)
{
	Actor *actor = entity->actor;
	RigidBody *body = &actor->body;
	ActorMotionData *data = &actor->motion.data;

	data->jump_timer += dt;
	if (cmd->jump_held) {
		data->jump_force += dt;
		vector3_scale(&body->velocity, ACTOR_JUMP_HOLD_VELOCITY_SCALE);
	}
}

static void actorMotion_jumpLaunch(Entity *entity, float dt)
{
	Actor *actor = entity->actor;
	RigidBody *body = &actor->body;
	ActorMotionData *data = &actor->motion.data;
	const JumpSettings *jump = &actor->motion.settings.jump;

	data->jump_timer += dt;
	body->velocity = data->jump_initial_velocity;
	vector3_scale(&body->velocity, ACTOR_JUMP_LAUNCH_VELOCITY_SCALE);
	body->velocity.z = data->jump_force * jump->force_multiplier;
	if (body->velocity.z < jump->minimum_speed)
		body->velocity.z = jump->minimum_speed;
	data->jump_force = 0;
}

static void actorMotion_jumpRising(Entity *entity, float dt)
{
	Actor *actor = entity->actor;
	ActorMotionData *data = &actor->motion.data;

	data->jump_timer += dt;
	actor->body.acceleration.z = ACTOR_GRAVITY;
}

static void actorMotion_jumpDone(Entity *entity)
{
	Actor *actor = entity->actor;
	actor->body.acceleration.z = ACTOR_GRAVITY;
	actor->motion.data.jump_timer = 0;
	actor->state.next = ACTOR_STATE_FALLING;
}

static void actorMotion_setJump(Entity *entity, MotionCommand *cmd, float dt)
{
	Actor *actor = entity->actor;
	RigidBody *body = &actor->body;
	ActorMotionData *data = &actor->motion.data;
	const JumpSettings *jump = &actor->motion.settings.jump;

	if (cmd->jump_triggered) {
		data->jump_initial_velocity = body->velocity;
		cmd->jump_triggered = false;
	}

	actorMotion_setHorizontalVelocity(entity, cmd->target_yaw, data->horizontal_speed, jump->response_rate, dt);

	switch (actorMotion_jumpPhase(data, body, jump)) {
		case ACTOR_JUMP_PHASE_CHARGING: actorMotion_jumpCharging(entity, cmd, dt); break;
		case ACTOR_JUMP_PHASE_LAUNCH:   actorMotion_jumpLaunch(entity, dt);        break;
		case ACTOR_JUMP_PHASE_RISING:   actorMotion_jumpRising(entity, dt);        break;
		case ACTOR_JUMP_PHASE_DONE:     actorMotion_jumpDone(entity);              break;
	}
}

static void actorMotion_setFalling(Entity *entity, MotionCommand *cmd, float dt)
{
	Actor *actor = entity->actor;
	RigidBody *body = &actor->body;
	ActorMotionData *data = &actor->motion.data;
	const JumpSettings *jump = &actor->motion.settings.jump;

	data->is_grounded = 0;
	actorMotion_setHorizontalVelocity(entity, cmd->target_yaw, data->horizontal_speed, jump->response_rate, dt);
	body->acceleration.z = ACTOR_GRAVITY;
	if (body->velocity.z < ACTOR_FALL_MAX_SPEED)
		body->velocity.z = ACTOR_FALL_MAX_SPEED;

	if (entity->transform.position.z <= data->grounding_height + ACTOR_GROUNDING_SNAP_ZONE) {
		data->is_grounded = 1;
		body->acceleration.z = 0;
		body->velocity.z = 0;
		entity->transform.position.z = data->grounding_height;
		actor->state.next = actor->state.locomotion;
		return;
	}
}

static void (*actorMotion_handler[ACTOR_STATE_COUNT])(Entity *, MotionCommand *, float) = {
	[ACTOR_STATE_IDLE]      = actorMotion_setLocomotion,
	[ACTOR_STATE_WALKING]   = actorMotion_setLocomotion,
	[ACTOR_STATE_RUNNING]   = actorMotion_setLocomotion,
	[ACTOR_STATE_SPRINTING] = actorMotion_setLocomotion,
	[ACTOR_STATE_ROLLING]   = actorMotion_setRolling,
	[ACTOR_STATE_JUMPING]   = actorMotion_setJump,
	[ACTOR_STATE_FALLING]   = actorMotion_setFalling,
};

_Static_assert(sizeof(actorMotion_handler) / sizeof(actorMotion_handler[0]) == ACTOR_STATE_COUNT, "actorMotion_handler must have one entry per actor state");

void actor_updateMotion(Entity *entity, MotionCommand *cmd, float dt)
{
	assert(entity);
	assert(entity->actor);
	assert(cmd);

	Actor *actor = entity->actor;

	assert(actor->state.current < ACTOR_STATE_COUNT);
	assert(actorMotion_handler[actor->state.current] != NULL);

	actor->motion.data.rotation_mode = ACTOR_ROTATION_MODE_LERP;
	actor->state.next = ACTOR_STATE_NONE;

	actorMotion_handler[actor->state.current](entity, cmd, dt);
	actorMotion_updateBody(entity, dt);
	actorStates_evaluateTransitions(entity);
}
