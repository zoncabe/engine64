#include <assert.h>
#include <math.h>
#include <float.h>

#include "../../../include/physics/math/math_common.h"


// function implemmentations


/* degrees to radians */
inline float rad(float angle)
{
	return PI / 180 * angle;
}

/* radians to degrees */
inline float deg(float rad)
{
	return 180 / PI * rad;
}


/* return the result of the "value" clamped by "lowerLimit" and "upperLimit" */
inline int clamp_int(int value, int lowerLimit, int upperLimit)
{
    assert(lowerLimit <= upperLimit);
    return (value < lowerLimit) ? lowerLimit : (value > upperLimit) ? upperLimit : value;
}

/* return the result of the "value" clamped by "lowerLimit" and "upperLimit" */
inline float clamp(float value, float lowerLimit, float upperLimit)
{
    assert(lowerLimit <= upperLimit);
    return (value < lowerLimit) ? lowerLimit : (value > upperLimit) ? upperLimit : value;
}

/* return higher value*/
inline float max2(float a, float b) 
{
    return (a > b) ? a : b;
}

/* return lower value*/
inline float min2(float a, float b) 
{
    return (a < b) ? a : b;
}

/* return the minimum value among three values */
inline float min3(float a, float b, float c)
{
    return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c);
}

/* return the maximum value among three values */
inline float max3(float a, float b, float c)
{
    return (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c);
}

/* return true if two values have the same sign */
inline bool sameSign(float a, float b)
{
    return a * b >= 0.0f;
}

/* Function to test if two real numbers are (almost) equal
We test if two numbers a and b are such that (a-b) are in [-EPSILON; EPSILON] */
inline bool approxEqual(float a, float b) 
{
    return (fabsf(a - b) < FLT_EPSILON);
}


inline bool isfinite(float x)
{
    return (x == x) && (x != INFINITY) && (x != -INFINITY);
}

/* wrap angle to [-180, 180) */
inline float angle_wrap(float angle)
{
    while (angle >  180.0f) angle -= 360.0f;
    while (angle <= -180.0f) angle += 360.0f;
    return angle;
}

/* wrap angle to the [-180, 180) range relative to a reference angle */
inline float angle_wrap_relative(float angle, float reference)
{
    while (angle >  reference + 180.0f) angle -= 360.0f;
    while (angle <= reference - 180.0f) angle += 360.0f;
    return angle;
}

inline float lerpf(float a, float b, float t)
{
    return a + t * (b - a);
}