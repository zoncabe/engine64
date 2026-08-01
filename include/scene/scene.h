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
#define SCENE_MAX_STATIC_MESHES 8
#define SCENE_MAX_STATIC_BOXES 12
#define SCENE_MAX_STATIC_SPHERES 8
#define SCENE_MAX_STATIC_CAPSULES 8


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
	EntityDef entity[SCENE_MAX_ENTITIES];
	uint8_t entity_count;

} SceneDef;

typedef struct Scene {

	Entity *entity[SCENE_MAX_ENTITIES];
	uint8_t entity_count;

	Character *character[SCENE_MAX_CHARACTERS];
	uint8_t character_count;

	CollisionMesh *static_mesh[SCENE_MAX_STATIC_MESHES];
	Vector3 static_mesh_origin[SCENE_MAX_STATIC_MESHES];
	uint8_t static_mesh_count;

	Box static_box[SCENE_MAX_STATIC_BOXES];
	Transform static_box_transform[SCENE_MAX_STATIC_BOXES];
	uint8_t static_box_count;

	Sphere static_sphere[SCENE_MAX_STATIC_SPHERES];
	Transform static_sphere_transform[SCENE_MAX_STATIC_SPHERES];
	uint8_t static_sphere_count;

	Capsule static_capsule[SCENE_MAX_STATIC_CAPSULES];
	Transform static_capsule_transform[SCENE_MAX_STATIC_CAPSULES];
	uint8_t static_capsule_count;

} Scene;

typedef enum {

	SCENE_DEMO,
	SCENE_COUNT,

} SceneID;


Scene *scene_get(void);
void scene_load(const SceneDef *def);
void scene_clear(void);
void scene_unload(void);
void scene_addEntity(Entity *entity);
Character *scene_getCharacter(uint8_t index);

const SceneDef *scene_getDef(SceneID id);

#endif
