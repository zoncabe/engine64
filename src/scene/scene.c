#include <assert.h>

#include "viewport/viewport.h"
#include "light/lighting.h"
#include "entity/entity.h"
#include "scene/scene.h"
#include "scene/demo_scene.h"


static Scene scene;

Scene *scene_get(void) { return &scene; }


void scene_load(const SceneDef *def)
{
	light_initAmbient(def->light.ambient_color);
	for (uint8_t i = 0; i < DIRECTIONAL_LIGHT_COUNT; i++)
		light_initDirectional(&light_get()->directional[i], def->light.directional[i].direction, def->light.directional[i].color);
	for (uint8_t i = 0; i < POINT_LIGHT_COUNT; i++)
		light_initPoint(&light_get()->point[i], def->light.point[i].position, def->light.point[i].color, def->light.point[i].size);

	Camera *camera = &viewport_get()->camera;
	camera_reset(camera);
	switch (def->camera.type) {
		case CAMERA_TYPE_SPHERICAL:
			cameraSpherical_init(camera, &def->camera.spherical);
			break;
		case CAMERA_TYPE_NONE:
		case CAMERA_TYPE_COUNT:
			break;
	}

	assert(scene.entity_count == 0);
	scene = (Scene){0};

	for (uint8_t i = 0; i < def->actor_count; i++) {
		const ActorDef *actor = &def->actor[i];
		Entity *entity = entity_create(ENTITY_ACTOR, actor->model_path, actor->motion_settings, actor->animation_settings, actor->animation_def);
		entity->transform.position = actor->position;
		entity->transform.rotation = actor->rotation;
		entity->transform.scale    = actor->scale;
		scene.entity[scene.entity_count++] = entity;
	}

	for (uint8_t i = 0; i < def->scenery_count; i++) {
		const SceneryDef *scenery = &def->scenery[i];
		Entity *entity = entity_create(ENTITY_SCENERY, scenery->model_path, NULL, NULL, NULL);
		entity->transform.position = scenery->position;
		entity->transform.rotation = scenery->rotation;
		entity->transform.scale    = scenery->scale;
		for (uint8_t fb = 0; fb < FB_COUNT; fb++)
			mesh_setMatrix(entity->mesh, &entity->transform, fb);
		scene.entity[scene.entity_count++] = entity;
	}
}


void scene_clear(void)
{
	scene = (Scene){0};
}

void scene_unload(void)
{
	for (uint8_t i = 0; i < scene.entity_count; i++)
		entity_delete(scene.entity[i]);
	scene_clear();
}

const SceneDef *scene_getDef(SceneID id)
{
	static const SceneDef * const table[SCENE_COUNT] = {
		[SCENE_DEMO] = &demo_scene,
	};
	return table[id];
}

void scene_addEntity(Entity *entity)
{
	assert(scene.entity_count < SCENE_MAX_ENTITIES);
	scene.entity[scene.entity_count++] = entity;
}

Entity *scene_getActor(uint8_t index)
{
	uint8_t found = 0;
	for (uint8_t i = 0; i < scene.entity_count; i++) {
		if (scene.entity[i]->type != ENTITY_ACTOR) continue;
		if (found == index) return scene.entity[i];
		found++;
	}
	return NULL;
}
