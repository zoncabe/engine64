/*
	The movement component, after Unreal's CharacterMovementComponent: the
	velocity model is CalcVelocity and ApplyVelocityBraking, read from the
	engine source (CharacterMovementComponent.cpp). Per frame: the input is
	turned into an acceleration, friction and braking shape the velocity, the
	mode adds its vertical, the collider is moved by the result and turned to
	face it. The collision pass runs after this and corrects what it has to.
*/
#include <assert.h>
#include <math.h>
#include <fmath.h>

#include "character3d/e64_character3d.h"
#include "math/e64_math.h"

namespace e64 {

namespace character3d {

/* Unreal's ApplyVelocityBraking, on the horizontal plane. With no input, the
   friction takes a share of the velocity per second and the braking a fixed
   amount: both bring it to a stop, and the stop is exact — a reversal or the
   brake-to-stop threshold zero it, so it never trails off for ever. */
void Movement::applyBraking(float friction, float braking_deceleration, float dt)
{
	if (friction <= 0.0f && braking_deceleration <= 0.0f) return;

	Vector2 old_velocity = { velocity.x, velocity.y };
	float speed = vector2::magnitude(&old_velocity);
	if (speed == 0.0f) return;

	/* The braking deceleration points against the velocity. */
	float reverse = braking_deceleration / speed;
	velocity.x += (-friction * velocity.x - reverse * velocity.x) * dt;
	velocity.y += (-friction * velocity.y - reverse * velocity.y) * dt;

	/* Overshot past zero, or close enough to it: stop. */
	if (velocity.x * old_velocity.x + velocity.y * old_velocity.y <= 0.0f
	 || velocity.x * velocity.x + velocity.y * velocity.y
	    <= E64_CHARACTER3D_BRAKE_TO_STOP_SPEED * E64_CHARACTER3D_BRAKE_TO_STOP_SPEED) {
		velocity.x = 0.0f;
		velocity.y = 0.0f;
	}
}

/* Unreal's CalcVelocity, on the horizontal plane. The input's length is the
   analog modifier: it scales the max speed and the acceleration both.

   With input, the friction kills the part of the velocity not along it — how
   fast a turn sheds the old heading — then the acceleration adds along it and
   the result is clamped to the max speed. A body already over the max (a
   launch, a push) keeps its speed and only steers: the clamp is to what it
   has, not to the max. Fluid friction decays the velocity whether there is
   input or not, so a stroke never coasts. Without input it brakes. */
void Movement::calcVelocity(const Vector2 *input, float max_speed, float friction,
                            float braking_deceleration, float acceleration_scale, bool fluid, float dt)
{
	float input_size = vector2::magnitude(input);
	if (input_size > 1.0f) input_size = 1.0f;

	float max_acceleration = settings.max_acceleration * acceleration_scale;
	bool zero_acceleration = input_size == 0.0f || max_acceleration <= 0.0f;

	if (zero_acceleration) {
		acceleration.x = 0.0f;
		acceleration.y = 0.0f;
		applyBraking(friction, braking_deceleration, dt);
		if (fluid) {
			float decay = 1.0f - friction * dt;
			if (decay < 0.0f) decay = 0.0f;
			velocity.x *= decay;
			velocity.y *= decay;
		}
		return;
	}

	Vector2 direction = vector2::normalized(input);
	acceleration.x = direction.x * max_acceleration * input_size;
	acceleration.y = direction.y * max_acceleration * input_size;

	/* Friction turns the velocity: the component off the input direction is
	   shed at friction per second, the one along it is kept whole. */
	float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y);
	float turn = friction * dt;
	if (turn > 1.0f) turn = 1.0f;
	velocity.x -= (velocity.x - direction.x * speed) * turn;
	velocity.y -= (velocity.y - direction.y * speed) * turn;

	if (fluid) {
		float decay = 1.0f - friction * dt;
		if (decay < 0.0f) decay = 0.0f;
		velocity.x *= decay;
		velocity.y *= decay;
	}

	float max_input_speed = max_speed * input_size;
	float max_input_speed_sq = max_input_speed * max_input_speed;

	/* Over the max already: keep what it has, steer only. */
	float speed_sq = velocity.x * velocity.x + velocity.y * velocity.y;
	if (speed_sq > max_input_speed_sq * 1.0201f) { /* Unreal's 1% tolerance, squared */
		max_input_speed_sq = speed_sq;
		max_input_speed = sqrtf(speed_sq);
	}

	velocity.x += acceleration.x * dt;
	velocity.y += acceleration.y * dt;

