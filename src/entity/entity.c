#include <assert.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "entity/entity.h"
#include "actor/actor_states.h"
#include "actor/actor_animation.h"
#include "viewport/viewport.h"
#include "physics/math/math_common.h"


void entity_init(Entity *entity, const EntityDef *def)
{
	*entity = (Entity){ .type = def->actor ? ENTITY_ACTOR : ENTITY_SCENERY };
	renderTransform_init(&entity->transform);
	entity->transform.position = def->position;
	entity->transform.rotation = def->rotation;
	entity->transform.scale    = def->scale;

	/* Physics body is created and owned by the PhysicsWorld; scene.c attaches
	   it after entity_init. */
	entity->body = NULL;

	if (def->actor) {
		entity->actor = malloc(sizeof(Actor));
		assert(entity->actor);
		*entity->actor = (Actor){
			.motion = (ActorMotion){ .settings = *def->actor->motion_settings, .data.is_grounded = true },
			.animation = (ActorAnimation){ .settings = *def->actor->animation_settings },
			.state = (ActorStateData){ .current = ACTOR_STATE_IDLE },
		};
	}
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

	if (def->actor) {
		actorAnimation_initGraph(entity, def->actor->animation_def);

		rspq_block_begin();
		t3d_model_draw_skinned(entity->mesh->model, &entity->actor->animation.main);
		entity->mesh->dl = rspq_block_end();
	} else {
		rspq_block_begin();
		t3d_model_draw(entity->mesh->model);
		entity->mesh->dl = rspq_block_end();
	}

	return entity;
}

void entity_delete(Entity *entity)
{
	rspq_block_free(entity->mesh->dl);
	free_uncached(entity->mesh->matrix_buffer);
	t3d_model_free(entity->mesh->model);
	free(entity->mesh);

	if (entity->type == ENTITY_ACTOR) {
		ActorAnimation *animation = &entity->actor->animation;
		for (int i = 0; i < entity->actor->animation_def->clip_count; i++)
			t3d_anim_destroy(&animation->clip[i]);
		for (int i = 0; i < entity->actor->animation_def->buffer_count; i++)
			t3d_skeleton_destroy(&animation->buffer[i]);
		t3d_skeleton_destroy(&animation->main);
		free(animation->clip);
		free(animation->buffer);
		free(animation->node_state);
		free(animation->node_active);
		free(entity->actor);
	}

	/* entity->body is owned and freed by the PhysicsWorld's heap. */

	free(entity);
}

void entity_syncTransformToBody(Entity *entity)
{
	const RigidBody *body = entity->body;
	if (!body) return;

	/* Physics is in metres; render uses ×100. Position always comes from the
	   body (kinematic or dynamic); rotation comes from the body only for
	   dynamic — kinematic bodies don't rotate, the actor controls facing. */
	entity->transform.position.x = body->tx.position.x * RENDER_SCALE;
	entity->transform.position.y = body->tx.position.y * RENDER_SCALE;
	entity->transform.position.z = body->tx.position.z * RENDER_SCALE;

	if (!(body->flags & BODY_FLAG_KINEMATIC)) {
		Vector3 euler = matrix3_toEuler(&body->tx.rotation);
		entity->transform.rotation.x = rad_to_deg(euler.x);
		entity->transform.rotation.y = rad_to_deg(euler.y);
		entity->transform.rotation.z = rad_to_deg(euler.z);
	}
}

void entity_setMatrix(Entity *entity, uint8_t fb_index)
{
	const RigidBody *body = entity->body;

	/* No body, or kinematic body (player): use the classic Euler path
	   through entity.transform. */
	if (!body || (body->flags & BODY_FLAG_KINEMATIC)) {
		mesh_setMatrix(entity->mesh, &entity->transform, fb_index);
		return;
	}

	/* Dynamic body: matrix from body->tx; scale position up to render units. */
	const Vector3 *s = &entity->transform.scale;
	float px = body->tx.position.x * RENDER_SCALE;
	float py = body->tx.position.y * RENDER_SCALE;
	float pz = body->tx.position.z * RENDER_SCALE;

	const Matrix3 *r = &body->tx.rotation;

	T3DMat4 mat;
	mat.m[0][0] = r->ex.x * s->x;
	mat.m[0][1] = r->ex.y * s->x;
	mat.m[0][2] = r->ex.z * s->x;
	mat.m[0][3] = 0.0f;
	mat.m[1][0] = r->ey.x * s->y;
	mat.m[1][1] = r->ey.y * s->y;
	mat.m[1][2] = r->ey.z * s->y;
	mat.m[1][3] = 0.0f;
	mat.m[2][0] = r->ez.x * s->z;
	mat.m[2][1] = r->ez.y * s->z;
	mat.m[2][2] = r->ez.z * s->z;
	mat.m[2][3] = 0.0f;
	mat.m[3][0] = px;
	mat.m[3][1] = py;
	mat.m[3][2] = pz;
	mat.m[3][3] = 1.0f;

	t3d_mat4_to_fixed_3x4(&entity->mesh->matrix_buffer[fb_index], &mat);
}
