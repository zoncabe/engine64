#include <assert.h>
#include <math.h>
#include <fmath.h>

#include "character/character.h"
#include "physics/math/math_common.h"


static const bool movement_updates_locomotion[MOVEMENT_STATE_COUNT] = {
	[MOVEMENT_STATE_IDLE]     = true,
	[MOVEMENT_STATE_WALKING]  = true,
	[MOVEMENT_STATE_ROLLING]  = false,
	[MOVEMENT_STATE_FALLING]  = false,
	[MOVEMENT_STATE_SWIMMING] = false,
	[MOVEMENT_STATE_CLIMBING] = false,
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

	float s, c;
	fm_sincosf(deg_to_rad(yaw), &s, &c);
	float target_vx = target_speed *  s;
	float target_vy = target_speed * -c;

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

	/* Climbing already set the facing from the ladder, and its velocity is
	   the pull onto the anchor: read as a heading it would spin the body. */
	if (character->movement.current == MOVEMENT_STATE_CLIMBING) return;

	bool moving = body->velocity.x != 0 || body->velocity.y != 0;

	if (moving) {
		Vector2 horizontal_velocity = {body->velocity.x, body->velocity.y};
		data->horizontal_speed = vector2_magnitude(&horizontal_velocity);
	}

	float facing_yaw;
	if (data->strafe) {
		/* Holding the bow the body keeps tracking the camera even planted;
		   the plain strafe leaves a standstill facing free. */
		if (!moving && !data->bow_hold && !data->bow_aim) return;
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
	if (state == MOVEMENT_STATE_ROLLING || state == MOVEMENT_STATE_FALLING)
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

	/* A ladder standing in water is still a ladder: the climb owns the
	   vertical, so the buoyancy must not bob the body off its rungs. */
	if (data->in_water && character->movement.current != MOVEMENT_STATE_CLIMBING) {
		const CharacterMovementSettings *settings = character->movement.settings;

		/* Stroking raises the equilibrium so the swim pose meets the surface. */
		float stroke = (settings->swim_slow_speed > 0.0f)
			? data->horizontal_speed / settings->swim_slow_speed : 0.0f;
		if (stroke > 1.0f) stroke = 1.0f;

		float equilibrium = settings->water_equilibrium_idle
			+ (settings->water_equilibrium_swim - settings->water_equilibrium_idle) * stroke;

		/* Gravity scaled by how far the capsule sits from the equilibrium
		   depth: deeper than it, the push turns upward. The drag grows with
		   the submerged body, so a dive keeps its momentum through the
		   surface and brakes progressively on the way down. */
		float buoyant = CHARACTER_GRAVITY * (1.0f - data->submerged_fraction / equilibrium);
		body->velocity.z += buoyant * dt;
		body->velocity.z /= (1.0f + CHARACTER_WATER_DRAG * data->submerged_fraction * dt);

		/* Progressive sink limit: none at the splash so a dive keeps its
		   momentum, full past the saturation fraction — the pool is barely
		   deeper than the capsule, so the brake cannot wait for the drag. */
		float t = data->submerged_fraction / CHARACTER_WATER_SINK_LIMIT_FULL;
		if (t > 1.0f) t = 1.0f;
		float limit = CHARACTER_FALL_MAX_SPEED + (CHARACTER_WATER_SINK_MAX_SPEED - CHARACTER_FALL_MAX_SPEED) * t;
		if (body->velocity.z < limit) body->velocity.z = limit;
	}
	else if (body->acceleration.z)
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

/* The crouch that starts a jump runs here, on the ground, over walk or idle:
   the stick still drives the body and holding the button only brakes it. The
   impulse and the switch to the air come together when the crouch ends, so a
   jump is never a state of its own — the air is always a fall. */
static void characterMovement_jumpCharge(Character *character, MovementCommand *cmd, float dt)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;
	const CharacterMovementSettings *settings = character->movement.settings;

	if (cmd->jump_triggered) {
		data->jump_initial_velocity = body->velocity;
		data->jump_timer    = 0.0f;
		data->jump_force    = 0.0f;
		cmd->jump_triggered = false;
	}
	else if (data->jump_timer == 0.0f) return;   /* nothing being charged */

	data->jump_timer += dt;
	if (cmd->jump_held) {
		data->jump_force += dt;
		vector3_scale(&body->velocity, CHARACTER_JUMP_HOLD_VELOCITY_SCALE);
	}

	if (data->jump_timer < settings->jump_timer_max) return;

	/* Leaving the floor: the launch keeps a share of the run it came from. */
	body->velocity = data->jump_initial_velocity;
	vector3_scale(&body->velocity, CHARACTER_JUMP_LAUNCH_VELOCITY_SCALE);
	body->velocity.z = data->jump_force * settings->jump_force_multiplier;
	if (body->velocity.z < settings->jump_minimum_speed)
		body->velocity.z = settings->jump_minimum_speed;

	data->jump_force = 0.0f;
	data->jump_timer = 0.0f;
	character->movement.next = MOVEMENT_STATE_FALLING;
}

static void characterMovement_setLocomotion(Character *character, MovementCommand *cmd, float dt)
{
	uint8_t state = character->movement.current;
	characterMovement_setHorizontalVelocity(character, cmd->target_yaw, characterMovement_targetSpeed(character, state) * cmd->speed_scale, characterMovement_accelerationRate(character, state), dt);
	characterMovement_jumpCharge(character, cmd, dt);
}

/* Gravity and the terminal speed that goes with it, for anything with no floor
   under it. The collision pass clears the acceleration again on landing. */
static void characterMovement_fall(KinematicBody *body)
{
	body->acceleration.z = CHARACTER_GRAVITY;
	if (body->velocity.z < CHARACTER_FALL_MAX_SPEED)
		body->velocity.z = CHARACTER_FALL_MAX_SPEED;
}

static uint8_t characterMovement_rollPhase(const CharacterMovementData *data, const CharacterMovementSettings *settings)
{
	if (data->roll_timer < settings->roll_ground_time) return CHARACTER_ROLL_PHASE_LAUNCH;
	if (data->roll_timer < settings->roll_grip_time)   return CHARACTER_ROLL_PHASE_SPIN;
	if (data->roll_timer < settings->roll_timer_max)   return CHARACTER_ROLL_PHASE_GRIP;
	return CHARACTER_ROLL_PHASE_DONE;
}

static void characterMovement_rollLaunch(Character *character, const CharacterMovementSettings *settings, uint8_t locomotion, float dt)
{
	(void)locomotion;
	CharacterMovementData *data = &character->movement.data;
	characterMovement_setHorizontalVelocity(character, data->roll_yaw, settings->roll_target_speed, settings->roll_launch_response_rate, dt);
	data->roll_timer += dt;
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

/* Out to idle, not to the locomotion state held from before the roll: the
   control runs first next frame and the stick raises it to walk if it is
   asking for one, the same way the floor probe turns it into a fall. */
static void characterMovement_rollDone(Character *character)
{
	character->movement.next = MOVEMENT_STATE_IDLE;
	character->movement.data.roll_timer = 0;
}

static void characterMovement_setRolling(Character *character, MovementCommand *cmd, float dt)
{
	CharacterMovementData *data = &character->movement.data;
	const CharacterMovementSettings *settings = character->movement.settings;
	uint8_t locomotion = character->movement.locomotion;

	/* The trigger is the entry mark and is consumed here: the stick yaw is
	   taken once and held until grip, and the timer starts from zero however
	   the previous roll ended — a ledge can end one before its own last phase. */
	if (cmd->roll_triggered) {
		data->roll_yaw      = cmd->target_yaw;
		data->roll_timer    = 0.0f;
		cmd->roll_triggered = false;
	}

	/* Rolling off a ledge drops: the state is what holds to the end of the
	   clip, not the ground. */
	if (!data->is_grounded) characterMovement_fall(&character->body);

	switch (characterMovement_rollPhase(data, settings)) {
		case CHARACTER_ROLL_PHASE_LAUNCH: characterMovement_rollLaunch(character, settings, locomotion, dt); break;
		case CHARACTER_ROLL_PHASE_SPIN:   characterMovement_rollSpin(character, settings, dt);               break;
		case CHARACTER_ROLL_PHASE_GRIP:   characterMovement_rollGrip(character, cmd, settings, dt);          break;
		case CHARACTER_ROLL_PHASE_DONE:   characterMovement_rollDone(character);                             break;
	}
}

static void characterMovement_setFalling(Character *character, MovementCommand *cmd, float dt)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;
	const CharacterMovementSettings *settings = character->movement.settings;

	data->is_grounded = 0;
	/* A crouch cut short by the ground disappearing is not a jump: left half
	   done it would finish on landing and take off on its own. */
	data->jump_timer = 0.0f;
	characterMovement_setHorizontalVelocity(character, cmd->target_yaw, data->horizontal_speed, settings->jump_response_rate, dt);
	characterMovement_fall(body);
}

/* The vertical is not touched here: the fake buoyancy in updateBody floats,
   bobs and damps the capsule on its own. The stick only swims horizontally. */
static void characterMovement_setSwimming(Character *character, MovementCommand *cmd, float dt)
{
	const CharacterMovementSettings *settings = character->movement.settings;
	KinematicBody *body = &character->body;

	/* Tired locks the fast stroke away, same as the top gait on land. */
	uint8_t swim_gait = cmd->swim_gait;
	if (cmd->speed_scale < 1.0f && swim_gait == CHARACTER_SWIM_GAIT_FAST)
		swim_gait = CHARACTER_SWIM_GAIT_SLOW;

	float target = 0.0f;
	if (swim_gait == CHARACTER_SWIM_GAIT_SLOW) target = settings->swim_slow_speed;
	if (swim_gait == CHARACTER_SWIM_GAIT_FAST) target = settings->swim_fast_speed;

	characterMovement_setHorizontalVelocity(character, cmd->target_yaw, target * cmd->speed_scale, settings->swim_response_rate, dt);
	body->acceleration.z = 0.0f;
}

/* On a ladder the state owns the body outright: no gravity, and the stick
   drives the vertical instead of a run. The horizontal is spent entirely on
   pulling the body onto the anchor the probe wrote, so an approach from the
   side slides into the ladder's centre line rather than climbing thin air. */
static void characterMovement_setClimbing(Character *character, MovementCommand *cmd, float dt)
{
	KinematicBody *body = &character->body;
	CharacterMovementData *data = &character->movement.data;
	const CharacterMovementSettings *settings = character->movement.settings;

	body->acceleration.z = 0.0f;

	float target = cmd->climb * settings->climb_speed * cmd->speed_scale;
	float factor = fm_expf(-settings->climb_response_rate * dt);
	body->velocity.z = body->velocity.z * factor + target * (1.0f - factor);

	/* The anchor is a position, not a target speed: the pull is written as
	   the velocity that closes the gap left this frame, so it dies out on
	   arrival instead of circling it. */
	float pull = (1.0f - fm_expf(-CHARACTER_LADDER_ANCHOR_RATE * dt)) / dt;
	body->velocity.x = (data->ladder_anchor_x - body->position.x) * pull;
	body->velocity.y = (data->ladder_anchor_y - body->position.y) * pull;

	data->horizontal_speed = 0.0f;
	data->is_grounded = 0;

	/* The rungs are what the body faces, whatever it faced walking in. */
	body->rotation.z   = data->ladder_yaw;
	data->rotation_mode = CHARACTER_ROTATION_MODE_SNAP;
}

/* Ladder entry and exit, from the climbable probe of the physics pass.
   Entering asks the stick to be pushing at the rungs, so walking past a
   ladder never snatches the body onto it.

   Every exit but the release is a loss of contact: the volume is what says
   where climbing is possible, and running off its top while still climbing
   is the body cresting the ladder. That exit gets a push toward the rungs so
   it lands on the ledge instead of peeling back off the face. */
static void characterMovement_evaluateLadder(Character *character, MovementCommand *cmd)
{
	CharacterMovement *movement = &character->movement;
	CharacterMovementData *data = &movement->data;
	KinematicBody *body = &character->body;

	if (movement->current != MOVEMENT_STATE_CLIMBING) {
		if (!data->on_ladder || cmd->climb <= 0.0f) return;
		if (movement->next != MOVEMENT_STATE_NONE) return;

		/* Swimming counts: a ladder in a pool is how the body gets out. */
		if (!characterMovement_isLocomotion(movement->current)
		 && movement->current != MOVEMENT_STATE_FALLING
		 && movement->current != MOVEMENT_STATE_SWIMMING) return;

		/* Not from the crest: standing on top of the ladder leaves the feet
		   right at the volume's ceiling, and without this margin the climb
		   that just ended there grabs straight back on. */
		if (body->position.z > data->ladder_top - CHARACTER_LADDER_ENTER_MARGIN) return;

		/* Grabbing on kills whatever vertical the body arrived with: a fall
		   caught by a ladder would otherwise ride its own speed down past
		   the rungs while the climb slowly talks it out of it. */
		body->velocity.z = 0.0f;
		movement->next   = MOVEMENT_STATE_CLIMBING;
		return;
	}

	if (cmd->climb_release) {
		movement->next = MOVEMENT_STATE_FALLING;
		return;
	}

	if (!data->on_ladder) {
		/* Off the top with the stick still climbing: step onto the landing.
		   The push is horizontal and the fall carries it from there, so the
		   body arrives with the landing clip it already has.

		   The heading is commandeered along with the velocity. The fall
		   steers toward whatever the stick last asked for, and on a ladder
		   the stick means up, not a direction — left to itself the body
		   would crest the top and immediately walk off wherever the camera
		   happened to be pointing, which mostly means back into the rungs
		   it just left. It holds this heading until the stick is moved. */
		if (cmd->climb > 0.0f) {
			float s, c;
			fm_sincosf(deg_to_rad(-data->ladder_yaw), &s, &c);
			body->velocity.x = CHARACTER_LADDER_EXIT_SPEED *  s;
			body->velocity.y = CHARACTER_LADDER_EXIT_SPEED * -c;
			body->velocity.z = 0.0f;
			data->horizontal_speed = CHARACTER_LADDER_EXIT_SPEED;
			cmd->target_yaw        = -data->ladder_yaw;
		}
		movement->next = MOVEMENT_STATE_FALLING;
		return;
	}

	/* Back at the foot with the stick pushing down: the ground itself is
	   what says so, not the volume — its floor sits below the stair's, so
	   that someone standing at the bottom is inside it to begin with. */
	if (cmd->climb < 0.0f && data->floor_distance >= 0.0f
	 && data->floor_distance <= CHARACTER_LADDER_GROUND_REACH)
		movement->next = movement->locomotion;
}

/* Swim entry and exit, from the water probe of the physics pass. Entering
   needs the chest under the surface; leaving needs footing and shallower
   water than that, so the waves cannot flicker the state at the border. */
static void characterMovement_evaluateWater(Character *character)
{
	CharacterMovement *movement = &character->movement;
	const CharacterMovementData *data = &movement->data;
	uint8_t current = movement->current;

	if (current != MOVEMENT_STATE_SWIMMING) {
		bool can_enter = characterMovement_isLocomotion(current)
		              || current == MOVEMENT_STATE_FALLING
		              || current == MOVEMENT_STATE_ROLLING;

		if (can_enter && data->in_water && data->submerged_fraction >= CHARACTER_WATER_SWIM_ENTER)
			movement->next = MOVEMENT_STATE_SWIMMING;
		return;
	}

	/* Out of the water entirely, locomotion; airborne, the floor probe of the
	   physics pass turns it into falling on its own. */
	if (!data->in_water || (data->is_grounded && data->submerged_fraction < CHARACTER_WATER_SWIM_EXIT))
		movement->next = movement->locomotion;
}

static void (*characterMovement_handler[MOVEMENT_STATE_COUNT])(Character *, MovementCommand *, float) = {
	[MOVEMENT_STATE_IDLE]     = characterMovement_setLocomotion,
	[MOVEMENT_STATE_WALKING]  = characterMovement_setLocomotion,
	[MOVEMENT_STATE_ROLLING]  = characterMovement_setRolling,
	[MOVEMENT_STATE_FALLING]  = characterMovement_setFalling,
	[MOVEMENT_STATE_SWIMMING] = characterMovement_setSwimming,
	[MOVEMENT_STATE_CLIMBING] = characterMovement_setClimbing,
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
	character->movement.data.bow_hold      = cmd->bow_hold;
	character->movement.data.bow_aim       = cmd->bow_aim;

	/* A scaled-down command locks the top gait away: the character stays on
	   the previous one until the scale is back at full. */
	uint8_t gait = cmd->gait;
	if (cmd->speed_scale < 1.0f && gait == character->movement.settings->gait_count - 1)
		gait = character->movement.settings->gait_count - 2;
	character->movement.data.gait = gait;
	character->movement.next = MOVEMENT_STATE_NONE;

	characterMovement_handler[character->movement.current](character, cmd, dt);
	characterMovement_updateBody(character, dt);
	characterMovement_evaluateWater(character);
	characterMovement_evaluateLadder(character, cmd);
	characterMovement_evaluateTransitions(character);
}
