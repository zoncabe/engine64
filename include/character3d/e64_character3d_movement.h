/*
	The movement component, after Unreal's CharacterMovementComponent: the
	game hands in a direction per frame and the component turns it into a
	velocity with acceleration, friction and braking, up to a max speed per
	mode. The settings are plain writable fields, so the game changes a max
	speed whenever it wants and it stays. Velocity is a public field too, and
	launch and impulse write it outright, for anything the model does not
	cover: a dash, a knockback, a gait table built outside.
*/
#ifndef ENGINE64_CHARACTER3D_MOVEMENT_H
#define ENGINE64_CHARACTER3D_MOVEMENT_H

#include <stdbool.h>
#include <stdint.h>

#include "math/e64_vector2.h"
#include "math/e64_vector3.h"
#include "character3d/e64_character3d_collision.h"

namespace e64 {

namespace character3d {

/* Below this, a horizontal velocity with nothing else moving the body is
   zeroed: the exponential approach never reaches zero on its own. Unreal's
   BRAKE_TO_STOP_VELOCITY. */
#define E64_CHARACTER3D_BRAKE_TO_STOP_SPEED 0.05f

/* Degrees. Closer than this to the heading, the body is set on it outright
   instead of easing the last sliver forever. */
#define E64_CHARACTER3D_ROTATION_SNAP_THRESHOLD 1.0f


enum MovementState {
	MOVEMENT_STATE_IDLE,
	MOVEMENT_STATE_WALK,
	MOVEMENT_STATE_FALL,
	MOVEMENT_STATE_SWIM,
	MOVEMENT_STATE_CLIMB,
	MOVEMENT_STATE_COUNT
};


struct MovementCommand {
	/* Where the body is asked to go, in the world's horizontal plane. Its
	   length, 0 to 1, is how hard: it scales the max speed and the
	   acceleration both, so half a stick walks at half speed. Zero brakes. */
	Vector2 input;

	bool jump_held;
	bool jump_triggered;
};

struct MovementSettings {

	/* Unreal's names, flat as on its component. Metres and seconds; the
	   rotation in degrees per second.

	   With input, friction kills the part of the velocity not along it — how
	   fast a turn sheds the old heading — and max_acceleration adds along it
	   up to the mode's max speed. Without input, friction and the mode's
	   braking deceleration take the velocity to zero: friction as a share of
	   it per second, braking as a fixed amount. */

	/* One acceleration and one turn rate for every mode; the game writes them
	   on a mode change if a mode wants its own. */
	float max_acceleration;
	float rotation_rate; /* how fast the body turns to face its velocity */

	float max_walk_speed;
	float ground_friction;
	float braking_deceleration_walking;

	/* The air. air_control scales max_acceleration: zero keeps the heading
	   and speed the body left the floor with until it lands. */
	float air_control;
	float falling_lateral_friction;
	float braking_deceleration_falling;

	/* Swimming: the fluid friction decays the velocity every frame, input or
	   not, so a stroke never coasts. */
	float max_swim_speed;
	float fluid_friction;
	float braking_deceleration_swimming;

	/* Climbing, own: Unreal has no such mode. Only the vertical moves. */
	float max_climb_speed;
	float braking_deceleration_climbing;

	/* Metres per second squared, negative. The acceleration the air applies,
	   and the terminal vertical speed the fall is clamped to. */
	float gravity;
	float fall_max_speed;

	/* The vertical speed the body leaves the floor with. */
	float jump_speed;

	/* The fraction of gravity the rise pays while the button is held. Lower
	   climbs higher; at 1.0 holding does nothing. It only ever applies on the
	   way up, so nobody floats down. */
	float jump_hold_gravity_scale;

	/* How long the jump still answers after the floor is gone. Walking off a
	   ledge the body falls as always, and the button launches the whole way
	   through this window. */
	float jump_coyote_time;

	/* Fake buoyancy: submerged fraction of the capsule where the scaled
	   gravity flips sign. Treading water holds one depth, stroking another,
	   and speed slides the target between the two so the swim rides up to
	   the surface.

	   The swim one doubles as the swim state threshold: sunk past it the
	   water carries the body, so that is where the swim begins, and exit
	   trails it by water_swim_exit_scale. */
	float water_equilibrium_idle;
	float water_equilibrium_swim;

	/* Swim entry is the character's own swim equilibrium: sunk past the depth
	   its buoyancy holds it at, the water is already carrying it, so it swims.
	   A fixed threshold could sit above that depth, and then a body floating
	   at its equilibrium never reaches it — it stays falling on the surface.

	   Exit trails entry by this factor, the hysteresis that keeps the waves
	   from flickering the state at the boundary. Exit also needs footing. */
	float water_swim_exit_scale;

	/* Progressive sink limit: none at the splash so a dive keeps its momentum,
	   the max speed once submerged past the limit fraction. */
	float water_sink_max_speed;
	float water_sink_limit_full;

};

struct MovementData {
	/* Time since the floor was lost, counted only while the coyote window is
	   still open. Reset on every landing. */
	float coyote_timer;
	bool is_grounded;
	/* Straight down from the capsule's bottom, written by the collision pass.
	   Negative with no floor within reach. */
	float floor_distance;

	bool in_water;
	float submerged_fraction; /* 0..1 of the capsule under the surface */
};


/* The movement component, laid out as Unreal's: it owns the velocity and the
   acceleration, and moves the collider it is handed, whose transform is where
   the character stands and where it faces. An aggregate on purpose: no constructor,
   public data, built by character3d::create with a designated initializer.
   The settings are the character's own copy, taken from the def on create:
   the game writes them at will and the change persists. */
class Movement {
public:

	MovementSettings settings;
	MovementData data;

	Vector3 velocity;
	Vector3 acceleration;

	uint8_t mode; /* MovementState */

	void update(Collider *collider, MovementCommand *cmd, float dt);
	void setMode(uint8_t new_mode);

	/* Still inside the coyote window: off the floor, but not for long enough
	   that the jump has stopped answering. */
	bool isCoyoteOpen() const;

	/* Velocity written outright, outside the model. launch replaces it and
	   puts the body in the air, the way Unreal's Launch does: a dash, a bounce
	   pad, a knockback. addImpulse adds to it and leaves the state alone
	   unless it lifts a grounded body. Both close the coyote window. */
	void launch(const Vector3 *velocity);
	void addImpulse(const Vector3 *impulse);

private:

	/* The velocity model, Unreal's CalcVelocity and ApplyVelocityBraking, on
	   the horizontal plane. */
	void calcVelocity(const Vector2 *input, float max_speed, float friction,
	                  float braking_deceleration, float acceleration_scale, bool fluid, float dt);
	void applyBraking(float friction, float braking_deceleration, float dt);

	/* The vertical, one per environment. */
	void applyGravity(float gravity_scale, float dt);
	void applyBuoyancy(float dt);

	void jump(MovementCommand *cmd);

	/* One per mode, called by update on the current one. */
	void updateWalking(MovementCommand *cmd, float dt);
	void updateFalling(MovementCommand *cmd, float dt);
	void updateSwimming(MovementCommand *cmd, float dt);
	void updateClimbing(MovementCommand *cmd, float dt);

	void evaluateWater();
	void rotate(Collider *collider, float dt);

};

}

}

#endif
