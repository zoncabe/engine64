#include <assert.h>
#include <malloc.h>
#include <libdragon.h>
#include <t3d/t3dmodel.h>

#include "entity/entity.h"
#include "character/character.h"
#include "character/character_animation.h"


Character *character_create(const CharacterDef *def, Entity *entity)
{
	Character *character = malloc(sizeof(Character));
	assert(character);

	*character = (Character){
		.entity    = entity,
		.body      = (KinematicBody){ .position = vector3_scaled(&entity->transform.position, RENDER_SCALE_INV), .rotation = entity->transform.rotation },
		.movement  = (CharacterMovement){ .settings = def->movement_settings, .data.is_grounded = true, .current = MOVEMENT_STATE_IDLE },
		.animation = (CharacterAnimation){ .def = def->animation_def },
	};

	characterCollider_init(&character->collider,
		def->collider_settings->radius,
		(def->collider_settings->height - 2.0f * def->collider_settings->radius) * 0.5f);
	characterCollider_setVertical(&character->collider, &character->body.position);

	characterAnimation_initGraph(character, def->animation_def);
	entity->mesh->skeleton = &character->animation.main;

	rspq_block_begin();
	t3d_model_draw_skinned(entity->mesh->model, &character->animation.main);
	entity->mesh->dl = rspq_block_end();

	return character;
}

void character_delete(Character *character)
{
	CharacterAnimation *animation = &character->animation;

	for (int i = 0; i < animation->def->clip_count; i++)
		t3d_anim_destroy(&animation->clip[i]);
	for (int i = 0; i < animation->def->buffer_count; i++)
		t3d_skeleton_destroy(&animation->buffer[i]);
	t3d_skeleton_destroy(&animation->main);

	free(animation->clip);
	free(animation->buffer);
	free(animation->node_state);
	free(animation->node_active);
	free(character);
}
