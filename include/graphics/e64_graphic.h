/*
	What a 2D entity draws with, the way Mesh is what a 3D entity draws
	with: a rectangle, a sprite or a text, and whether it shows. Where it is
	drawn, how big and how turned is the entity's, not here.
*/
#ifndef ENGINE64_GRAPHIC_H
#define ENGINE64_GRAPHIC_H

#include <stdbool.h>
#include <stdint.h>

#include "graphics/e64_shapes.h"
#include "graphics/e64_sprites.h"
#include "graphics/e64_font.h"

namespace e64 {

class Graphic {
public:

	enum Type {
		RECTANGLE,
		SPRITE,
		TEXT,
	};


	Type type;

	union {
		Rectangle rectangle;
		Sprite sprite;
		Text text;
	};

	uint8_t transparency;
	bool is_hidden;
};


namespace graphic {

/* The definition names the file or the clips; the live copy gets them loaded.
   An animated sprite starts on the first frame of its first clip. destroy
   closes them and frees the copy. */
Graphic *create(const Graphic *def);
void destroy(Graphic *graphic);

}

}

#endif
