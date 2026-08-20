#include "assets/ball.h"

/* Color says density, density says how deep it floats: the submerged
   fraction at rest is density over the water's 1000. Green rides at 30%,
   yellow at 80%, red outweighs the water and goes to the bottom. Same
   convention as the crates. */
#define BALL_DENSITY_GREEN   300.0f
#define BALL_DENSITY_YELLOW  800.0f
#define BALL_DENSITY_RED    1200.0f

#define BALL_SHAPE(d) { .type = SHAPE_SPHERE, .sphere = { \
	.radius = 0.5f, \
	.friction = 0.4f, .restitution = 0.1f, .density = d }}

static const PhysicsShapeDef green_ball_shapes[]  = { BALL_SHAPE(BALL_DENSITY_GREEN) };
static const PhysicsShapeDef yellow_ball_shapes[] = { BALL_SHAPE(BALL_DENSITY_YELLOW) };
static const PhysicsShapeDef red_ball_shapes[]    = { BALL_SHAPE(BALL_DENSITY_RED) };

const EntityColliderDef green_ball_collider  = { green_ball_shapes,  1 };
const EntityColliderDef yellow_ball_collider = { yellow_ball_shapes, 1 };
const EntityColliderDef red_ball_collider    = { red_ball_shapes,    1 };

const RigidBodyDef ball_body = {
	.body_type     = BODY_DYNAMIC,
	.gravity_scale = 1.0f,
	.allow_sleep   = 1,
	.awake         = 1,
	.active        = 1,
};

/* The character's swim splashes, until the props get samples of their own. */
static const SoundID ball_splash[] = { SOUND_SWIM_SPLASH_1, SOUND_SWIM_SPLASH_2 };

/* The crate thuds double as the bounce, until the ball gets its own. */
static const SoundID ball_thud[] = { SOUND_THUD_1, SOUND_THUD_2,
                                     SOUND_THUD_3, SOUND_THUD_4 };

static const AssetSound ball_sound[] = {
	{ ASSET_SOUND_WATER_ENTRY, ball_splash, 2 },
	{ ASSET_SOUND_COLLISION,   ball_thud,   4 },
};

const Asset red_ball = {
	.type        = ASSET_PROP,
	.model       = red_ball_model,
	.sound       = ball_sound,
	.sound_count = 2,
	.prop        = { &red_ball_collider, &ball_body },
};

const Asset yellow_ball = {
	.type        = ASSET_PROP,
	.model       = yellow_ball_model,
	.sound       = ball_sound,
	.sound_count = 2,
	.prop        = { &yellow_ball_collider, &ball_body },
};

const Asset green_ball = {
	.type        = ASSET_PROP,
	.model       = green_ball_model,
	.sound       = ball_sound,
	.sound_count = 2,
	.prop        = { &green_ball_collider, &ball_body },
};
