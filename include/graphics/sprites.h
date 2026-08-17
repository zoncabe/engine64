#ifndef SPRITES_H
#define SPRITES_H

#include "physics/math/vector2.h"


typedef enum {

	SPRITE_GORILLA,
	SPRITE_BTN_A,
	SPRITE_BTN_B,
	SPRITE_D_UP,
	SPRITE_D_DOWN,
	SPRITE_D_LEFT,
	SPRITE_D_RIGHT,
	SPRITE_LIBDRAGON,
	SPRITE_TINY3D,
	SPRITE_ZONCABE,
	SPRITE_COUNT

} SpriteID;

typedef struct {

	SpriteID id;

} Sprite;


void sprite_loadAsset(SpriteID id);
void sprite_unloadAsset(SpriteID id);
void sprite_setMode(void);
void sprite_draw(const Sprite *element, Vector2 position, Vector2 scale, float rotation);

#endif
