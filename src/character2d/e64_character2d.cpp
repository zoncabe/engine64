/*
	The 2D character: the movement moves it and this hands the body to the
	scene entity. Which clip the sprite plays is the game's to pick, on the
	entity's sprite animation. The position moves at the game's rate and is
	snapped to whole pixels only when set on the entity, so the sprite never
	lands on a half pixel.
*/
#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <libdragon.h>

#include "character2d/e64_character2d.h"

namespace e64 {

namespace character2d {

/* Hands the scene entity the body: the flip, and the feet position carried
   to the top-left corner the blit draws from. The sprite drawn is whatever
   the entity holds, the frame the scene's animation pass left there. */
static void setEntity(Character2D *character)
{
	Entity2D *entity = character->entity;
	Sprite *sprite = &entity->graphic->sprite;

	/* The frames are drawn facing left, so the mirror is the right side. */
	sprite->flip_x = !character->facing_left;

	/* Feet at the bottom center of the frame as drawn: the placement's
	   scale is applied at the blit, so it counts here too.

	   Left in world pixels, fractions and all: the scene rounds once, after
	   the camera, the same as it does with a tile. Rounding here as well
	   would land the body on whole world pixels and then on whole screen
	   ones, and the two together hold it still for a frame and move it two
	   the next. */
	float w = sprite->asset->width * entity->scale.x;
	float h = sprite->asset->height * entity->scale.y;

	entity->position.x = character->position.x - w * 0.5f;
	entity->position.y = character->position.y - h;
}


Character2D *create(const Def *def, Entity2D *entity)
{
	assert(def && entity && entity->graphic->type == Graphic::SPRITE);

	Character2D *character = (Character2D *)malloc(sizeof(Character2D));
	assert(character);

	/* The placement put the feet where the entity stands. */
	*character = (Character2D){
		.def = def,
		.entity = entity,
		.position = entity->position,
		.movement = (Movement){ .settings = def->movement_settings, .data = { .is_grounded = true }, .current = MOVEMENT2D_STATE_IDLE },
	};

	setEntity(character);
	return character;
}

void destroy(Character2D *character)
{
	::free(character);
}

void update(Character2D *character, float dt)
{
	setEntity(character);
}

}

}
