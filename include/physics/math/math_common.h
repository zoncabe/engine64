#ifndef PHYSICS_MATH_COMMON_H
#define PHYSICS_MATH_COMMON_H

#define PI 3.141592f
#define PI_TIMES_2 6.283185f

#define TOLERANCE 0.000001f


float deg_to_rad(float angle);
float rad_to_deg(float rad);

float angle_wrap(float angle);
float angle_wrap_relative(float angle, float reference);

float lerpf(float a, float b, float t);

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
