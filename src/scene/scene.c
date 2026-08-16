#include <assert.h>
#include <malloc.h>

#include <t3d/t3dmath.h>

#include "shaders/mesh_deform.h"
#include "viewport/viewport.h"
#include "light/lighting.h"
#include "entity/entity.h"
#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "physics/world/physics_world.h"
#include "physics/shapes/physics_shape.h"
#include "physics/body/rigid_body.h"
#include "physics/physics_settings.h"
#include "physics/math/math_common.h"    /* RENDER_SCALE_INV */


static Scene scene;
static PhysicsWorld g_physics;

Scene        *scene_get(void)        { return &scene; }
PhysicsWorld *scene_getPhysics(void) { return &g_physics; }
PhysicsWorld *physics_getWorld(void) { return &g_physics; }

void scene_load(const SceneDef *def)
{
	light_initAmbient(def->light.ambient_color);
	for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; i++)
		light_initDirectional(&light_get()->directional[i], def->light.directional[i].direction, def->light.directional[i].color);
	for (int i = 0; i < POINT_LIGHT_COUNT; i++)
		light_initPoint(&light_get()->point[i], def->light.point[i].position, def->light.point[i].color, def->light.point[i].size);

	Camera *camera = &viewport_get()->camera;
	camera_reset(camera);
	switch (def->camera.type) {
		case CAMERA_TYPE_SPRING_ARM:
			cameraSpringArm_init(camera, &def->camera.spring_arm);
			break;
		case CAMERA_TYPE_NONE:
		case CAMERA_TYPE_COUNT:
			break;
	}

	assert(scene.entity_count == 0);
	scene = (Scene){0};

	Vector3 gravity = { 0.0f, 0.0f, -9.8f };
	physicsWorld_init(&g_physics, PHYSICS_TIMESTEP, gravity, PHYSICS_SOLVER_ITERATIONS);
	physicsWorld_setWind(&g_physics, def->wind);

	for (int i = 0; i < def->entity_count; i++) {
		const EntityDef *entity_def = &def->entity[i];
		Entity *entity = entity_create(entity_def);

		if (entity_def->collider)
			entity_attachPhysics(entity, entity_def, &g_physics);

		if (entity_def->cloth) {
			Cloth *cloth = physicsWorld_createCloth(&g_physics, entity_def->cloth);
			/* The cloth runs in metres, the vertex buffer in render units. */
			if (cloth) mesh_setDeform(entity->mesh, cloth->render_position, cloth->normal,
			                         cloth->particle_count, RENDER_SCALE);
		}

		if (entity_def->character) {
			assert(scene.character_count < SCENE_MAX_CHARACTERS);
			Character *character = character_create(entity_def->character, entity);
			scene.character[scene.character_count++] = character;

			characterPhysics_createBody(character, &g_physics);

			const CharacterWeaponsDef *weapons = entity_def->character->weapons_def;
			for (int slot = 0; weapons && slot < WEAPON_SLOT_COUNT; slot++)
				if (weapons->weapon[slot])
					character_equipWeapon(character, slot, weapons->weapon[slot]);
		}

		if (!entity_def->character) {
			for (int fb = 0; fb < FB_COUNT; fb++)
				mesh_setMatrix(entity->mesh, &entity->transform, fb);
		}

		scene.entity[scene.entity_count++] = entity;
	}

	for (int i = 0; i < def->sound_count; i++) {
		const SceneSoundDef *sound_def = &def->sound[i];
		scene.sound[scene.sound_count++] = sound_play(sound_def->id, &sound_def->position, 1.0f, 0.0f);
	}
}

void scene_clear(void)
{
	scene = (Scene){0};
}

void scene_unload(void)
{
	for (int i = 0; i < scene.sound_count; i++)
		sound_stop(scene.sound[i]);
	for (int i = 0; i < scene.character_count; i++)
		character_delete(scene.character[i]);
	for (int i = 0; i < scene.entity_count; i++)
		entity_delete(scene.entity[i]);
	scene_clear();
	physicsWorld_shutdown(&g_physics);
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

Character *scene_getCharacter(uint8_t index)
{
	if (index >= scene.character_count) return NULL;
	return scene.character[index];
}
