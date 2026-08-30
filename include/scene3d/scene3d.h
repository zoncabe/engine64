#ifndef SCENE3D_H
#define SCENE3D_H

#include "prefab/prefab.h"
#include "entity/entity.h"
#include "scene3d/lighting.h"
#include "scene3d/fog.h"
#include "camera/camera.h"
#include "camera/spring_arm.h"
#include "physics/shapes/physics_shape.h"
#include "physics/body/rigid_body.h"
#include "physics/collision/collision_mesh.h"
#include "sound/sound.h"
#include "sound/prefab_sound.h"

#define SCENE_MAX_CHARACTERS 6

#define SCENE_MAX_ENTITIES 64
/* Counts primitives, not entities: one compound collider takes several. */


/* One prefab placed in a scene. A scene's content is an array of these; the
   load walks it in order. A zero scale means identity. */
typedef struct Scene3DPrefab {

	const Prefab *prefab;
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;

	/* Written by the load: the instance this placement produced. NULL
	   while no scene is up. Whoever needs to go from a prefab to its
	   living entity reads it here. */
	Entity *entity;

} Scene3DPrefab;

typedef struct Scene3DDef {

	const LightDef *light;
	const FogDef *fog;
	const CameraDef *camera;
	Vector3 wind;

	Scene3DPrefab *prefab;
	uint8_t prefab_count;

} Scene3DDef;


typedef struct Scene3D {

	Entity *entity[SCENE_MAX_ENTITIES];
	uint8_t entity_count;

	Character *character[SCENE_MAX_CHARACTERS];
	uint8_t character_count;

} Scene3D;

Scene3D *scene3d_get(void);

/* The physics world the scene loaded its bodies and cloths into. */
PhysicsWorld *scene3d_getPhysics(void);

/* the load writes each placement's entity reference. */
void scene3d_load(Scene3DDef *def);
void scene3d_clear(void);
void scene3d_unload(void);
/* Collides every character and carries the result to what draws it. Call
   after physics_update, with the frame's buffer index. */
void scene3d_updateCharacters(uint8_t fb_index);

void scene3d_addEntity(Entity *entity);
Character *scene3d_getCharacter(uint8_t index);

#endif
