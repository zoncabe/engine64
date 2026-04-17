#include "graphics/shapes.h"


#define COLOR_NORM 0.003922f

void shape_drawRectangle(const Rectangle *rect)
{
	float x  = rect->position.x, y  = rect->position.y;
	float sx = rect->size.x,     sy = rect->size.y;

	if (rect->fill == SHAPE_FILL_SOLID) {

		rdpq_set_prim_color(rect->color);
		rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
		rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
		rdpq_fill_rectangle(x, y, sx, sy);
	}
	else {

		color_t c0 = rect->gradient[0], c1 = rect->gradient[1],
				c2 = rect->gradient[2], c3 = rect->gradient[3];
		rdpq_set_mode_standard();
		rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
		rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
		rdpq_mode_dithering(DITHER_NOISE_NOISE);

		float vtx[4][6] = {
			{ x,    y,     c0.r*COLOR_NORM, c0.g*COLOR_NORM, c0.b*COLOR_NORM, c0.a*COLOR_NORM },
			{ x+sx, y,     c1.r*COLOR_NORM, c1.g*COLOR_NORM, c1.b*COLOR_NORM, c1.a*COLOR_NORM },
			{ x+sx, y+sy,  c2.r*COLOR_NORM, c2.g*COLOR_NORM, c2.b*COLOR_NORM, c2.a*COLOR_NORM },
			{ x,    y+sy,  c3.r*COLOR_NORM, c3.g*COLOR_NORM, c3.b*COLOR_NORM, c3.a*COLOR_NORM },
		};

		rdpq_triangle(&TRIFMT_SHADE, vtx[0], vtx[1], vtx[2]);
		rdpq_triangle(&TRIFMT_SHADE, vtx[0], vtx[2], vtx[3]);
	}
}

#undef COLOR_NORM