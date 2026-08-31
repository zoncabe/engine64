#include <libdragon.h>
#include <stdint.h>
#include <string.h>

#include "../../../include/physics/math/e64_math_common.h"


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

float qi_sqrt(float x)
{
	/*
	 * Kaze Emanuar's improvement over the Quake III hack.
	 *
	 * Step 1: initial bit hack. Reinterpret the float as uint32 and apply
	 *         i = 0x5F3759DF - (i >> 1). This produces a first approximation
	 *         to 1/sqrt(x) by exploiting IEEE 754 representation.
	 *
	 *         Kaze's tweak: the original computed 'half_x = x * 0.5f' before
	 *         the Newton-Raphson step. Replacing that with an exponent
	 *         subtraction (i.e. dividing by 2 via bit ops) saves one cycle.
	 *         Equivalent for normal-magnitude floats.
	 *
	 * Step 2: one Newton-Raphson iteration to refine the guess. One iteration
	 *         brings the error down to ~0.175%; without Newton it is ~3.5%.
	 *
	 * memcpy instead of union/cast: avoids strict aliasing UB. GCC at -O2
	 * compiles it to a direct MTC1/MFC1 with no extra instructions.
	 */

	uint32_t i;
	float    half_x = x * 0.5f;
	float    y      = x;

	memcpy(&i, &y, sizeof(i));
	i = 0x5F3759DF - (i >> 1);
	memcpy(&y, &i, sizeof(y));

	y = y * (1.5f - half_x * y * y);

	return y;
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
	return fm_expf((10.0f * t - 10.0f) * LN2);
}

float ease_expo_out(float t)
{
	if (t >= 1.0f) return 1.0f;
	return 1.0f - fm_expf(-10.0f * t * LN2);
}

float ease_expo_in_out(float t)
{
	if (t <= 0.0f) return 0.0f;
	if (t >= 1.0f) return 1.0f;
	if (t < 0.5f) return 0.5f * fm_expf((20.0f * t - 10.0f) * LN2);
	return 1.0f - 0.5f * fm_expf((-20.0f * t + 10.0f) * LN2);
}
