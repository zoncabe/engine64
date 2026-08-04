#include <assert.h>
#include <malloc.h>
#include <string.h>
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
		.weapons   = (CharacterWeapons){ .def = def->weapons_def, .drawn = CHARACTER_WEAPON_DRAWN_NONE },
	};

	characterCollider_init(&character->collider,
		def->collider_settings->radius,
		(def->collider_settings->height - 2.0f * def->collider_settings->radius) * 0.5f);
	characterCollider_setVertical(&character->collider, &character->body.position);

	characterAnimation_initGraph(character, def->animation_def);
	entity->mesh->skeleton = &character->animation.main;

	/* Part 0 = body, parts 1..N = one per weapon object, def order.
	   Only the body starts visible; equipping turns weapon bits on. */
	mesh_recordParts(entity->mesh, def->weapons_def->mesh, def->weapons_def->mesh_count,
		(const T3DMat4FP *)t3d_segment_placeholder(T3D_SEGMENT_SKELETON));

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
