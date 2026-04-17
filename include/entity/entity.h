#ifndef ENTITY_H
#define ENTITY_H

#include "physics/math/transform.h"
#include "physics/body/rigid_body.h"
#include "graphics/mesh.h"
#include "actor/actor.h"

typedef enum {

	ENTITY_ACTOR,
	ENTITY_SCENERY,

} EntityType;

typedef struct Entity {

	EntityType type;
	Transform transform;
	Mesh *mesh;
	Actor *actor;

} Entity;


void entity_init(Entity *entity, EntityType type, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings);
Entity *entity_create(EntityType type, const char *model_path, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings, const AnimationDef *animation_def);
void entity_delete(Entity *entity);

#endif
