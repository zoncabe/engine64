/*
	Two volumes over the same rungs, both rooted at the model's foot.

	The solid one is what the body walks into, grown well past the mesh: the
	rungs are 5 deep and nothing that thin can be pushed against, and the
	character's own capsule is 70 across, so it would sit on the box's edge
	and slide off.

	The climbable one is the reach around the rungs. Its top is where the
	climb ends, so it stops at the top rung; its depth has to reach past
	where the solid box stops the body, or the climb can never be caught.
	It starts below the foot, so standing at the bottom is already inside.
*/
#include "prefab/e64_prefab.h"


static const PhysicsShapeDef ladder_shapes[] = {
	{ .type = SHAPE_BOX, .box = {
		.tx = { .position = { 0.0f, 0.0f, 2.5f } },
		.e  = { 0.30f, 0.20f, 2.5f },
	}},
	{ .type = SHAPE_BOX, .box = {
		.tx     = { .position = { 0.0f, 0.0f, 2.3f } },
		.e      = { 0.30f, 0.65f, 2.7f },
		.sensor = SENSOR_CLIMBABLE,
	}},
};

static const EntityColliderDef ladder_collider = { ladder_shapes, 2 };

const Prefab ladder = {

	.type     = PREFAB_PROP,
	.model    = "rom:/models/ladder.t3dm",
	.collider = &ladder_collider,
};
