#ifndef SHAPES_H
#define SHAPES_H

#include <libdragon.h>
#include "physics/math/vector2.h"


typedef enum {

	SHAPE_FILL_SOLID,
	SHAPE_FILL_GRADIENT,
	
} ShapeFill;

typedef struct {

	ShapeFill fill;
	union {
		color_t color;
		color_t gradient[4];
	};

} Rectangle;


void shape_drawRectangle(const Rectangle *rect, Vector2 position, Vector2 scale);

#endif