#include <assert.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "entity/entity.h"
#include "actor/actor_states.h"
#include "actor/actor_animation.h"
#include "viewport/viewport.h"


void entity_init(Entity *entity, EntityType type, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings)
{
	*entity = (Entity){ .type = type };
	transform_init(&entity->transform);

	if (type == ENTITY_ACTOR) {
		entity->actor = malloc(sizeof(Actor));
		assert(entity->actor);
		*entity->actor = (Actor){
			.body = (RigidBody){0},
			.motion = (ActorMotion){ .settings = *motion_settings, .data.is_grounded = true },
			.animation = (ActorAnimation){ .settings = *animation_settings },
			.state = (ActorStateData){ .current = ACTOR_STATE_IDLE },
		};
	}
}

Entity *entity_create(EntityType type, const char *model_path, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings, const AnimationDef *animation_def)
{
	Entity *entity = malloc(sizeof(Entity));
	assert(entity);
	entity_init(entity, type, motion_settings, animation_settings);

	entity->mesh = malloc(sizeof(Mesh));
	assert(entity->mesh);
	entity->mesh->model = t3d_model_load(model_path);
	assert(entity->mesh->model);
	entity->mesh->matrix_buffer = malloc_uncached(sizeof(T3DMat4FP) * FB_COUNT);
	assert(entity->mesh->matrix_buffer);
	t3d_mat4fp_identity(entity->mesh->matrix_buffer);

	if (type == ENTITY_ACTOR) {
		actorAnimation_initGraph(entity, animation_def);

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
		for (uint8_t i = 0; i < entity->actor->animation_def->clip_count; i++)
			t3d_anim_destroy(&animation->clip[i]);
		for (uint8_t i = 0; i < entity->actor->animation_def->buffer_count; i++)
			t3d_skeleton_destroy(&animation->buffer[i]);
		t3d_skeleton_destroy(&animation->main);
		free(animation->clip);
		free(animation->buffer);
		free(animation->node_state);
		free(animation->node_active);
		free(entity->actor);
	}

	free(entity);
}
