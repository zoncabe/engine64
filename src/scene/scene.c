#include <assert.h>

#include <t3d/t3dmath.h>

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

Scene *scene_get(void) { return &scene; }
PhysicsWorld *physics_getWorld(void) { return &g_physics; }


/* World transform of a static collider: entity position in metres plus the
   entity rotation built with the same euler function the renderer uses, so
   collision and visuals always match. */
static Transform entity_colliderTransform(const EntityDef *def)
{
	T3DMat4 mat;
	t3d_mat4_from_srt_euler(&mat,
		(float[3]){1.0f, 1.0f, 1.0f},
		(float[3]){deg_to_rad(def->rotation.x), deg_to_rad(def->rotation.y), deg_to_rad(def->rotation.z)},
		(float[3]){0.0f, 0.0f, 0.0f});

	return (Transform){
		.rotation = {
			.ex = { mat.m[0][0], mat.m[0][1], mat.m[0][2] },
			.ey = { mat.m[1][0], mat.m[1][1], mat.m[1][2] },
			.ez = { mat.m[2][0], mat.m[2][1], mat.m[2][2] },
		},
		.position = {
			def->position.x * RENDER_SCALE_INV,
			def->position.y * RENDER_SCALE_INV,
			def->position.z * RENDER_SCALE_INV,
		},
	};
}

/* Shape defs usually initialise .tx with a position only, leaving the
   rotation matrix zeroed: treat an all-zero rotation as identity. */
static Transform shape_localTransform(const Transform *tx)
{
	Transform local = *tx;
	if (vector3_squaredMagnitude(&local.rotation.ex) == 0.0f &&
	    vector3_squaredMagnitude(&local.rotation.ey) == 0.0f &&
	    vector3_squaredMagnitude(&local.rotation.ez) == 0.0f)
		local.rotation = matrix3_identity();
	return local;
}

/* Entities with a shape and no rigid body become static colliders. */
static void entity_attachStaticShape(const EntityDef *def)
{
	Transform world = entity_colliderTransform(def);

	/* The entity scale applies to the collider too, so one shape def
	   serves any prop size and visuals and collision cannot diverge. */
	Vector3 scale = def->scale;

	switch (def->shape->type) {
		case SHAPE_BOX: {
			assert(scene.static_box_count < SCENE_MAX_STATIC_BOXES);
			Transform local = shape_localTransform(&def->shape->box.tx);
			local.position = (Vector3){ local.position.x * scale.x, local.position.y * scale.y, local.position.z * scale.z };
			scene.static_box_transform[scene.static_box_count] = transform_product(&world, &local);
			scene.static_box[scene.static_box_count++] = (Box){ .e = {
				def->shape->box.e.x * scale.x,
				def->shape->box.e.y * scale.y,
				def->shape->box.e.z * scale.z,
			}};
			break;
		}
		case SHAPE_SPHERE: {
			assert(scene.static_sphere_count < SCENE_MAX_STATIC_SPHERES);
			Transform local = shape_localTransform(&def->shape->sphere.tx);
			local.position = (Vector3){ local.position.x * scale.x, local.position.y * scale.y, local.position.z * scale.z };
			scene.static_sphere_transform[scene.static_sphere_count] = transform_product(&world, &local);
			scene.static_sphere[scene.static_sphere_count++] = (Sphere){ .radius = def->shape->sphere.radius * scale.x };
			break;
		}
		case SHAPE_CAPSULE: {
			assert(scene.static_capsule_count < SCENE_MAX_STATIC_CAPSULES);
			Transform local = shape_localTransform(&def->shape->capsule.tx);
			local.position = (Vector3){ local.position.x * scale.x, local.position.y * scale.y, local.position.z * scale.z };
			scene.static_capsule_transform[scene.static_capsule_count] = transform_product(&world, &local);
			scene.static_capsule[scene.static_capsule_count++] = (Capsule){
				.radius      = def->shape->capsule.radius * scale.x,
				.half_height = def->shape->capsule.half_height * scale.z,
			};
			break;
		}
	}
}


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

	RigidBody *body = physicsWorld_createBody(&g_physics, &body_def);
	body->owner = entity;

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
	physicsWorld_init(&g_physics, PHYSICS_TIMESTEP, gravity, PHYSICS_SOLVER_ITERATIONS);

	for (int i = 0; i < def->entity_count; i++) {
		const EntityDef *entity_def = &def->entity[i];
		Entity *entity = entity_create(entity_def);

		if (entity_def->body) {
			entity_attachPhysics(entity, entity_def);
		} else if (entity_def->shape) {
			entity_attachStaticShape(entity_def);
		}

		if (entity_def->character) {
			assert(scene.character_count < SCENE_MAX_CHARACTERS);
			scene.character[scene.character_count++] = character_create(entity_def->character, entity);
		}

		if (entity_def->collision_path) {
			assert(scene.static_mesh_count < SCENE_MAX_STATIC_MESHES);
			/* Entity positions are in render units; collision runs in metres. */
			scene.static_mesh_origin[scene.static_mesh_count] = (Vector3){
				entity_def->position.x * RENDER_SCALE_INV,
				entity_def->position.y * RENDER_SCALE_INV,
				entity_def->position.z * RENDER_SCALE_INV,
			};
			scene.static_mesh[scene.static_mesh_count++] = collisionMesh_load(entity_def->collision_path);
		}

		if (!entity_def->character) {
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
	for (int i = 0; i < scene.character_count; i++)
		character_delete(scene.character[i]);
	for (int i = 0; i < scene.static_mesh_count; i++)
		collisionMesh_delete(scene.static_mesh[i]);
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
