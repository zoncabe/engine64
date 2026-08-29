#ifndef SCENE3D_H
#define SCENE3D_H

#include "entity/entity.h"
#include "scene3d/lighting.h"
#include "scene3d/fog.h"
#include "camera/camera.h"
#include "camera/spring_arm.h"
#include "physics/shapes/physics_shape.h"
#include "physics/body/rigid_body.h"
#include "physics/collision/collision_mesh.h"
#include "sound/sound.h"
#include "sound/asset_sound.h"

#define SCENE_MAX_CHARACTERS 6
#define SCENE_MAX_SCENERY 12

#define SCENE_MAX_ENTITIES 64
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
typedef struct Scene3DAsset {

	const Asset *asset;
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;

	/* Written by the load: the instance this placement produced. NULL
	   while no scene is up. Whoever needs to go from an asset to its
	   living entity reads it here. */
	Entity *entity;

} Scene3DAsset;

typedef struct Scene3DDef {

	const LightDef *light;
	const FogDef *fog;
	const CameraDef *camera;
	Vector3 wind;

	Scene3DAsset *asset;
	uint8_t asset_count;

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
void scene3d_addEntity(Entity *entity);
Character *scene3d_getCharacter(uint8_t index);

#endif
