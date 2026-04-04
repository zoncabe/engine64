#ifndef SHAPES_H
#define SHAPES_H

#include <libdragon.h> // color_t
#include "../physics/math/vector2.h"

typedef enum {
	COLOR_SOLID,
	COLOR_GRADIENT,
} ShapeFill;

typedef struct {

	ShapeFill fill;
	Vector2       position;
	Vector2       size;
	union {
		color_t color;
		color_t gradient[4];
	};

} Rectangle;


void shapes_drawRectangle(const Rectangle *rect);


#endif