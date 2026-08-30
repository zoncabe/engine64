/*
	The same capsule the character wears, standing still. Its shape is written
	here by hand because a prop knows nothing about character settings: radius
	0.35 and 1.80 tall, so the segment between the two caps is 0.55 either side
	of the middle.
*/
#include "prefab/prefab.h"


static const PhysicsShapeDef capsule_shapes[] = {
	{ .type = SHAPE_CAPSULE, .capsule = {
		.tx          = { .position = { 0.0f, 0.0f, 0.90f } },
		.radius      = 0.35f,
		.half_height = 0.55f,
		.friction    = 0.8f,
		.restitution = 0.1f,
	}},
};

static const EntityColliderDef capsule_collider = { capsule_shapes, 1 };

const Prefab capsule = {

	.type     = PREFAB_PROP,
	.model    = "rom:/models/capsule.t3dm",
	.collider = &capsule_collider,
};
