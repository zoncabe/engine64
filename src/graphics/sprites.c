#include <assert.h>
#include <libdragon.h>
#include "graphics/sprites.h"


static const char *sprite_paths[SPRITE_COUNT] = {

	[SPRITE_GORILLA]   = "rom:/textures/gorilla.rgba32.sprite",
	[SPRITE_BTN_A]     = "rom:/textures/AButton.sprite",
	[SPRITE_BTN_B]     = "rom:/textures/BButton.sprite",
	[SPRITE_D_UP]      = "rom:/textures/DUp.sprite",
	[SPRITE_D_DOWN]    = "rom:/textures/DDown.sprite",
	[SPRITE_N64LOGO]   = "rom:/textures/n64logo.sprite",
	[SPRITE_LIBDRAGON] = "rom:/textures/libdragon.sprite",
	[SPRITE_TINY3D]    = "rom:/textures/tiny3d.sprite",
	[SPRITE_ZONCABE]   = "rom:/textures/zoncabe.sprite",
	
};

static sprite_t *sprite[SPRITE_COUNT];


void sprite_loadAsset(SpriteID id)
{
	sprite[id] = sprite_load(sprite_paths[id]);
	assert(sprite[id]);
}

void sprite_unloadAsset(SpriteID id)
{
	sprite_free(sprite[id]);
	sprite[id] = NULL;
}

void sprite_setMode()
{
	rdpq_set_mode_standard();
	rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
	rdpq_mode_alphacompare(1);
}

void sprite_draw(const Sprite *element)
{
	rdpq_sprite_blit(sprite[element->id], element->position.x, element->position.y,
		&(rdpq_blitparms_t){ .scale_x = element->scale.x, .scale_y = element->scale.y });
}
