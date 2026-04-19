#include <libdragon.h>

#include "../../../include/physics/math/math_common.h"


#define LN2 0.6931472f


float deg_to_rad(float angle)
{
	return PI / 180 * angle;
}

float rad_to_deg(float rad)
{
	return 180 / PI * rad;
}

float angle_wrap(float angle)
{
	while (angle >  180.0f) angle -= 360.0f;
	while (angle <= -180.0f) angle += 360.0f;
	return angle;
}

float angle_wrap_relative(float angle, float reference)
{
	while (angle >  reference + 180.0f) angle -= 360.0f;
	while (angle <= reference - 180.0f) angle += 360.0f;
	return angle;
}

float lerpf(float a, float b, float t)
{
	return a + t * (b - a);
}

float ease_linear(float t)
{
	return t;
}

float ease_quad_in(float t)
{
	return t * t;
}

float ease_quad_out(float t)
{
	return 1.0f - (1.0f - t) * (1.0f - t);
}

float ease_quad_in_out(float t)
{
	if (t < 0.5f) return 2.0f * t * t;
	float inv = 1.0f - t;
	return 1.0f - 2.0f * inv * inv;
}

float ease_cubic_in(float t)
{
	return t * t * t;
}

float ease_cubic_out(float t)
{
	float inv = 1.0f - t;
	return 1.0f - inv * inv * inv;
}

float ease_cubic_in_out(float t)
{
	if (t < 0.5f) return 4.0f * t * t * t;
	float inv = 1.0f - t;
	return 1.0f - 4.0f * inv * inv * inv;
}

float ease_expo_in(float t)
{
	if (t <= 0.0f) return 0.0f;
	return fm_exp((10.0f * t - 10.0f) * LN2);
}

float ease_expo_out(float t)
{
	if (t >= 1.0f) return 1.0f;
	return 1.0f - fm_exp(-10.0f * t * LN2);
}

float ease_expo_in_out(float t)
{
	if (t <= 0.0f) return 0.0f;
	if (t >= 1.0f) return 1.0f;
	if (t < 0.5f) return 0.5f * fm_exp((20.0f * t - 10.0f) * LN2);
	return 1.0f - 0.5f * fm_exp((-20.0f * t + 10.0f) * LN2);
}
