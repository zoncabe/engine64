#include <assert.h>
#include <malloc.h>
#include <libdragon.h>
#include "graphics/e64_sprites.h"


/* The game's table, handed over at sprite_init. */
static const char *const *sprite_path;
static uint8_t             sprite_count;

static sprite_t **sprite;


void sprite_init(const char *const *paths, uint8_t count)
{
	sprite_path  = paths;
	sprite_count = count;

	sprite = calloc(count, sizeof(sprite_t *));
	assert(sprite);
}

void sprite_loadAsset(SpriteID id)
{
	assert(id < sprite_count);
	sprite[id] = sprite_load(sprite_path[id]);
	assert(sprite[id]);
}

void sprite_unloadAsset(SpriteID id)
{
	sprite_free(sprite[id]);
	sprite[id] = NULL;
}

sprite_t *sprite_getAsset(SpriteID id)
{
	return sprite[id];
}

void sprite_setMode()
{
	rdpq_set_mode_standard();
	rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
	rdpq_mode_alphacompare(1);
}

void sprite_drawTiled(const Sprite *element, Vector2 position, Vector2 size)
{
	sprite_t *s = sprite[element->id];
	rdpq_sprite_upload(TILE0, s, &(rdpq_texparms_t){
		.s = { .repeats = REPEAT_INFINITE },
		.t = { .repeats = REPEAT_INFINITE },
	});
	rdpq_texture_rectangle(TILE0, position.x, position.y,
	                       position.x + size.x, position.y + size.y, 0, 0);
}

void sprite_draw(const Sprite *element, Vector2 position, Vector2 scale, float rotation)
{
	sprite_t *s = sprite[element->id];
	int count = element->frame_count ? element->frame_count : 1;
	int h     = s->height / count;

	rdpq_sprite_blit(s, position.x, position.y, &(rdpq_blitparms_t){
		.t0      = element->frame * h,
		.height  = h,
		.scale_x = scale.x,
		.scale_y = scale.y,
		.theta   = rotation,
		.cx      = (rotation != 0.0f) ? s->width / 2 : 0,
		.cy      = (rotation != 0.0f) ? h / 2 : 0,
	});
}
