/*
	Everything the body runs on. Physics units are metres, so the capsule that
	stands 180 units tall on screen is 1.8 here.

	The engine brings no movement of its own: a character is these settings and
	nothing else. What this def leaves out, the animation graph, the sounds,
	the weapons, the aim bend and the spring bones, is skipped entirely.
*/
#include "prefab/prefab.h"


static const CharacterColliderSettings collider = {

	.radius = 0.35f,
	.height = 1.80f,
};

/* Gaits run slowest to fastest. Three of them: the stick alone picks the
   middle one and sprint reaches the top, which is the one that tires the
   body out. */
static const CharacterGaitSettings gait[] = {

	{ .target_speed = 4.0f, .response_rate =  8.0f, .rotation_response_rate = 12.0f },
	{ .target_speed = 6.5f, .response_rate =  9.0f, .rotation_response_rate = 10.0f },
	{ .target_speed = 8.5f, .response_rate = 10.0f, .rotation_response_rate =  8.0f },
};

static const CharacterMovementSettings movement = {

	.idle_target_speed           =  0.0f,
	.idle_response_rate          = 12.0f,
	.idle_rotation_response_rate = 10.0f,

	.gait       = gait,
	.gait_count = sizeof(gait) / sizeof(gait[0]),

	/* Snap: the body leaves the floor on the press and, while A stays down,
	   the rise pays less gravity. A tap reaches a metre, holding all the way
	   up reaches two and a half, and the rise ends itself either way. */
	.jump_mode               = JUMP_SNAP,
	.jump_response_rate      = 10.0f,
	.jump_base_speed         = 9.0f,
	.jump_hold_gravity_scale = 0.6f,

	/* Half the steering of the ground: the jump keeps most of the run that
	   launched it, but the stick still counts for something. */
	.air_control = 0.8f,

	/* Only read under charge, the other mode: how long the crouch lasts and
	   what it turns into. */
	.jump_force_multiplier = 30.0f,
	.jump_timer_max        = 0.25f,

	.swim_slow_speed    = 2.0f,
	.swim_fast_speed    = 3.0f,
	.swim_response_rate = 4.0f,

	/* Vertical speed on the ladder. Left at zero the body grabs the rungs
	   and stays there, which is what an unfilled climb looks like. */
	.climb_speed         = 2.5f,
	.climb_response_rate = 8.0f,

	/* How deep the capsule floats: idle keeps the head out, swimming rides
	   a little lower. */
	.water_equilibrium_idle = 0.70f,
	.water_equilibrium_swim = 0.70f,
};

/* Stamina is read on every update, so it needs settings even where nothing
   spends it. A drain of zero never empties: the body sprints forever, which
   is what an example with no gauge on screen wants. */
static const CharacterStatsSettings stats = {

	.stamina_drain_rate = 0.0f,
	.stamina_regen_rate = 0.35f,
	.tired_speed_scale  = 0.60f,
};


static const CharacterDef example_character = {

	.collider_settings = &collider,
	.movement_settings = &movement,
	.stats_settings    = &stats,
};

/* No collider of its own: the character builds its capsule from the radius
   and height above, and the scene gives it a body from that. */
const Prefab character = {

	.type      = PREFAB_CHARACTER,
	.model     = "rom:/models/capsule.t3dm",
	.character = &example_character,
};
