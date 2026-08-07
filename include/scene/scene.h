#ifndef SCENE_H
#define SCENE_H

#include "entity/entity.h"
#include "light/lighting.h"
#include "camera/camera.h"
#include "camera/spherical.h"
#include "physics/shapes/physics_shape.h"
#include "physics/body/rigid_body.h"
#include "physics/collision/collision_mesh.h"

#define SCENE_MAX_CHARACTERS 6
#define SCENE_MAX_SCENERY 12

#define SCENE_MAX_ENTITIES 32
/* Counts primitives, not entities: one compound collider takes several. */


typedef struct {

	color_t ambient_color;
	DirectionalLight directional[DIRECTIONAL_LIGHT_COUNT];
	PointLight point[POINT_LIGHT_COUNT];

} LightDef;

typedef struct {

	CameraType type;
	union {
		CameraSphericalData spherical;
	};

} CameraDef;

typedef struct {

	LightDef light;
	CameraDef camera;
	Vector3 wind;
	EntityDef entity[SCENE_MAX_ENTITIES];
	uint8_t entity_count;

} SceneDef;

typedef struct Scene {

	Entity *entity[SCENE_MAX_ENTITIES];
	uint8_t entity_count;

	Character *character[SCENE_MAX_CHARACTERS];
	uint8_t character_count;

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
