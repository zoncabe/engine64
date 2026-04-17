#include <libdragon.h>


#include "graphics/color.h"


color_hsv rgb_to_hsv(color_t c)
{
	float r = c.r / 255.0f;
	float g = c.g / 255.0f;
	float b = c.b / 255.0f;

	float max = r > g ? (r > b ? r : b) : (g > b ? g : b);
	float min = r < g ? (r < b ? r : b) : (g < b ? g : b);
	float d = max - min;

	color_hsv out;
	out.v = max;
	out.s = (max == 0.0f) ? 0.0f : d / max;

	if (d == 0.0f) {
		out.h = 0.0f;
	} else if (max == r) {
		out.h = (g - b) / d + (g < b ? 6.0f : 0.0f);
	} else if (max == g) {
		out.h = (b - r) / d + 2.0f;
	} else {
		out.h = (r - g) / d + 4.0f;
	}

	out.h /= 6.0f;
	out.a = c.a / 255.0f;
	return out;
}

color_t hsv_to_rgb(color_hsv h)
{
	float r, g, b;

	if (h.s == 0.0f) {
		r = g = b = h.v;
	} else {
		float i = (float)((int)(h.h * 6.0f));
		float f = h.h * 6.0f - i;
		float p = h.v * (1.0f - h.s);
		float q = h.v * (1.0f - h.s * f);
		float t = h.v * (1.0f - h.s * (1.0f - f));

		switch ((int)i % 6) {
			case 0: r = h.v; g = t;   b = p;   break;
			case 1: r = q;   g = h.v; b = p;   break;
			case 2: r = p;   g = h.v; b = t;   break;
			case 3: r = p;   g = q;   b = h.v; break;
			case 4: r = t;   g = p;   b = h.v; break;
			default:r = h.v; g = p;   b = q;   break;
		}
	}

	return (color_t){
		.r = (uint8_t)(r * 255.0f),
		.g = (uint8_t)(g * 255.0f),
		.b = (uint8_t)(b * 255.0f),
		.a = (uint8_t)(h.a * 255.0f),
	};
}

color_t color_lerp(color_t* a, color_t* b, float t)
{
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	color_hsv ha = rgb_to_hsv(*a);
	color_hsv hb = rgb_to_hsv(*b);

	float dh = hb.h - ha.h;
	if (dh > 0.5f) dh -= 1.0f;
	if (dh < -0.5f) dh += 1.0f;

	color_hsv h = {
		.h = ha.h + dh * t,
		.s = ha.s + (hb.s - ha.s) * t,
		.v = ha.v + (hb.v - ha.v) * t,
		.a = ha.a + (hb.a - ha.a) * t,
	};

	if (h.h < 0.0f) h.h += 1.0f;
	if (h.h > 1.0f) h.h -= 1.0f;

	return hsv_to_rgb(h);
}

color_t color_lerpRGB(color_t *a, color_t *b, float t)
{
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	return (color_t){
		.r = (uint8_t)(a->r + (b->r - a->r) * t),
		.g = (uint8_t)(a->g + (b->g - a->g) * t),
		.b = (uint8_t)(a->b + (b->b - a->b) * t),
		.a = (uint8_t)(a->a + (b->a - a->a) * t),
	};
}

/*
color_t ambient_color = {10, 10, 10, 0xFF};

color_t red = {255, 0, 0, 0xFF}, green = {0, 255, 0, 0xFF}, blue = {0, 0, 255, 0xFF};

float interpolator[4] = {0.0f, 0.75f, 1.5f, 2.25f};

color_t lamp[4];

void set_RGB_colors(color_t *color, float *interpolator)
{

	*interpolator += timer.delta;
	if (*interpolator >= 3.0f) *interpolator -= 3.0f;

	if (*interpolator < 1.0f) *color = color_lerp(&red, &blue, *interpolator);
	else if (*interpolator < 2.0f) *color = color_lerp(&blue, &green, *interpolator - 1.0f);
	else *color = color_lerp(&green, &red, *interpolator - 2.0f);
}

void change_lamp_colors()
{
	for (int i = 0; i < 4; i++) {
		set_RGB_colors(&lamp[i], &interpolator[i]);
		light_updatePoint(&light.point[i], light.point[i].position, lamp[i], light.point[i].size);
	}
}
*/
