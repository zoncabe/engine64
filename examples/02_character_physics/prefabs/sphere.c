/*
	A one metre ball. Rounded, so the character slides off it instead of
	standing on top: the counterpart of the cube's flat faces.
*/
#include "prefab/prefab.h"


static const PhysicsShapeDef sphere_shapes[] = {
	{ .type = SHAPE_SPHERE, .sphere = {
		.radius      = 0.5f,
		.friction    = 0.8f,
		.restitution = 0.1f,
	}},
};

static const EntityColliderDef sphere_collider = { sphere_shapes, 1 };

const Prefab sphere = {

	.type     = PREFAB_PROP,
	.model    = "rom:/models/sphere.t3dm",
	.collider = &sphere_collider,
};
