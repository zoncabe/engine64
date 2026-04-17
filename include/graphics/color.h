#ifndef COLOR_H
#define COLOR_H

typedef struct {

	float h, s, v, a;

} color_hsv;


color_t color_lerp(color_t *a, color_t *b, float t);

color_t color_lerpRGB(color_t *a, color_t *b, float t);

#endif