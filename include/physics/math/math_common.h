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
 * Fast inverse square root — Quake III original (Q_rsqrt) and Kaze's variant
 * (qi_sqrt).
 *
 * Returns an approximation of 1/sqrt(x) in ~6 cycles instead of the ~58 cycles
 * that 1.0f / sqrtf(x) costs on N64 (29 cycles sqrt + 29 cycles divide).
 *
 * Apply only when ALL THREE of the following hold:
 *   1. You need 1/sqrt(x), not sqrt(x). If you need sqrt, use the hardware sqrtf.
 *   2. The function lives in code already in icache (hot path). A cache miss
 *      loading the 8 extra instructions kills the gain.
 *   3. ~3% precision is acceptable. DO NOT use in physics, contact normals,
 *      raycasts or any accumulating computation.
 *
 * Typical use cases: normalizing vectors for shadows, background lighting,
 * non-critical visual effects, massive particle systems.
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
