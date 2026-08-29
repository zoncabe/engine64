#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>

#include "physics/math/vector2.h"


/* Index into the path table the game hands to sprite_init. */
typedef uint8_t SpriteID;

typedef struct {

	SpriteID id;
	uint8_t  frame;         /* frame within the sheet */
	uint8_t  frame_count;   /* 0 or 1 = the whole sprite */

} Sprite;


/* Hands the engine the game's sprite path table. Runs once, before any
   state loads resources. */
void sprite_init(const char *const *paths, uint8_t count);

void sprite_loadAsset(SpriteID id);
void sprite_unloadAsset(SpriteID id);

/* The loaded sprite behind an id, for whoever uploads it by hand instead of
   going through sprite_draw. NULL while its state has it unloaded. */
struct sprite_s *sprite_getAsset(SpriteID id);
void sprite_setMode(void);
void sprite_draw(const Sprite *element, Vector2 position, Vector2 scale, float rotation);
void sprite_drawTiled(const Sprite *element, Vector2 position, Vector2 size);

#endif
