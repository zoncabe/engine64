#include <assert.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "entity/entity.h"
#include "character/character_animation.h"
#include "viewport/viewport.h"
#include "physics/math/math_common.h"


void entity_init(Entity *entity, const EntityDef *def)
{
	*entity = (Entity){ .type = def->character ? ENTITY_CHARACTER : ENTITY_SCENERY };
	renderTransform_init(&entity->transform);
	entity->transform.position = def->position;
	entity->transform.rotation = def->rotation;
	entity->transform.scale    = def->scale;

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

	entity->mesh->skeleton = NULL;

	if (def->character) {
		entity->mesh->dl = NULL;   /* character_create builds the skinned dl */
	} else {
		rspq_block_begin();
		t3d_model_draw(entity->mesh->model);
		entity->mesh->dl = rspq_block_end();
	}

	return entity;
}

void entity_delete(Entity *entity)
{
	if (entity->mesh->dl) rspq_block_free(entity->mesh->dl);
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
