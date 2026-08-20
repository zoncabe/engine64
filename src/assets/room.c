#include "assets/room.h"

static const PhysicsShapeDef room_shapes[] = {
	{ .type = SHAPE_MESH, .mesh = {
		.path        = "rom:/collision/room.collision",
		.friction    = 0.9f,
		.restitution = 0.1f,
	}},
};

const EntityColliderDef room_collider = { room_shapes, 1 };

const Asset room = {
	.type    = ASSET_SCENERY,
	.model   = room_model,
	.scenery = { &room_collider },
};
