/*
	The live graphic of an entity: a copy of the prefab's, with the sprite
	file or clips it names opened, the way mesh::create opens a model.
*/
#include <assert.h>
#include <malloc.h>
#include <libdragon.h>

#include "graphics/e64_graphic.h"
#include "resource/e64_resource.h"

namespace e64 {

namespace graphic {

Graphic *create(const Graphic *def)
{
	assert(def);

	Graphic *graphic = (Graphic *)malloc(sizeof(Graphic));
	assert(graphic);

	*graphic = *def;
	if (graphic->type == Graphic::SPRITE) {
		Sprite *sprite = &graphic->sprite;
		if (sprite->animation_def) {
			sprite->animation = (sprite::Animation *)malloc(sizeof(sprite::Animation));
			assert(sprite->animation);
			sprite::animation::init(sprite->animation, sprite->animation_def, 0);
			sprite->asset = sprite::animation::getSprite(sprite->animation);
		}
		else {
			sprite->asset = (sprite_t *)resource::load(sprite->path, Resource::SPRITE, NULL);
			assert(sprite->asset);
		}
	}

	return graphic;
}

void destroy(Graphic *graphic)
{
	if (!graphic) return;

	if (graphic->type == Graphic::SPRITE) {
		Sprite *sprite = &graphic->sprite;
		if (sprite->animation) {
			sprite::animation::destroy(sprite->animation);
			free(sprite->animation);
		}
		else resource::unload(sprite->asset);
	}

	free(graphic);
}

}

}
