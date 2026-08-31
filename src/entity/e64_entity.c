#include <assert.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "entity/e64_entity.h"
#include "shaders/e64_mesh_deform.h"
#include "character/e64_character_animation.h"
#include "viewport/e64_viewport.h"
#include "physics/math/e64_math_common.h"
#include "physics/world/e64_physics_world.h"


void entity_init(Entity *entity, const EntityDef *def)
{
	*entity = (Entity){0};
	renderTransform_init(&entity->transform);
	entity->transform.position = def->position;
	entity->transform.rotation = def->rotation;
	entity->transform.scale    = def->scale;
	entity->cull               = def->cull;

}

Entity *entity_create(const EntityDef *def)
{
	Entity *entity = malloc(sizeof(Entity));
	assert(entity);
	entity_init(entity, def);

	entity->mesh = malloc(sizeof(Mesh));
	assert(entity->mesh);
	entity->mesh->model = t3d_model_load(def->model_path);
	assert(entity->mesh->model);
	entity->mesh->matrix_buffer = malloc_uncached(sizeof(T3DMat4FP) * FB_COUNT);
	assert(entity->mesh->matrix_buffer);
	t3d_mat4fp_identity(entity->mesh->matrix_buffer);

	entity->mesh->skeleton  = NULL;
	entity->mesh->deform    = NULL;
	entity->mesh->draw_conf = NULL;
	mesh_initBounds(entity->mesh);

	if (def->character) {
		entity->mesh->dl = NULL;   /* character_create builds the skinned parts */
		entity->mesh->dl_count = 0;
		entity->mesh->visible  = 0;
	} else if (def->cloth) {
		entity->mesh->dl = malloc(sizeof(rspq_block_t *));
		assert(entity->mesh->dl);
		rspq_block_begin();
		t3d_model_draw(entity->mesh->model);
		entity->mesh->dl[0]    = rspq_block_end();
		entity->mesh->dl_count = 1;
		entity->mesh->visible  = 1;
	} else {
		mesh_recordObjects(entity->mesh);
	}

	return entity;
}

void entity_delete(Entity *entity)
{
	for (int i = 0; i < entity->mesh->dl_count; i++)
		rspq_block_free(entity->mesh->dl[i]);
	free(entity->mesh->dl);
	if (entity->mesh->deform) {
		meshDeform_delete(entity->mesh->deform);
		free(entity->mesh->deform);
	}
	free(entity->mesh->bound);
	free_uncached(entity->mesh->matrix_buffer);
	t3d_model_free(entity->mesh->model);
	free(entity->mesh);

	free(entity);
}

/* The body lives in metres; the render transform in render units. */
void entity_setTransform(Entity *entity, const KinematicBody *body)
{
	entity->transform.position = vector3_scaled(&body->position, RENDER_SCALE);
	entity->transform.rotation = body->rotation;
}

void entity_setMatrix(Entity *entity, uint8_t fb_index)
{
	mesh_setMatrix(entity->mesh, &entity->transform, fb_index);
}

/* For entities the solver moves: their placement lives in the body, not in the
   render transform, and a tumbling body needs its quaternion rather than the
   euler angles the transform carries. No-op for anything else. */
void entity_setMatrixFromBody(Entity *entity, uint8_t fb_index)
{
	if (entity->body == NULL || !(entity->body->flags & BODY_FLAG_DYNAMIC)) return;

	mesh_setMatrixFromBody(entity->mesh, &entity->body->tx.position, &entity->body->q,
	                       &entity->transform.scale, fb_index);
}


/* World transform of a static collider: entity position in metres plus the
   entity rotation built with the same euler function the renderer uses, so
   collision and visuals always match. */
Transform entity_colliderTransform(const EntityDef *def)
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

/* Copies the body-def override bits on top of the freshly-initialised body
   (position, orientation) and then attaches the entity's shape to it. */
RigidBody *entity_attachPhysics(Entity *entity, const EntityDef *def, PhysicsWorld *world)
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

	/* Position in metres and rotation through the renderer's euler convention,
	   both from the collider transform: a rotated entity collides the way it
	   renders. */
	Transform collider = entity_colliderTransform(def);
	body_def.position = collider.position;

	Quaternion rotation = quaternion_fromMatrix3(&collider.rotation);
	quaternion_toAxisAngle(&rotation, &body_def.axis, &body_def.angle);

	/* Identity degenerates to a zero axis; any axis stands for no rotation. */
	if (vector3_squaredMagnitude(&body_def.axis) == 0.0f)
		body_def.axis = (Vector3){ 0.0f, 0.0f, 1.0f };

	RigidBody *body = physicsWorld_createBody(world, &body_def);
	body->owner   = entity;
	entity->body  = body;

	if (def->collider) {
		for (uint8_t i = 0; i < def->collider->count; i++)
			rigidBody_addShape(body, &def->collider->shape[i], def->scale);
	}

	return body;
}

