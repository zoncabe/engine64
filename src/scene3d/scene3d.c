#include <assert.h>
#include <malloc.h>

#include <t3d/t3dmath.h>

#include "shaders/mesh_deform.h"
#include "viewport/viewport.h"
#include "scene3d/lighting.h"
#include "scene3d/fog.h"
#include "entity/entity.h"
#include "scene3d/scene3d.h"
#include "physics/world/physics_world.h"
#include "physics/shapes/physics_shape.h"
#include "physics/body/rigid_body.h"
#include "physics/physics_settings.h"
#include "physics/math/math_common.h"    /* RENDER_SCALE_INV */


static Scene3D scene;
static PhysicsWorld g_physics;

/* The def whose placements hold the entity references, to clear on unload. */
static Scene3DDef *loaded_def;

Scene3D        *scene3d_get(void)        { return &scene; }
PhysicsWorld *scene3d_getPhysics(void) { return &g_physics; }
PhysicsWorld *physics_getWorld(void) { return &g_physics; }

void scene3d_load(Scene3DDef *def)
{
	/* A scene with no light def is left pitch black rather than lit by
	   something the game never asked for. */
	static const LightDef unlit;
	static const FogDef   clear;

	const LightDef *light = def->light ? def->light : &unlit;
	const FogDef   *fog   = def->fog   ? def->fog   : &clear;

	light_init(light);
	fog_init(fog);

	Camera *camera = &viewport_get()->camera;
	camera_reset(camera);
	if (def->camera) {
		camera->field_of_view = def->camera->field_of_view;
		camera->near_clipping = def->camera->near_clipping;
		camera->far_clipping  = def->camera->far_clipping;
	}
	switch (def->camera ? def->camera->type : CAMERA_TYPE_NONE) {
		case CAMERA_TYPE_SPRING_ARM:
			cameraSpringArm_init(camera, &def->camera->spring_arm);
			break;
		case CAMERA_TYPE_NONE:
		case CAMERA_TYPE_COUNT:
			break;
	}

	assert(scene.entity_count == 0);
	scene = (Scene3D){0};

	Vector3 gravity = { 0.0f, 0.0f, -9.8f };
	physicsWorld_init(&g_physics, PHYSICS_TIMESTEP, gravity, PHYSICS_SOLVER_ITERATIONS);
	physicsWorld_setWind(&g_physics, def->wind);

	for (int i = 0; i < def->prefab_count; i++) {
		Scene3DPrefab *placed = &def->prefab[i];
		const Prefab *prefab = placed->prefab;

		Vector3 scale = placed->scale;
		if (scale.x == 0.0f && scale.y == 0.0f && scale.z == 0.0f)
			scale = (Vector3){ 1.0f, 1.0f, 1.0f };

		/* entity.c builds from a flat parameter block: filled here straight
		   from the prefab and its placement, and gone after the load. */
		EntityDef entity_def = {
			.model_path = prefab->model,
			.position   = placed->position,
			.rotation   = placed->rotation,
			.scale      = scale,
			.collider   = prefab->collider,
			.cull       = true,
		};

		switch (prefab->type) {
			case PREFAB_CHARACTER:
				entity_def.character = prefab->character;
				break;
			case PREFAB_PROP:
				entity_def.body = prefab->prop;
				break;
			case PREFAB_CLOTH:
				entity_def.cloth = prefab->cloth;
				break;
			case PREFAB_WATER:
				entity_def.water = prefab->water;
				break;
		}

		Entity *entity = entity_create(&entity_def);

		if (entity_def.collider)
			entity_attachPhysics(entity, &entity_def, &g_physics);

		if (entity_def.cloth) {
			Cloth *cloth = physicsWorld_createCloth(&g_physics, entity_def.cloth);
			/* The cloth runs in metres, the vertex buffer in render units. */
			if (cloth) {
				cloth->culled = &entity->mesh->culled;
				mesh_setDeform(entity->mesh, cloth->render_position, cloth->normal,
				               NULL, cloth->particle_count, RENDER_SCALE);
			}
		}

		if (entity_def.water) {
			Water *water = water_create(entity_def.water);
			/* Same contract as the cloth: points in metres, buffer in render
			   units. The draw conf is what scrolls the texture layers, so it
			   only works through the per-frame material path. */
			if (water) {
				water->culled = &entity->mesh->culled;
				mesh_setDeform(entity->mesh, water->position, water->normal,
				               water->rgba, water->count, RENDER_SCALE);
				entity->mesh->draw_conf = &water->conf;

				/* The entity's collider is the water's sensor volume: bind
				   them and the bodies inside it start floating. */
				if (entity->body)
					water_bindPhysics(water, entity->body, &g_physics);
			}
		}

		if (entity_def.character) {
			assert(scene.character_count < SCENE_MAX_CHARACTERS);
			Character *character = character_create(entity_def.character, entity);
			scene.character[scene.character_count++] = character;

			characterPhysics_createBody(character, &g_physics);

			const CharacterWeaponsDef *weapons = entity_def.character->weapons_def;
			for (int slot = 0; weapons && slot < WEAPON_SLOT_COUNT; slot++)
				if (weapons->weapon[slot])
					character_equipWeapon(character, slot, weapons->weapon[slot]);
		}

		if (!entity_def.character) {
			for (int fb = 0; fb < FB_COUNT; fb++)
				mesh_setMatrix(entity->mesh, &entity->transform, fb);
		}

		placed->entity = entity;
		scene.entity[scene.entity_count++] = entity;
	}

	loaded_def = def;
	prefabSound_start(def);
}

void scene3d_clear(void)
{
	scene = (Scene3D){0};
}

void scene3d_unload(void)
{
	prefabSound_stop();
	if (loaded_def) {
		for (int i = 0; i < loaded_def->prefab_count; i++)
			loaded_def->prefab[i].entity = NULL;
		loaded_def = NULL;
	}
	for (int i = 0; i < scene.character_count; i++)
		character_delete(scene.character[i]);
	for (int i = 0; i < scene.entity_count; i++)
		entity_delete(scene.entity[i]);
	water_clear();
	scene3d_clear();
	physicsWorld_shutdown(&g_physics);
}

/* The characters' half of the frame after physics_update: each one collides
   against the world, hands the outcome to its body, and from there to what
   draws it. Always these four, always in this order, so no game writes them
   out. The list is the scene's, which is why it lives here. */
void scene3d_updateCharacters(uint8_t fb_index)
{
	for (int i = 0; i < scene.character_count; i++) {
		Character *character = scene.character[i];

		characterPhysics_collide(character, &g_physics);
		characterPhysics_syncBody(character);
		entity_setTransform(character->entity, &character->body);
		entity_setMatrix(character->entity, fb_index);
	}
}

void scene3d_addEntity(Entity *entity)
{
	assert(scene.entity_count < SCENE_MAX_ENTITIES);
	scene.entity[scene.entity_count++] = entity;
}

Character *scene3d_getCharacter(uint8_t index)
{
	if (index >= scene.character_count) return NULL;
	return scene.character[index];
}
