/*
	The room: floor, walls, the raised platform and the mound. Its collision is
	the triangle mesh the importer builds from the same .glb, so what you see
	is what the character walks on.
*/
#include "prefab/e64_prefab.h"


static const PhysicsShapeDef room_shapes[] = {
	{ .type = SHAPE_MESH, .mesh = {
		.path        = "rom:/collision/room.collision",
		.friction    = 0.9f,
		.restitution = 0.1f,
	}},
};

static const EntityColliderDef room_collider = { room_shapes, 1 };

/* No body: a prop without one is static, which is what a room is. */
const Prefab room = {

	.type     = PREFAB_PROP,
	.model    = "rom:/models/room.t3dm",
	.collider = &room_collider,
};
