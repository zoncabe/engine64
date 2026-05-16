#include <assert.h>

#include "viewport/viewport.h"
#include "light/lighting.h"
#include "entity/entity.h"
#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "physics/world/physics_scene.h"
#include "physics/shapes/physics_shape.h"
#include "physics/body/rigid_body.h"
#include "physics/physics_settings.h"
#include "physics/math/math_common.h"    /* RENDER_SCALE_INV */


static Scene scene;
static PhysicsScene g_physics;

Scene *scene_get(void) { return &scene; }
PhysicsScene *physics_getScene(void) { return &g_physics; }


/* Copies the body-def override bits on top of the freshly-initialised body
   (position, orientation) and then attaches the entity's shape to it. */
static void entity_attachPhysics(Entity *entity, const EntityDef *def)
{
	RigidBodyDef body_def;
	rigidBodyDef_init(&body_def);

	if (def->body) {
		/* The user-supplied RigidBodyDef describes only the body properties,
		   not the world position. Position/rotation come from the entity. */
		body_def.body_type       = def->body->body_type;
		body_def.gravity_scale   = def->body->gravity_scale;
		body_def.layers          = def->body->layers ? def->body->layers : 1;
		body_def.linear_damping  = def->body->linear_damping;
		body_def.angular_damping = def->body->angular_damping;
		body_def.allow_sleep     = def->body->allow_sleep;
		body_def.awake           = def->body->awake;
		body_def.active          = def->body->active;
		body_def.lock_axis_x     = def->body->lock_axis_x;
		body_def.lock_axis_y     = def->body->lock_axis_y;
		body_def.lock_axis_z     = def->body->lock_axis_z;
	}

	/* Entity positions are in render-space; physics runs in metres. */
	body_def.position = (Vector3){
		def->position.x * RENDER_SCALE_INV,
		def->position.y * RENDER_SCALE_INV,
		def->position.z * RENDER_SCALE_INV,
	};
	body_def.axis  = (Vector3){ 0.0f, 0.0f, 1.0f };
	body_def.angle = 0.0f;

	RigidBody *body = physicsScene_createBody(&g_physics, &body_def);
	entity->body = body;

	if (def->shape) {
		switch (def->shape->type) {
			case SHAPE_BOX:     rigidBody_addBox    (body, &def->shape->box);     break;
			case SHAPE_SPHERE:  rigidBody_addSphere (body, &def->shape->sphere);  break;
			case SHAPE_CAPSULE: rigidBody_addCapsule(body, &def->shape->capsule); break;
		}
	}
}


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
		case CAMERA_TYPE_SPHERICAL:
			cameraSpherical_init(camera, &def->camera.spherical);
			break;
		case CAMERA_TYPE_NONE:
		case CAMERA_TYPE_COUNT:
			break;
	}

	assert(scene.entity_count == 0);
	scene = (Scene){0};

	Vector3 gravity = { 0.0f, 0.0f, -9.8f };
	physicsScene_init(&g_physics, PHYSICS_TIMESTEP, gravity, PHYSICS_SOLVER_ITERATIONS);

	for (int i = 0; i < def->entity_count; i++) {
		const EntityDef *entity_def = &def->entity[i];
		Entity *entity = entity_create(entity_def);

		if (entity_def->body) {
			entity_attachPhysics(entity, entity_def);
		}

		if (!entity_def->actor) {
			for (int fb = 0; fb < FB_COUNT; fb++)
				mesh_setMatrix(entity->mesh, &entity->transform, fb);
		}

		scene.entity[scene.entity_count++] = entity;
	}
}


void scene_clear(void)
{
	scene = (Scene){0};
}

void scene_unload(void)
{
	for (int i = 0; i < scene.entity_count; i++)
		entity_delete(scene.entity[i]);
	scene_clear();
	physicsScene_shutdown(&g_physics);
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
	for (int i = 0; i < scene.entity_count; i++) {
		if (scene.entity[i]->type != ENTITY_ACTOR) continue;
		if (found == index) return scene.entity[i];
		found++;
	}
	return NULL;
}
