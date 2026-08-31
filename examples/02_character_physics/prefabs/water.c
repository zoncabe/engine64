/*
	The pool that fills the sunken quad. Two halves that have to agree: the
	sensor is the volume buoyancy reads, from the basin floor up to the
	resting surface, and the WaterDef is the surface itself, waves included.

	The mesh the waves run on is the welded collision mesh of the same plane,
	not the model: the model is what gets drawn, deformed from those points.
*/
#include "prefab/e64_prefab.h"


static const PhysicsShapeDef water_shapes[] = {
	{ .type = SHAPE_BOX, .box = {
		.tx     = { .position = { 0.0f, 0.0f, -1.25f } },
		.e      = { 5.0f, 5.0f, 1.25f },
		.sensor = SENSOR_VOLUME,
	}},
};

static const EntityColliderDef water_collider = { water_shapes, 1 };

/* Three sines with unrelated frequencies: one alone reads as a machine. The
   two texture layers scroll against each other, which is what sells the
   caustics. Density and drag left at zero take the defaults. */
static const WaterDef water_surface = {

	.mesh_path = "rom:/collision/water.collision",

	.wave = {
		{ .direction_x =  1.0f, .direction_y =  0.3f, .amplitude = 0.05f, .frequency = 1.6f, .speed = 1.6f },
		{ .direction_x = -0.4f, .direction_y =  1.0f, .amplitude = 0.03f, .frequency = 2.9f, .speed = 2.3f },
		{ .direction_x =  0.6f, .direction_y = -1.0f, .amplitude = 0.02f, .frequency = 4.3f, .speed = 3.1f },
	},
	.wave_count = 3,

	.scroll_a = {  1.5f, 2.4f },
	.scroll_b = { -2.0f, 1.0f },
	.wrap_a   = 64.0f,
	.wrap_b   = 64.0f,

	.color = { 89, 166, 204 },
};

const Prefab water = {

	.type     = PREFAB_WATER,
	.model    = "rom:/models/water.t3dm",
	.collider = &water_collider,
	.water    = &water_surface,
};
