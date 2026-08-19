#ifndef SCENE_H
#define SCENE_H

#include "entity/entity.h"
#include "scene/lighting.h"
#include "scene/fog.h"
#include "camera/camera.h"
#include "camera/spring_arm.h"
#include "physics/shapes/physics_shape.h"
#include "physics/body/rigid_body.h"
#include "physics/collision/collision_mesh.h"
#include "sound/sound.h"

#define SCENE_MAX_CHARACTERS 6
#define SCENE_MAX_SCENERY 12
#define SCENE_MAX_SOUNDS 8

#define SCENE_MAX_ENTITIES 32
/* Counts primitives, not entities: one compound collider takes several. */


/* An ambience of the scene itself: it plays from a fixed point in the world
   for as long as the scene is loaded. */
typedef struct {

	SoundID id;
	Vector3 position;

} SceneSoundDef;

typedef struct {

	LightDef light;
	FogDef fog;
	CameraDef camera;
	Vector3 wind;
	EntityDef entity[SCENE_MAX_ENTITIES];
	uint8_t entity_count;
	SceneSoundDef sound[SCENE_MAX_SOUNDS];
	uint8_t sound_count;

} SceneDef;

typedef struct Scene {

	Entity *entity[SCENE_MAX_ENTITIES];
	uint8_t entity_count;

	Character *character[SCENE_MAX_CHARACTERS];
	uint8_t character_count;

	SoundEmitter sound[SCENE_MAX_SOUNDS];
	uint8_t sound_count;

} Scene;

typedef enum {

	SCENE_DEMO,
	SCENE_COUNT,

} SceneID;


Scene *scene_get(void);

/* The physics world the scene loaded its bodies and cloths into. */
PhysicsWorld *scene_getPhysics(void);
void scene_load(const SceneDef *def);
void scene_clear(void);
void scene_unload(void);
void scene_addEntity(Entity *entity);
Character *scene_getCharacter(uint8_t index);

const SceneDef *scene_getDef(SceneID id);

#endif
