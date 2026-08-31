#ifndef ENGINE64_RESOURCES_H
#define ENGINE64_RESOURCES_H

#include <stdint.h>
#include "graphics/e64_sprites.h"
#include "graphics/e64_font.h"


typedef struct {

	const SpriteID *sprite;
	uint8_t         sprite_count;
	const uint8_t  *font;
	uint8_t         font_count;

} ResourceSet;


void resources_load(const ResourceSet *set);
void resources_unload(const ResourceSet *set);

#endif
