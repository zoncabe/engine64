#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdint.h>
#include "graphics/sprites.h"
#include "graphics/font.h"
#include "game/game_states.h"


typedef struct {

	const SpriteID *sprite;
	uint8_t         sprite_count;
	const uint8_t  *font;
	uint8_t         font_count;

} ResourceSet;

const ResourceSet *resources_forState(GameState state);


void resources_load(const ResourceSet *set);
void resources_unload(const ResourceSet *set);

#endif
