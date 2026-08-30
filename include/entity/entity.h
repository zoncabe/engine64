#ifndef ENTITY_H
#define ENTITY_H

#include "render/render.h"
#include "physics/body/rigid_body.h"
#include "physics/shapes/physics_shape.h"
#include "physics/cloth/cloth.h"
#include "shaders/water.h"
#include "graphics/mesh.h"
#include "character/character.h"

typedef struct Entity {

	RenderTransform transform;
	Mesh *mesh;

	/* Set when the entity has a rigid body. If that body is simulated, it is
	   what places the mesh each frame instead of the transform above. */
	RigidBody *body;

	/* Test the model's bounding box against the view frustum before drawing
	   it. Off means the entity is drawn every frame, no questions asked. */
	bool cull;

} Entity;


/* A collider is one or more primitives, each carrying its own offset in its
   .tx. The entity transform and scale apply to all of them, so the group
   stays consistent at any prop size. */
typedef struct EntityColliderDef {
	const PhysicsShapeDef *shape;
	uint8_t                count;
} EntityColliderDef;


typedef struct EntityDef {

	const char *model_path;
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
	const CharacterDef *character;
	const RigidBodyDef      *body;
	const EntityColliderDef *collider;
	const ClothDef          *cloth;
	const WaterDef          *water;
	bool cull;

} EntityDef;


struct PhysicsWorld;

void entity_init(Entity *entity, const EntityDef *def);
Entity *entity_create(const EntityDef *def);
void entity_delete(Entity *entity);
void entity_setTransform(Entity *entity, const KinematicBody *body);
void entity_setMatrix(Entity *entity, uint8_t fb_index);
void entity_setMatrixFromBody(Entity *entity, uint8_t fb_index);

/* Def → physics wiring. The caller owns the destinations. */
Transform entity_colliderTransform(const EntityDef *def);

/* Builds the entity's body and hangs its collider off it. Without a .body def
   the body comes out static, which is what scenery wants. */
RigidBody *entity_attachPhysics(Entity *entity, const EntityDef *def, struct PhysicsWorld *world);

#endif
