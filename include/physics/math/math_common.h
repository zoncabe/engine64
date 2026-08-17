#ifndef PHYSICS_MATH_COMMON_H
#define PHYSICS_MATH_COMMON_H

#define PI 3.141592f
#define PI_TIMES_2 6.283185f

#define TOLERANCE 0.000001f

#define RENDER_SCALE      100.0f
#define RENDER_SCALE_INV  0.01f


float deg_to_rad(float angle);
float rad_to_deg(float rad);

float angle_wrap(float angle);
float angle_wrap_relative(float angle, float reference);

float lerpf(float a, float b, float t);

/*
	Fast inverse square root, Kaze's variant of the Quake III Q_rsqrt.
	Approximates 1/sqrt(x) in ~6 cycles instead of the ~58 that 1.0f / sqrtf(x)
	costs on N64 (29 for the sqrt, 29 for the divide).

	Only worth it when all three hold:
	  1. You need 1/sqrt(x), not sqrt(x). For sqrt use the hardware sqrtf.
	  2. The caller is already in icache. A miss loading the 8 extra
	     instructions kills the gain.
	  3. ~3% error is acceptable. Never in physics, contact normals, raycasts
	     or anything that accumulates.
*/
float qi_sqrt(float x);

float ease_linear(float t);

float ease_quad_in(float t);
float ease_quad_out(float t);
float ease_quad_in_out(float t);

float ease_cubic_in(float t);
float ease_cubic_out(float t);
float ease_cubic_in_out(float t);

float ease_expo_in(float t);
float ease_expo_out(float t);
float ease_expo_in_out(float t);


#endif
