#include "assets/pool.h"

/* The pool as a volume: footprint of the water plane, from the basin floor
   up to the resting surface. Sensor, so it reports who is inside without
   colliding; buoyancy reads its overlaps once the water binds to it. */
static const PhysicsShapeDef pool_shapes[] = {
	{ .type = SHAPE_BOX, .box = {
		.tx     = { .position = { -5.0f, 12.5f, -1.45f } },
		.e      = { 15.0f, 7.5f, 0.975f },
		.sensor = 1,
	}},
};

const EntityColliderDef pool_collider = { pool_shapes, 1 };

const WaterDef pool_water = {
	.mesh_path = "rom:/collision/water.collision",
	.wave = {
		{ .direction_x =  1.0f, .direction_y =  0.3f, .amplitude = 0.05f, .frequency = 1.6f, .speed = 1.6f },
		{ .direction_x = -0.4f, .direction_y =  1.0f, .amplitude = 0.03f, .frequency = 2.9f, .speed = 2.3f },
		{ .direction_x =  0.6f, .direction_y = -1.0f, .amplitude = 0.02f, .frequency = 4.3f, .speed = 3.1f },
	},
	.wave_count = 3,
	.scroll_a = {  1.5f, 2.4f },
	.scroll_b = { -2.0f, 1.0f },
	.wrap_a = 64.0f,
	.wrap_b = 64.0f,
	.color  = { 110, 180, 215 },
};

const Asset pool = {
	.type  = ASSET_WATER,
	.model = pool_model,
	.water = { &pool_water, &pool_collider },
};
