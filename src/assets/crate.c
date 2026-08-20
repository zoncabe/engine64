#include "assets/crate.h"

/* Color says density, density says how deep it floats: the submerged
   fraction at rest is density over the water's 1000. Green rides at 30%,
   yellow at 80%, red outweighs the water and goes to the bottom. Same
   convention as the balls. */
#define CRATE_DENSITY_GREEN   300.0f
#define CRATE_DENSITY_YELLOW  800.0f
#define CRATE_DENSITY_RED    1200.0f

#define CRATE_SHAPE(d) { .type = SHAPE_BOX, .box = { \
	.e = { 0.5f, 0.5f, 0.5f }, \
	.friction = 0.4f, .restitution = 0.1f, .density = d }}

static const PhysicsShapeDef green_crate_shapes[]  = { CRATE_SHAPE(CRATE_DENSITY_GREEN) };
static const PhysicsShapeDef yellow_crate_shapes[] = { CRATE_SHAPE(CRATE_DENSITY_YELLOW) };
static const PhysicsShapeDef red_crate_shapes[]    = { CRATE_SHAPE(CRATE_DENSITY_RED) };

const EntityColliderDef green_crate_collider  = { green_crate_shapes,  1 };
const EntityColliderDef yellow_crate_collider = { yellow_crate_shapes, 1 };
const EntityColliderDef red_crate_collider    = { red_crate_shapes,    1 };

const RigidBodyDef crate_body = {
	.body_type     = BODY_DYNAMIC,
	.gravity_scale = 1.0f,
	.allow_sleep   = 1,
	.awake         = 1,
	.active        = 1,
};

/* The character's swim splashes, until the props get samples of their own. */
static const SoundID crate_splash[] = { SOUND_SWIM_SPLASH_1, SOUND_SWIM_SPLASH_2 };

static const SoundID crate_thud[] = { SOUND_THUD_1, SOUND_THUD_2,
                                      SOUND_THUD_3, SOUND_THUD_4 };

static const AssetSound crate_sound[] = {
	{ ASSET_SOUND_WATER_ENTRY, crate_splash, 2 },
	{ ASSET_SOUND_COLLISION,   crate_thud,   4 },
};

const Asset green_crate = {
	.type        = ASSET_PROP,
	.model       = green_crate_model,
	.sound       = crate_sound,
	.sound_count = 2,
	.prop        = { &green_crate_collider, &crate_body },
};

const Asset yellow_crate = {
	.type        = ASSET_PROP,
	.model       = yellow_crate_model,
	.sound       = crate_sound,
	.sound_count = 2,
	.prop        = { &yellow_crate_collider, &crate_body },
};

const Asset red_crate = {
	.type        = ASSET_PROP,
	.model       = red_crate_model,
	.sound       = crate_sound,
	.sound_count = 2,
	.prop        = { &red_crate_collider, &crate_body },
};
