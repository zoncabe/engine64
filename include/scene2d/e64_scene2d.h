#ifndef ENGINE64_SCENE2D_H
#define ENGINE64_SCENE2D_H

#include "render/e64_render.h"

#define SCENE2D_MAX_LAYER    8
#define SCENE2D_MAX_ELEMENT 64


/* A layer groups elements under one scissor. */
typedef struct Scene2DLayer {

	const Element2D   *element;
	uint8_t            element_count;

	bool               has_scissor;
	float              scissor_x;
	float              scissor_y;
	float              scissor_w;
	float              scissor_h;

} Scene2DLayer;

typedef struct Scene2DDef {

	const Scene2DLayer *layer;
	uint8_t             layer_count;

} Scene2DDef;


/* The live scene: the elements the render draws and whoever animates them
   writes, flat, with each layer's start. */
typedef struct Scene2D {

	const Scene2DDef *def;

	Element2D element[SCENE2D_MAX_ELEMENT];
	uint8_t   layer_start[SCENE2D_MAX_LAYER];
	uint8_t   element_count;

} Scene2D;


Scene2D *scene2d_get(void);

void scene2d_load(const Scene2DDef *def);
void scene2d_unload(void);

/* The live element behind a layer and its index in the definition. */
Element2D *scene2d_getElement(Scene2D *scene2d, uint8_t layer, uint8_t element);

#endif