	speed_sq = velocity.x * velocity.x + velocity.y * velocity.y;
	if (speed_sq > max_input_speed_sq) {
		float scale = max_input_speed / sqrtf(speed_sq);
		velocity.x *= scale;
		velocity.y *= scale;
	}
}

/* Gravity and the terminal speed that goes with it. The collision pass clears
   the acceleration again on landing. */
void Movement::applyGravity(float gravity_scale, float dt)
{
	acceleration.z = settings.gravity * gravity_scale;
	velocity.z += acceleration.z * dt;
	if (velocity.z < settings.fall_max_speed)
		velocity.z = settings.fall_max_speed;
}

/* Fake buoyancy. Gravity scaled by how far the capsule sits from the
   equilibrium depth: deeper than it, the push turns upward. Stroking slides
   the equilibrium toward the swim one so the swim pose meets the surface. The
   fluid friction drags the vertical too, growing with the submerged body, so a
   dive keeps its momentum through the surface and brakes on the way down. */
void Movement::applyBuoyancy(float dt)
{
	float stroke = 0.0f;
	if (settings.max_swim_speed > 0.0f) {
		Vector2 horizontal = { velocity.x, velocity.y };
		stroke = vector2::magnitude(&horizontal) / settings.max_swim_speed;
		if (stroke > 1.0f) stroke = 1.0f;
	}

	float equilibrium = settings.water_equilibrium_idle
		+ (settings.water_equilibrium_swim - settings.water_equilibrium_idle) * stroke;

	float buoyant = settings.gravity * (1.0f - data.submerged_fraction / equilibrium);
	acceleration.z = buoyant;
	velocity.z += buoyant * dt;
	velocity.z /= (1.0f + settings.fluid_friction * data.submerged_fraction * dt);

	/* Progressive sink limit: none at the splash so a dive keeps its momentum,
	   the sink max once submerged past the limit fraction — the pool is barely
	   deeper than the capsule, so the brake cannot wait for the drag. */
	float t = data.submerged_fraction / settings.water_sink_limit_full;
	if (t > 1.0f) t = 1.0f;
	float limit = settings.fall_max_speed + (settings.water_sink_max_speed - settings.fall_max_speed) * t;
	if (velocity.z < limit) velocity.z = limit;
}

/* The jump leaves the floor with the velocity it had plus the jump speed up.
   The air is always a fall: a jump is never a mode of its own. */
void Movement::jump(MovementCommand *cmd)
{
	velocity.z = settings.jump_speed;
	data.is_grounded = false;
	data.coyote_timer = settings.jump_coyote_time;
	setMode(MOVEMENT_STATE_FALL);
	cmd->jump_triggered = false;
}

/* Unreal's walking: full friction and braking, no vertical. Idle and walk are
   the same ground: the mode is read off the result after the integration. */
void Movement::updateWalking(MovementCommand *cmd, float dt)
{
	calcVelocity(&cmd->input, settings.max_walk_speed,
	             settings.ground_friction, settings.braking_deceleration_walking,
	             1.0f, false, dt);

	acceleration.z = 0.0f;

	if (cmd->jump_triggered) jump(cmd);
}

/* Unreal's falling: air_control scales the acceleration, the lateral friction
   and the falling braking shape the rest. Coyote time: for a moment after the
   edge the button still launches. Holding it makes the rise cost less gravity,
   only on the way up — past the top the fall is the fall. */
void Movement::updateFalling(MovementCommand *cmd, float dt)
{
	data.is_grounded = false;

	if (data.coyote_timer < settings.jump_coyote_time) {
		data.coyote_timer += dt;
		if (cmd->jump_triggered) jump(cmd);
	}

	calcVelocity(&cmd->input, settings.max_walk_speed,
	             settings.falling_lateral_friction, settings.braking_deceleration_falling,
	             settings.air_control, false, dt);

	float gravity_scale = 1.0f;
	if (cmd->jump_held && velocity.z > 0.0f)
		gravity_scale = settings.jump_hold_gravity_scale;

	applyGravity(gravity_scale, dt);
}

/* Unreal's swimming, horizontal only: the vertical is the buoyancy's. */
void Movement::updateSwimming(MovementCommand *cmd, float dt)
{
	data.is_grounded = false;

	calcVelocity(&cmd->input, settings.max_swim_speed,
	             settings.fluid_friction, settings.braking_deceleration_swimming,
	             1.0f, true, dt);

	applyBuoyancy(dt);
}

/* Climb: no gravity and the horizontal brakes to a stop. What drives the
   vertical and what enters and leaves the mode is not defined yet — the
   command carries no climb axis and the world has no climbable. Until then a
   body put in this mode hangs still where it is. */
void Movement::updateClimbing(MovementCommand *cmd, float dt)
{
	(void)cmd;

	data.is_grounded = false;
	acceleration = (Vector3){ 0.0f, 0.0f, 0.0f };

	applyBraking(0.0f, settings.braking_deceleration_climbing, dt);
	velocity.z = 0.0f;
}

/* Swim entry and exit, from the water probe of the collision pass. Entry is
   the swim equilibrium: sunk past the depth the buoyancy holds the body at,
   the water is already carrying it. Exit trails entry by the exit scale, the
   hysteresis against the waves, and needs footing; out of the water with
   nothing under the feet it is a fall. */
void Movement::evaluateWater()
{
	if (mode != MOVEMENT_STATE_SWIM) {
		if (mode == MOVEMENT_STATE_CLIMB) return;
		if (data.in_water && data.submerged_fraction >= settings.water_equilibrium_swim)
			setMode(MOVEMENT_STATE_SWIM);
		return;
	}

	if (!data.in_water) {
		setMode(MOVEMENT_STATE_FALL);
		return;
	}

	if (data.is_grounded
	 && data.submerged_fraction < settings.water_equilibrium_swim * settings.water_swim_exit_scale)
		setMode(MOVEMENT_STATE_WALK);
}

/* Unreal's bOrientRotationToMovement: the collider turns toward its velocity
   at a constant rate, degrees per second, and snaps the last sliver. Yaw
   convention of the engine: velocity (sin yaw, -cos yaw) faces yaw. The yaw is
   read from the collider's rotation matrix and the matrix is rebuilt from the
   new one, so no wrap is needed. */
void Movement::rotate(Collider *collider, float dt)
{
	if (mode == MOVEMENT_STATE_CLIMB) return;
	if (velocity.x == 0.0f && velocity.y == 0.0f) return;

	float facing_yaw = rad_to_deg(fm_atan2f(-velocity.x, -velocity.y));
	float current_yaw = collider::getYaw(collider);
	float target_yaw = angle_wrap_relative(facing_yaw, current_yaw);
	float delta = target_yaw - current_yaw;
	float step = settings.rotation_rate * dt;

	if (fabsf(delta) <= step || fabsf(delta) <= E64_CHARACTER3D_ROTATION_SNAP_THRESHOLD) {
		collider::setYaw(collider, target_yaw);
		return;
	}

	collider::setYaw(collider, current_yaw + (delta > 0.0f ? step : -step));
}


void Movement::update(Collider *collider, MovementCommand *cmd, float dt)
{
	assert(collider);
	assert(cmd);

	switch (mode) {
		case MOVEMENT_STATE_IDLE:
		case MOVEMENT_STATE_WALK:  updateWalking(cmd, dt); break;
		case MOVEMENT_STATE_FALL:  updateFalling(cmd, dt); break;
		case MOVEMENT_STATE_SWIM:  updateSwimming(cmd, dt); break;
		case MOVEMENT_STATE_CLIMB: updateClimbing(cmd, dt); break;
		default: assert(0 && "movement mode out of range");
	}

	if (velocity.x != 0.0f || velocity.y != 0.0f || velocity.z != 0.0f)
		vector3::addScaledVector(&collider->transform.position, &velocity, dt);

	rotate(collider, dt);
	evaluateWater();

	/* The ground modes are one ground: moving is walk, still is idle. */
	if (mode == MOVEMENT_STATE_IDLE || mode == MOVEMENT_STATE_WALK) {
		bool moving = velocity.x != 0.0f || velocity.y != 0.0f
		           || cmd->input.x != 0.0f || cmd->input.y != 0.0f;
		mode = moving ? MOVEMENT_STATE_WALK : MOVEMENT_STATE_IDLE;
	}
}

void Movement::setMode(uint8_t new_mode)
{
	assert(new_mode < MOVEMENT_STATE_COUNT);
	mode = new_mode;
}

bool Movement::isCoyoteOpen() const
{
	return !data.is_grounded && data.coyote_timer < settings.jump_coyote_time;
}

/* Unreal's Launch: the velocity is replaced and the body is in the air. */
void Movement::launch(const Vector3 *new_velocity)
{
	velocity = *new_velocity;
	data.is_grounded = false;
	data.coyote_timer = settings.jump_coyote_time;
	setMode(MOVEMENT_STATE_FALL);
}

/* Added to the velocity; the mode changes only if it lifts a grounded body. */
void Movement::addImpulse(const Vector3 *impulse)
{
	vector3::add(&velocity, impulse);
	data.coyote_timer = settings.jump_coyote_time;

	if ((mode == MOVEMENT_STATE_IDLE || mode == MOVEMENT_STATE_WALK) && velocity.z > 0.0f) {
		data.is_grounded = false;
		setMode(MOVEMENT_STATE_FALL);
	}
}

}


void Character3D::updateMovement(character3d::MovementCommand *cmd, float dt)
{
	movement.update(&collider, cmd, dt);
}

}
