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
#include "sound/asset_sound.h"

#define SCENE_MAX_CHARACTERS 6
#define SCENE_MAX_SCENERY 12

#define SCENE_MAX_ENTITIES 32
/* Counts primitives, not entities: one compound collider takes several. */


/* What an asset is to the scene: its model plus the pieces its kind needs,
   behind a tag — same scheme as PhysicsShapeDef. Placement is per instance:
   position, rotation and scale come through scene_addAsset. */
typedef enum {

	ASSET_CHARACTER,
	ASSET_SCENERY,
	ASSET_PROP,
	ASSET_CLOTH,
	ASSET_WATER,

} AssetType;

typedef struct Asset {

	AssetType type;
	const char *model;

	const AssetSound *sound;
	uint8_t sound_count;

	union {
		struct { const CharacterDef *def; } character;

		/* Static geometry; a NULL collider is pure dressing. */
		struct { const EntityColliderDef *collider; } scenery;

		/* Simulated: the solver places it from then on. */
		struct { const EntityColliderDef *collider;
		         const RigidBodyDef      *body; } prop;

		struct { const ClothDef *def; } cloth;

		/* The collider is the sensor volume buoyancy reads. */
		struct { const WaterDef          *def;
		         const EntityColliderDef *sensor; } water;
	};

} Asset;

/* One asset placed in a scene. A scene's content is an array of these; the
   load walks it in order. A zero scale means identity. */
typedef struct SceneAsset {

	const Asset *asset;
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;

	/* Written by the load: the instance this placement produced. NULL
	   while no scene is up. Whoever needs to go from an asset to its
	   living entity reads it here. */
	Entity *entity;

} SceneAsset;

typedef struct SceneDef {

	LightDef light;
	FogDef fog;
	CameraDef camera;
	Vector3 wind;

	SceneAsset *asset;
	uint8_t asset_count;

} SceneDef;


typedef struct Scene {

	Entity *entity[SCENE_MAX_ENTITIES];
	uint8_t entity_count;

	Character *character[SCENE_MAX_CHARACTERS];
	uint8_t character_count;

} Scene;

Scene *scene_get(void);

/* The physics world the scene loaded its bodies and cloths into. */
PhysicsWorld *scene_getPhysics(void);

/* the load writes each placement's entity reference. */
void scene_load(SceneDef *def);
void scene_clear(void);
void scene_unload(void);
void scene_addEntity(Entity *entity);
Character *scene_getCharacter(uint8_t index);

#endif
