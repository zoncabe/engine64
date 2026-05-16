#ifndef ENTITY_H
#define ENTITY_H

#include "render/render.h"
#include "physics/body/rigid_body.h"
#include "physics/shapes/box.h"
#include "physics/shapes/sphere.h"
#include "physics/shapes/capsule.h"
#include "physics/shapes/physics_shape.h"
#include "graphics/mesh.h"
#include "actor/actor.h"

typedef enum {

	ENTITY_ACTOR,
	ENTITY_SCENERY,

} EntityType;

typedef struct Entity {

	EntityType type;
	RenderTransform transform;
	Mesh *mesh;
	Actor *actor;
	RigidBody *body;

} Entity;


typedef struct EntityShapeDef {
	ShapeType type;
	union {
		BoxDef     box;
		SphereDef  sphere;
		CapsuleDef capsule;
	};
} EntityShapeDef;


typedef struct EntityDef {

	const char *model_path;
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
	const ActorDef *actor;
	const RigidBodyDef   *body;
	const EntityShapeDef *shape;

} EntityDef;


void entity_init(Entity *entity, const EntityDef *def);
Entity *entity_create(const EntityDef *def);
void entity_delete(Entity *entity);
void entity_syncTransformToBody(Entity *entity);
void entity_setMatrix(Entity *entity, uint8_t fb_index);

#endif
