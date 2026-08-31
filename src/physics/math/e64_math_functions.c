#include "physics/math/e64_math_functions.h"


#define EPSILON 1e-6f


float clampf(float v, float lo, float hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}


Vector3 segment_closestToPoint(const Vector3 *a, const Vector3 *b, const Vector3 *point)
{
	Vector3 ab = {b->x - a->x, b->y - a->y, b->z - a->z};
	Vector3 ap = {point->x - a->x, point->y - a->y, point->z - a->z};

	float ab_len_sq = ab.x*ab.x + ab.y*ab.y + ab.z*ab.z;
	if (ab_len_sq < EPSILON) return *a;

	float t = (ap.x*ab.x + ap.y*ab.y + ap.z*ab.z) / ab_len_sq;
	t = clampf(t, 0.0f, 1.0f);

	return (Vector3){
		a->x + t * ab.x,
		a->y + t * ab.y,
		a->z + t * ab.z,
	};
}


void segment_closestToSegment(
	const Vector3 *a1, const Vector3 *b1,
	const Vector3 *a2, const Vector3 *b2,
	Vector3 *closest1, Vector3 *closest2)
{
	Vector3 d1 = {b1->x - a1->x, b1->y - a1->y, b1->z - a1->z};
	Vector3 d2 = {b2->x - a2->x, b2->y - a2->y, b2->z - a2->z};
	Vector3 r  = {a1->x - a2->x, a1->y - a2->y, a1->z - a2->z};

	float a = d1.x*d1.x + d1.y*d1.y + d1.z*d1.z;
	float e = d2.x*d2.x + d2.y*d2.y + d2.z*d2.z;
	float f = d2.x*r.x  + d2.y*r.y  + d2.z*r.z;

	float s, t;

	if (a <= EPSILON && e <= EPSILON) {
		*closest1 = *a1;
		*closest2 = *a2;
		return;
	}

	if (a <= EPSILON) {
		s = 0.0f;
		t = clampf(f / e, 0.0f, 1.0f);
	}
	else {
		float c = d1.x*r.x + d1.y*r.y + d1.z*r.z;
		if (e <= EPSILON) {
			t = 0.0f;
			s = clampf(-c / a, 0.0f, 1.0f);
		}
		else {
			float b     = d1.x*d2.x + d1.y*d2.y + d1.z*d2.z;
			float denom = a * e - b * b;
			if (denom != 0.0f) s = clampf((b*f - c*e) / denom, 0.0f, 1.0f);
			else               s = 0.0f;
			t = (b*s + f) / e;
			if (t < 0.0f) {
				t = 0.0f;
				s = clampf(-c / a, 0.0f, 1.0f);
			}
			else if (t > 1.0f) {
				t = 1.0f;
				s = clampf((b - c) / a, 0.0f, 1.0f);
			}
		}
	}

	*closest1 = (Vector3){a1->x + d1.x*s, a1->y + d1.y*s, a1->z + d1.z*s};
	*closest2 = (Vector3){a2->x + d2.x*t, a2->y + d2.y*t, a2->z + d2.z*t};
}


void triangle_barycentric(
	const Vector3 *a, const Vector3 *b, const Vector3 *c,
	const Vector3 *point,
	float *u, float *v, float *w)
{
	Vector3 v0 = {b->x - a->x, b->y - a->y, b->z - a->z};
	Vector3 v1 = {c->x - a->x, c->y - a->y, c->z - a->z};
	Vector3 v2 = {point->x - a->x, point->y - a->y, point->z - a->z};

	float d00 = v0.x*v0.x + v0.y*v0.y + v0.z*v0.z;
	float d01 = v0.x*v1.x + v0.y*v1.y + v0.z*v1.z;
	float d11 = v1.x*v1.x + v1.y*v1.y + v1.z*v1.z;
	float d20 = v2.x*v0.x + v2.y*v0.y + v2.z*v0.z;
	float d21 = v2.x*v1.x + v2.y*v1.y + v2.z*v1.z;

	float denom = d00 * d11 - d01 * d01;
	if (denom < EPSILON) {
		*u = 1.0f; *v = 0.0f; *w = 0.0f;
		return;
	}

	*v = (d11 * d20 - d01 * d21) / denom;
	*w = (d00 * d21 - d01 * d20) / denom;
	*u = 1.0f - *v - *w;
}


Vector3 triangle_closestToPoint(
	const Vector3 *a, const Vector3 *b, const Vector3 *c,
	const Vector3 *point)
{
	Vector3 ab = {b->x - a->x, b->y - a->y, b->z - a->z};
	Vector3 ac = {c->x - a->x, c->y - a->y, c->z - a->z};
	Vector3 ap = {point->x - a->x, point->y - a->y, point->z - a->z};

	float d1 = ab.x*ap.x + ab.y*ap.y + ab.z*ap.z;
	float d2 = ac.x*ap.x + ac.y*ap.y + ac.z*ap.z;
	if (d1 <= 0.0f && d2 <= 0.0f) return *a;

	Vector3 bp = {point->x - b->x, point->y - b->y, point->z - b->z};
	float d3 = ab.x*bp.x + ab.y*bp.y + ab.z*bp.z;
	float d4 = ac.x*bp.x + ac.y*bp.y + ac.z*bp.z;
	if (d3 >= 0.0f && d4 <= d3) return *b;

	float vc = d1*d4 - d3*d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
		float v = d1 / (d1 - d3);
		return (Vector3){a->x + v*ab.x, a->y + v*ab.y, a->z + v*ab.z};
	}

	Vector3 cp = {point->x - c->x, point->y - c->y, point->z - c->z};
	float d5 = ab.x*cp.x + ab.y*cp.y + ab.z*cp.z;
	float d6 = ac.x*cp.x + ac.y*cp.y + ac.z*cp.z;
	if (d6 >= 0.0f && d5 <= d6) return *c;

	float vb = d5*d2 - d1*d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
		float w = d2 / (d2 - d6);
		return (Vector3){a->x + w*ac.x, a->y + w*ac.y, a->z + w*ac.z};
	}

	float va = d3*d6 - d5*d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		return (Vector3){
			b->x + w*(c->x - b->x),
			b->y + w*(c->y - b->y),
			b->z + w*(c->z - b->z),
		};
	}

	float denom = 1.0f / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;
	return (Vector3){
		a->x + ab.x*v + ac.x*w,
		a->y + ab.y*v + ac.y*w,
		a->z + ab.z*v + ac.z*w,
	};
}
