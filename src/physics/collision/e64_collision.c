/*
	Ported from qu3e q3Collide.cpp — altered source, not the original software.

	Copyright (c) 2014 Randy Gaul http://www.randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
*/

/*
	Narrowphase. The OBB-vs-OBB SAT and Sutherland-Hodgman clip come from
	qu3e; the type-based dispatcher and the sphere / capsule / triangle pairs
	are added on top.
*/
#include <float.h>
#include <math.h>

#include "physics/collision/e64_collision.h"
#include "physics/collision/e64_collision_mesh.h"
#include "physics/body/e64_rigid_body.h"
#include "physics/math/e64_math_functions.h"    /* segment_closestToPoint */


static inline int trackFaceAxis(int32_t *axis, int32_t n, float s, float *s_max,
                                 Vector3 normal, Vector3 *axis_normal)
{
	if (s > 0.0f) return 1;
	if (s > *s_max) {
		*s_max       = s;
		*axis        = n;
		*axis_normal = normal;
	}
	return 0;
}


static inline int trackEdgeAxis(int32_t *axis, int32_t n, float s, float *s_max,
                                 Vector3 normal, Vector3 *axis_normal)
{
	if (s > 0.0f) return 1;
	float l = 1.0f / vector3_magnitude(&normal);
	s *= l;
	if (s > *s_max) {
		*s_max       = s;
		*axis        = n;
		*axis_normal = vector3_scaled(&normal, l);
	}
	return 0;
}


typedef struct ClipVertex {
	Vector3     v;
	FeaturePair f;
} ClipVertex;


static void computeReferenceEdgesAndBasis(Vector3 e_r, Transform rtx, Vector3 n, int32_t axis,
                                           uint8_t *out, Matrix3 *basis, Vector3 *e)
{
	n = matrix3_transformVectorTransposed(&rtx.rotation, &n);

	if (axis >= 3) axis -= 3;

	Vector3 neg_ex = vector3_inverted(&rtx.rotation.ex);
	Vector3 neg_ey = vector3_inverted(&rtx.rotation.ey);
	Vector3 neg_ez = vector3_inverted(&rtx.rotation.ez);

	switch (axis) {
	case 0:
		if (n.x > 0.0f) {
			out[0] = 1;  out[1] = 8;  out[2] = 7;  out[3] = 9;
			*e = vector3_create(e_r.y, e_r.z, e_r.x);
			matrix3_setRows(basis, &rtx.rotation.ey, &rtx.rotation.ez, &rtx.rotation.ex);
		} else {
			out[0] = 11; out[1] = 3;  out[2] = 10; out[3] = 5;
			*e = vector3_create(e_r.z, e_r.y, e_r.x);
			matrix3_setRows(basis, &rtx.rotation.ez, &rtx.rotation.ey, &neg_ex);
		}
		break;

	case 1:
		if (n.y > 0.0f) {
			out[0] = 0;  out[1] = 1;  out[2] = 2;  out[3] = 3;
			*e = vector3_create(e_r.z, e_r.x, e_r.y);
			matrix3_setRows(basis, &rtx.rotation.ez, &rtx.rotation.ex, &rtx.rotation.ey);
		} else {
			out[0] = 4;  out[1] = 5;  out[2] = 6;  out[3] = 7;
			*e = vector3_create(e_r.z, e_r.x, e_r.y);
			matrix3_setRows(basis, &rtx.rotation.ez, &neg_ex, &neg_ey);
		}
		break;

	case 2:
		if (n.z > 0.0f) {
			out[0] = 11; out[1] = 4;  out[2] = 8;  out[3] = 0;
			*e = vector3_create(e_r.y, e_r.x, e_r.z);
			matrix3_setRows(basis, &neg_ey, &rtx.rotation.ex, &rtx.rotation.ez);
		} else {
			out[0] = 6;  out[1] = 10; out[2] = 2;  out[3] = 9;
			*e = vector3_create(e_r.y, e_r.x, e_r.z);
			matrix3_setRows(basis, &neg_ey, &neg_ex, &neg_ez);
		}
		break;
	}
}


/* Corners of the incident face, on the box opposing the reference face. */
static void computeIncidentFace(Transform itx, Vector3 e, Vector3 n, ClipVertex *out)
{
	Vector3 n_local = matrix3_transformVectorTransposed(&itx.rotation, &n);
	n = vector3_inverted(&n_local);
	Vector3 abs_n = vector3_abs(&n);

	if (abs_n.x > abs_n.y && abs_n.x > abs_n.z) {
		if (n.x > 0.0f) {
			out[0].v = vector3_create( e.x,  e.y, -e.z);
			out[1].v = vector3_create( e.x,  e.y,  e.z);
			out[2].v = vector3_create( e.x, -e.y,  e.z);
			out[3].v = vector3_create( e.x, -e.y, -e.z);

			out[0].f.in_i = 9;  out[0].f.out_i = 1;
			out[1].f.in_i = 1;  out[1].f.out_i = 8;
			out[2].f.in_i = 8;  out[2].f.out_i = 7;
			out[3].f.in_i = 7;  out[3].f.out_i = 9;
		} else {
			out[0].v = vector3_create(-e.x, -e.y,  e.z);
			out[1].v = vector3_create(-e.x,  e.y,  e.z);
			out[2].v = vector3_create(-e.x,  e.y, -e.z);
			out[3].v = vector3_create(-e.x, -e.y, -e.z);

			out[0].f.in_i = 5;  out[0].f.out_i = 11;
			out[1].f.in_i = 11; out[1].f.out_i = 3;
			out[2].f.in_i = 3;  out[2].f.out_i = 10;
			out[3].f.in_i = 10; out[3].f.out_i = 5;
		}
	}
	else if (abs_n.y > abs_n.x && abs_n.y > abs_n.z) {
		if (n.y > 0.0f) {
			out[0].v = vector3_create(-e.x,  e.y,  e.z);
			out[1].v = vector3_create( e.x,  e.y,  e.z);
			out[2].v = vector3_create( e.x,  e.y, -e.z);
			out[3].v = vector3_create(-e.x,  e.y, -e.z);

			out[0].f.in_i = 3;  out[0].f.out_i = 0;
			out[1].f.in_i = 0;  out[1].f.out_i = 1;
			out[2].f.in_i = 1;  out[2].f.out_i = 2;
			out[3].f.in_i = 2;  out[3].f.out_i = 3;
		} else {
			out[0].v = vector3_create( e.x, -e.y,  e.z);
			out[1].v = vector3_create(-e.x, -e.y,  e.z);
			out[2].v = vector3_create(-e.x, -e.y, -e.z);
			out[3].v = vector3_create( e.x, -e.y, -e.z);

			out[0].f.in_i = 7;  out[0].f.out_i = 4;
			out[1].f.in_i = 4;  out[1].f.out_i = 5;
			out[2].f.in_i = 5;  out[2].f.out_i = 6;
			out[3].f.in_i = 5;  out[3].f.out_i = 6;
		}
	}
	else {
		if (n.z > 0.0f) {
			out[0].v = vector3_create(-e.x,  e.y,  e.z);
			out[1].v = vector3_create(-e.x, -e.y,  e.z);
			out[2].v = vector3_create( e.x, -e.y,  e.z);
			out[3].v = vector3_create( e.x,  e.y,  e.z);

			out[0].f.in_i = 0;  out[0].f.out_i = 11;
			out[1].f.in_i = 11; out[1].f.out_i = 4;
			out[2].f.in_i = 4;  out[2].f.out_i = 8;
			out[3].f.in_i = 8;  out[3].f.out_i = 0;
		} else {
			out[0].v = vector3_create( e.x, -e.y, -e.z);
			out[1].v = vector3_create(-e.x, -e.y, -e.z);
			out[2].v = vector3_create(-e.x,  e.y, -e.z);
			out[3].v = vector3_create( e.x,  e.y, -e.z);

			out[0].f.in_i = 9;  out[0].f.out_i = 6;
			out[1].f.in_i = 6;  out[1].f.out_i = 10;
			out[2].f.in_i = 10; out[2].f.out_i = 2;
			out[3].f.in_i = 2;  out[3].f.out_i = 9;
		}
	}

	for (int32_t i = 0; i < 4; ++i) {
		out[i].v = transform_mulVector(&itx, &out[i].v);
	}
}


/* Sutherland-Hodgman one-plane clip. */
#define IN_FRONT(a) ((a) < 0.0f)
#define BEHIND(a)   ((a) >= 0.0f)
#define ON_PLANE(a) ((a) < 0.005f && (a) > -0.005f)

static int32_t orthographic(float sign, float e, int32_t axis, int32_t clip_edge,
                             ClipVertex *in, int32_t in_count, ClipVertex *out)
{
	int32_t out_count = 0;
	ClipVertex a = in[in_count - 1];

	for (int32_t i = 0; i < in_count; ++i) {
		ClipVertex b = in[i];

		float a_axis = (axis == 0) ? a.v.x : (axis == 1) ? a.v.y : a.v.z;
		float b_axis = (axis == 0) ? b.v.x : (axis == 1) ? b.v.y : b.v.z;

		float da = sign * a_axis - e;
		float db = sign * b_axis - e;

		ClipVertex cv;

		if (((IN_FRONT(da) && IN_FRONT(db)) || ON_PLANE(da) || ON_PLANE(db))) {
			out[out_count++] = b;
		}
		else if (IN_FRONT(da) && BEHIND(db)) {
			Vector3 diff   = vector3_difference(&b.v, &a.v);
			Vector3 scaled = vector3_scaled(&diff, da / (da - db));
			cv.v           = vector3_sum(&a.v, &scaled);
			cv.f           = b.f;
			cv.f.out_r     = (uint8_t)clip_edge;
			cv.f.out_i     = 0;
			out[out_count++] = cv;
		}
		else if (BEHIND(da) && IN_FRONT(db)) {
			Vector3 diff   = vector3_difference(&b.v, &a.v);
			Vector3 scaled = vector3_scaled(&diff, da / (da - db));
			cv.v           = vector3_sum(&a.v, &scaled);
			cv.f           = a.f;
			cv.f.in_r      = (uint8_t)clip_edge;
			cv.f.in_i      = 0;
			out[out_count++] = cv;
			out[out_count++] = b;
		}

		a = b;
	}

	return out_count;
}


/* Clip the incident face against the reference face. */
static int32_t clipFace(Vector3 r_pos, Vector3 e, uint8_t *clip_edges, Matrix3 basis,
                         ClipVertex *incident, ClipVertex *out_verts, float *out_depths)
{
	int32_t in_count = 4;
	int32_t out_count;
	ClipVertex in[8];
	ClipVertex out[8];

	for (int32_t i = 0; i < 4; ++i) {
		Vector3 diff = vector3_difference(&incident[i].v, &r_pos);
		in[i].v = matrix3_transformVectorTransposed(&basis, &diff);
		in[i].f = incident[i].f;
	}

	out_count = orthographic( 1.0f, e.x, 0, clip_edges[0], in,  in_count,  out);
	if (!out_count) return 0;

	in_count  = orthographic( 1.0f, e.y, 1, clip_edges[1], out, out_count, in);
	if (!in_count) return 0;

	out_count = orthographic(-1.0f, e.x, 0, clip_edges[2], in,  in_count,  out);
	if (!out_count) return 0;

	in_count  = orthographic(-1.0f, e.y, 1, clip_edges[3], out, out_count, in);

	/* Keep only vertices behind the reference face. */
	out_count = 0;
	for (int32_t i = 0; i < in_count; ++i) {
		float d = in[i].v.z - e.z;
		if (d <= 0.0f) {
			Vector3 back = matrix3_transformVector(&basis, &in[i].v);
			out_verts[out_count].v = vector3_sum(&back, &r_pos);
			out_verts[out_count].f = in[i].f;
			out_depths[out_count++] = d;
		}
	}

	return out_count;
}


/* Closest points between two line segments (edge-edge contact). */
static void edgesContact(Vector3 *ca, Vector3 *cb,
                          Vector3 pa, Vector3 qa, Vector3 pb, Vector3 qb)
{
	Vector3 da = vector3_difference(&qa, &pa);
	Vector3 db = vector3_difference(&qb, &pb);
	Vector3 r  = vector3_difference(&pa, &pb);
	float a = vector3_dot(&da, &da);
	float e = vector3_dot(&db, &db);
	float f = vector3_dot(&db, &r);
	float c = vector3_dot(&da, &r);
	float b = vector3_dot(&da, &db);
	float denom = a * e - b * b;

	float ta = (b * f - c * e) / denom;
	float tb = (b * ta + f) / e;

	Vector3 scaled_a = vector3_scaled(&da, ta);
	Vector3 scaled_b = vector3_scaled(&db, tb);
	*ca = vector3_sum(&pa, &scaled_a);
	*cb = vector3_sum(&pb, &scaled_b);
}


/* Supporting edge of a box for the given direction n (world space). */
static void supportEdge(Transform tx, Vector3 e, Vector3 n, Vector3 *a_out, Vector3 *b_out)
{
	Vector3 n_local = matrix3_transformVectorTransposed(&tx.rotation, &n);
	Vector3 abs_n   = vector3_abs(&n_local);
	Vector3 a, b;

	if (abs_n.x > abs_n.y) {
		if (abs_n.y > abs_n.z) {
			a = vector3_create( e.x,  e.y,  e.z);
			b = vector3_create( e.x,  e.y, -e.z);
		} else {
			a = vector3_create( e.x,  e.y,  e.z);
			b = vector3_create( e.x, -e.y,  e.z);
		}
	} else {
		if (abs_n.x > abs_n.z) {
			a = vector3_create( e.x,  e.y,  e.z);
			b = vector3_create( e.x,  e.y, -e.z);
		} else {
			a = vector3_create( e.x,  e.y,  e.z);
			b = vector3_create(-e.x,  e.y,  e.z);
		}
	}

	float sign_x = (n_local.x >= 0.0f) ? 1.0f : -1.0f;
	float sign_y = (n_local.y >= 0.0f) ? 1.0f : -1.0f;
	float sign_z = (n_local.z >= 0.0f) ? 1.0f : -1.0f;

	a.x *= sign_x; a.y *= sign_y; a.z *= sign_z;
	b.x *= sign_x; b.y *= sign_y; b.z *= sign_z;

	*a_out = transform_mulVector(&tx, &a);
	*b_out = transform_mulVector(&tx, &b);
}


/* OBB-vs-OBB SAT plus face and edge clipping. */
void boxToBox(ContactManifold *m, PhysicsShape *a, PhysicsShape *b)
{
	Transform atx = rigidBody_getTransform(a->body);
	Transform btx = rigidBody_getTransform(b->body);
	atx = transform_product(&atx, &a->local);
	btx = transform_product(&btx, &b->local);
	Vector3 e_a = a->box.e;
	Vector3 e_b = b->box.e;

	/* B's frame in A's space. */
	Matrix3 atx_t = matrix3_transposed(&atx.rotation);
	Matrix3 C     = matrix3_product(&atx_t, &btx.rotation);

	Matrix3 abs_C;
	int     parallel = 0;
	const float k_cos_tol = 1.0e-6f;
	for (int32_t i = 0; i < 3; ++i) {
		for (int32_t j = 0; j < 3; ++j) {
			float val = fabsf(matrix3_get(&C, i, j));
			if (i == 0) {
				if (j == 0) abs_C.ex.x = val;
				else if (j == 1) abs_C.ex.y = val;
				else abs_C.ex.z = val;
			} else if (i == 1) {
				if (j == 0) abs_C.ey.x = val;
				else if (j == 1) abs_C.ey.y = val;
				else abs_C.ey.z = val;
			} else {
				if (j == 0) abs_C.ez.x = val;
				else if (j == 1) abs_C.ez.y = val;
				else abs_C.ez.z = val;
			}
			if (val + k_cos_tol >= 1.0f) parallel = 1;
		}
	}

	Vector3 b_minus_a = vector3_difference(&btx.position, &atx.position);
	Vector3 t         = matrix3_transformVectorTransposed(&atx.rotation, &b_minus_a);

	float s;
	float a_max = -FLT_MAX;
	float b_max = -FLT_MAX;
	float e_max = -FLT_MAX;
	int32_t a_axis = ~0;
	int32_t b_axis = ~0;
	int32_t e_axis = ~0;
	Vector3 n_a = vector3_zero();
	Vector3 n_b = vector3_zero();
	Vector3 n_e = vector3_zero();

	Vector3 col0 = matrix3_column0(&abs_C);
	Vector3 col1 = matrix3_column1(&abs_C);
	Vector3 col2 = matrix3_column2(&abs_C);

	/* Face axes of A. */
	s = fabsf(t.x) - (e_a.x + vector3_dot(&col0, &e_b));
	if (trackFaceAxis(&a_axis, 0, s, &a_max, atx.rotation.ex, &n_a)) return;

	s = fabsf(t.y) - (e_a.y + vector3_dot(&col1, &e_b));
	if (trackFaceAxis(&a_axis, 1, s, &a_max, atx.rotation.ey, &n_a)) return;

	s = fabsf(t.z) - (e_a.z + vector3_dot(&col2, &e_b));
	if (trackFaceAxis(&a_axis, 2, s, &a_max, atx.rotation.ez, &n_a)) return;

	/* Face axes of B. */
	s = fabsf(vector3_dot(&t, &C.ex)) - (e_b.x + vector3_dot(&abs_C.ex, &e_a));
	if (trackFaceAxis(&b_axis, 3, s, &b_max, btx.rotation.ex, &n_b)) return;

	s = fabsf(vector3_dot(&t, &C.ey)) - (e_b.y + vector3_dot(&abs_C.ey, &e_a));
	if (trackFaceAxis(&b_axis, 4, s, &b_max, btx.rotation.ey, &n_b)) return;

	s = fabsf(vector3_dot(&t, &C.ez)) - (e_b.z + vector3_dot(&abs_C.ez, &e_a));
	if (trackFaceAxis(&b_axis, 5, s, &b_max, btx.rotation.ez, &n_b)) return;

	if (!parallel) {
		float r_a, r_b;
		Vector3 n;

		/* Cross(a.x, b.x) */
		r_a = e_a.y * matrix3_get(&abs_C, 0, 2) + e_a.z * matrix3_get(&abs_C, 0, 1);
		r_b = e_b.y * matrix3_get(&abs_C, 2, 0) + e_b.z * matrix3_get(&abs_C, 1, 0);
		s   = fabsf(t.z * matrix3_get(&C, 0, 1) - t.y * matrix3_get(&C, 0, 2)) - (r_a + r_b);
		n = vector3_create(0.0f, -matrix3_get(&C, 0, 2), matrix3_get(&C, 0, 1));
		if (trackEdgeAxis(&e_axis, 6, s, &e_max, n, &n_e)) return;

		/* Cross(a.x, b.y) */
		r_a = e_a.y * matrix3_get(&abs_C, 1, 2) + e_a.z * matrix3_get(&abs_C, 1, 1);
		r_b = e_b.x * matrix3_get(&abs_C, 2, 0) + e_b.z * matrix3_get(&abs_C, 0, 0);
		s   = fabsf(t.z * matrix3_get(&C, 1, 1) - t.y * matrix3_get(&C, 1, 2)) - (r_a + r_b);
		n = vector3_create(0.0f, -matrix3_get(&C, 1, 2), matrix3_get(&C, 1, 1));
		if (trackEdgeAxis(&e_axis, 7, s, &e_max, n, &n_e)) return;

		/* Cross(a.x, b.z) */
		r_a = e_a.y * matrix3_get(&abs_C, 2, 2) + e_a.z * matrix3_get(&abs_C, 2, 1);
		r_b = e_b.x * matrix3_get(&abs_C, 1, 0) + e_b.y * matrix3_get(&abs_C, 0, 0);
		s   = fabsf(t.z * matrix3_get(&C, 2, 1) - t.y * matrix3_get(&C, 2, 2)) - (r_a + r_b);
		n = vector3_create(0.0f, -matrix3_get(&C, 2, 2), matrix3_get(&C, 2, 1));
		if (trackEdgeAxis(&e_axis, 8, s, &e_max, n, &n_e)) return;

		/* Cross(a.y, b.x) */
		r_a = e_a.x * matrix3_get(&abs_C, 0, 2) + e_a.z * matrix3_get(&abs_C, 0, 0);
		r_b = e_b.y * matrix3_get(&abs_C, 2, 1) + e_b.z * matrix3_get(&abs_C, 1, 1);
		s   = fabsf(t.x * matrix3_get(&C, 0, 2) - t.z * matrix3_get(&C, 0, 0)) - (r_a + r_b);
		n = vector3_create(matrix3_get(&C, 0, 2), 0.0f, -matrix3_get(&C, 0, 0));
		if (trackEdgeAxis(&e_axis, 9, s, &e_max, n, &n_e)) return;

		/* Cross(a.y, b.y) */
		r_a = e_a.x * matrix3_get(&abs_C, 1, 2) + e_a.z * matrix3_get(&abs_C, 1, 0);
		r_b = e_b.x * matrix3_get(&abs_C, 2, 1) + e_b.z * matrix3_get(&abs_C, 0, 1);
		s   = fabsf(t.x * matrix3_get(&C, 1, 2) - t.z * matrix3_get(&C, 1, 0)) - (r_a + r_b);
		n = vector3_create(matrix3_get(&C, 1, 2), 0.0f, -matrix3_get(&C, 1, 0));
		if (trackEdgeAxis(&e_axis, 10, s, &e_max, n, &n_e)) return;

		/* Cross(a.y, b.z) */
		r_a = e_a.x * matrix3_get(&abs_C, 2, 2) + e_a.z * matrix3_get(&abs_C, 2, 0);
		r_b = e_b.x * matrix3_get(&abs_C, 1, 1) + e_b.y * matrix3_get(&abs_C, 0, 1);
		s   = fabsf(t.x * matrix3_get(&C, 2, 2) - t.z * matrix3_get(&C, 2, 0)) - (r_a + r_b);
		n = vector3_create(matrix3_get(&C, 2, 2), 0.0f, -matrix3_get(&C, 2, 0));
		if (trackEdgeAxis(&e_axis, 11, s, &e_max, n, &n_e)) return;

		/* Cross(a.z, b.x) */
		r_a = e_a.x * matrix3_get(&abs_C, 0, 1) + e_a.y * matrix3_get(&abs_C, 0, 0);
		r_b = e_b.y * matrix3_get(&abs_C, 2, 2) + e_b.z * matrix3_get(&abs_C, 1, 2);
		s   = fabsf(t.y * matrix3_get(&C, 0, 0) - t.x * matrix3_get(&C, 0, 1)) - (r_a + r_b);
		n = vector3_create(-matrix3_get(&C, 0, 1), matrix3_get(&C, 0, 0), 0.0f);
		if (trackEdgeAxis(&e_axis, 12, s, &e_max, n, &n_e)) return;

		/* Cross(a.z, b.y) */
		r_a = e_a.x * matrix3_get(&abs_C, 1, 1) + e_a.y * matrix3_get(&abs_C, 1, 0);
		r_b = e_b.x * matrix3_get(&abs_C, 2, 2) + e_b.z * matrix3_get(&abs_C, 0, 2);
		s   = fabsf(t.y * matrix3_get(&C, 1, 0) - t.x * matrix3_get(&C, 1, 1)) - (r_a + r_b);
		n = vector3_create(-matrix3_get(&C, 1, 1), matrix3_get(&C, 1, 0), 0.0f);
		if (trackEdgeAxis(&e_axis, 13, s, &e_max, n, &n_e)) return;

		/* Cross(a.z, b.z) */
		r_a = e_a.x * matrix3_get(&abs_C, 2, 1) + e_a.y * matrix3_get(&abs_C, 2, 0);
		r_b = e_b.x * matrix3_get(&abs_C, 1, 2) + e_b.y * matrix3_get(&abs_C, 0, 2);
		s   = fabsf(t.y * matrix3_get(&C, 2, 0) - t.x * matrix3_get(&C, 2, 1)) - (r_a + r_b);
		n = vector3_create(-matrix3_get(&C, 2, 1), matrix3_get(&C, 2, 0), 0.0f);
		if (trackEdgeAxis(&e_axis, 14, s, &e_max, n, &n_e)) return;
	}

	/* Pick the SAT axis, biased to avoid flipping between frames. */
	const float k_rel_tol = 0.95f;
	const float k_abs_tol = 0.01f;
	int32_t axis;
	float   s_max;
	Vector3 n;
	float face_max = (a_max > b_max) ? a_max : b_max;
	if (k_rel_tol * e_max > face_max + k_abs_tol) {
		axis = e_axis; s_max = e_max; n = n_e;
	} else {
		if (k_rel_tol * b_max > a_max + k_abs_tol) {
			axis = b_axis; s_max = b_max; n = n_b;
		} else {
			axis = a_axis; s_max = a_max; n = n_a;
		}
	}

	Vector3 pos_diff = vector3_difference(&btx.position, &atx.position);
	if (vector3_dot(&n, &pos_diff) < 0.0f) {
		n = vector3_inverted(&n);
	}

	if (axis == ~0) return;

	if (axis < 6) {
		Transform rtx;
		Transform itx;
		Vector3   e_r;
		Vector3   e_i;
		int       flip;

		if (axis < 3) {
			rtx = atx; itx = btx; e_r = e_a; e_i = e_b; flip = 0;
		} else {
			rtx = btx; itx = atx; e_r = e_b; e_i = e_a; flip = 1;
			n = vector3_inverted(&n);
		}

		ClipVertex incident[4];
		computeIncidentFace(itx, e_i, n, incident);
		uint8_t clip_edges[4] = { 0, 0, 0, 0 };
		Matrix3 basis         = matrix3_identity();
		Vector3 e             = vector3_zero();
		computeReferenceEdgesAndBasis(e_r, rtx, n, axis, clip_edges, &basis, &e);

		ClipVertex out[8];
		float depths[8];
		int32_t out_num = clipFace(rtx.position, e, clip_edges, basis, incident, out, depths);

		if (out_num) {
			m->contact_count = out_num;
			m->normal        = flip ? vector3_inverted(&n) : n;

			for (int32_t i = 0; i < out_num; ++i) {
				ContactPoint *c = m->contacts + i;

				FeaturePair pair = out[i].f;
				if (flip) {
					uint8_t tmp;
					tmp = pair.in_i;  pair.in_i  = pair.in_r;  pair.in_r  = tmp;
					tmp = pair.out_i; pair.out_i = pair.out_r; pair.out_r = tmp;
				}

				c->fp          = pair;
				c->position    = out[i].v;
				c->penetration = depths[i];
			}
		}
		(void)s_max;
	}
	else {
		n = matrix3_transformVector(&atx.rotation, &n);

		Vector3 pos_diff2 = vector3_difference(&btx.position, &atx.position);
		if (vector3_dot(&n, &pos_diff2) < 0.0f) {
			n = vector3_inverted(&n);
		}

		Vector3 pa, qa, pb, qb;
		Vector3 neg_n = vector3_inverted(&n);
		supportEdge(atx, e_a, n,     &pa, &qa);
		supportEdge(btx, e_b, neg_n, &pb, &qb);

		Vector3 ca, cb;
		edgesContact(&ca, &cb, pa, qa, pb, qb);

		m->normal        = n;
		m->contact_count = 1;

		ContactPoint *c = m->contacts;
		FeaturePair pair;
		pair.key       = axis;
		c->fp          = pair;
		c->penetration = s_max;
		Vector3 sum    = vector3_sum(&ca, &cb);
		c->position    = vector3_scaled(&sum, 0.5f);
	}
}


/* Tier 2: single-contact pairs (sphere/capsule). Normal points A→B. Ported
   from the old engine, which handled every non-box shape by transforming into
   box-local space and leveraging the AABB primitives. */


void sphereToSphere(ContactManifold *m, PhysicsShape *a, PhysicsShape *b)
{
	Transform atx = rigidBody_getTransform(a->body);
	Transform btx = rigidBody_getTransform(b->body);
	atx = transform_product(&atx, &a->local);
	btx = transform_product(&btx, &b->local);

	Vector3 d     = vector3_difference(&btx.position, &atx.position);
	float   rsum  = a->sphere.radius + b->sphere.radius;
	float   dist2 = vector3_dot(&d, &d);
	if (dist2 > rsum * rsum) return;

	float dist = sqrtf(dist2);
	Vector3 n = (dist > 1.0e-6f)
		? vector3_scaled(&d, 1.0f / dist)
		: vector3_create(0.0f, 0.0f, 1.0f);

	m->normal        = n;
	m->contact_count = 1;
	ContactPoint *c  = m->contacts;
	Vector3 off      = vector3_scaled(&n, a->sphere.radius);
	c->position      = vector3_sum(&atx.position, &off);
	c->penetration   = dist - rsum;
	c->fp.key        = 0;
}


/* Sphere is A, box is B. */
void sphereToBox(ContactManifold *m, PhysicsShape *a, PhysicsShape *b)
{
	Transform stx = rigidBody_getTransform(a->body);
	Transform btx = rigidBody_getTransform(b->body);
	stx = transform_product(&stx, &a->local);
	btx = transform_product(&btx, &b->local);

	float   r = a->sphere.radius;
	Vector3 e = b->box.e;
	AABB    box_local = { { -e.x, -e.y, -e.z }, { e.x, e.y, e.z } };

	/* Sphere center in box-local space. Closest point on the local AABB. */
	Vector3 s_local   = transform_mulVectorTransposed(&btx, &stx.position);
	Vector3 c_local   = aabb_closestToPoint(&box_local, &s_local);
	Vector3 d_local   = vector3_difference(&s_local, &c_local);
	float   dist2     = vector3_dot(&d_local, &d_local);
	if (dist2 > r * r) return;

	/* Bring the contact point back to world, build normal sphere→box. */
	Vector3 c_world = transform_mulVector(&btx, &c_local);
	Vector3 normal  = vector3_difference(&c_world, &stx.position);
	float   len     = vector3_magnitude(&normal);
	if (len > 1.0e-6f) normal = vector3_scaled(&normal, 1.0f / len);
	else               normal = vector3_create(0.0f, 0.0f, 1.0f);

	m->normal        = normal;
	m->contact_count = 1;
	ContactPoint *c  = m->contacts;
	c->position      = c_world;
	c->penetration   = len - r;   /* negative = overlap */
	c->fp.key        = 0;
}


/* Capsule is A, box is B. */
void capsuleToBox(ContactManifold *m, PhysicsShape *a, PhysicsShape *b)
{
	Transform atx = rigidBody_getTransform(a->body);
	Transform btx = rigidBody_getTransform(b->body);
	atx = transform_product(&atx, &a->local);
	btx = transform_product(&btx, &b->local);

	float   r = a->capsule.radius;
	float   h = a->capsule.half_height;
	Vector3 local_top    = { 0.0f, 0.0f,  h };
	Vector3 local_bottom = { 0.0f, 0.0f, -h };

	/* Capsule segment endpoints in box-local space. */
	Vector3 top_world    = transform_mulVector(&atx, &local_top);
	Vector3 bot_world    = transform_mulVector(&atx, &local_bottom);
	Vector3 top_local    = transform_mulVectorTransposed(&btx, &top_world);
	Vector3 bot_local    = transform_mulVectorTransposed(&btx, &bot_world);

	Vector3 e = b->box.e;
	AABB    box_local = { { -e.x, -e.y, -e.z }, { e.x, e.y, e.z } };

	Vector3 c_on_box = aabb_closestToSegment(&box_local, &bot_local, &top_local);
	Vector3 c_on_seg = segment_closestToPoint(&bot_local, &top_local, &c_on_box);
	Vector3 d_local  = vector3_difference(&c_on_seg, &c_on_box);
	float   dist2    = vector3_dot(&d_local, &d_local);
	if (dist2 > r * r) return;

	/* Contact lives on the box surface; normal goes capsule→box. */
	Vector3 box_world     = transform_mulVector(&btx, &c_on_box);
	Vector3 seg_world     = transform_mulVector(&btx, &c_on_seg);
	Vector3 to_box        = vector3_difference(&box_world, &seg_world);
	float   len           = vector3_magnitude(&to_box);
	Vector3 normal;
	if (len > 1.0e-6f) normal = vector3_scaled(&to_box, 1.0f / len);
	else               normal = vector3_create(0.0f, 0.0f, 1.0f);

	m->normal        = normal;
	m->contact_count = 1;
	ContactPoint *c  = m->contacts;
	c->position      = box_world;
	c->penetration   = len - r;
	c->fp.key        = 0;
}


/* Sphere is A, capsule is B. Treat the capsule as its segment plus radius:
   closest-point-on-segment reduces the pair to sphere-vs-sphere. */
void sphereToCapsule(ContactManifold *m, PhysicsShape *a, PhysicsShape *b)
{
	Transform stx = rigidBody_getTransform(a->body);
	Transform ctx = rigidBody_getTransform(b->body);
	stx = transform_product(&stx, &a->local);
	ctx = transform_product(&ctx, &b->local);

	float   sr  = a->sphere.radius;
	float   cr  = b->capsule.radius;
	float   h   = b->capsule.half_height;

	Vector3 local_top    = { 0.0f, 0.0f,  h };
	Vector3 local_bottom = { 0.0f, 0.0f, -h };
	Vector3 top = transform_mulVector(&ctx, &local_top);
	Vector3 bot = transform_mulVector(&ctx, &local_bottom);

	Vector3 on_seg = segment_closestToPoint(&bot, &top, &stx.position);
	Vector3 d      = vector3_difference(&on_seg, &stx.position);
	float   rsum   = sr + cr;
	float   dist2  = vector3_dot(&d, &d);
	if (dist2 > rsum * rsum) return;

	float dist = sqrtf(dist2);
	Vector3 n = (dist > 1.0e-6f)
		? vector3_scaled(&d, 1.0f / dist)
		: vector3_create(0.0f, 0.0f, 1.0f);

	m->normal        = n;
	m->contact_count = 1;
	ContactPoint *c  = m->contacts;
	Vector3 off      = vector3_scaled(&n, sr);
	c->position      = vector3_sum(&stx.position, &off);
	c->penetration   = dist - rsum;
	c->fp.key        = 0;
}


/* Closest point between the two inner segments. */
void capsuleToCapsule(ContactManifold *m, PhysicsShape *a, PhysicsShape *b)
{
	Transform atx = rigidBody_getTransform(a->body);
	Transform btx = rigidBody_getTransform(b->body);
	atx = transform_product(&atx, &a->local);
	btx = transform_product(&btx, &b->local);

	float ra = a->capsule.radius;
	float rb = b->capsule.radius;

	Vector3 la_top = { 0.0f, 0.0f,  a->capsule.half_height };
	Vector3 la_bot = { 0.0f, 0.0f, -a->capsule.half_height };
	Vector3 lb_top = { 0.0f, 0.0f,  b->capsule.half_height };
	Vector3 lb_bot = { 0.0f, 0.0f, -b->capsule.half_height };
	Vector3 pa1 = transform_mulVector(&atx, &la_bot);
	Vector3 pa2 = transform_mulVector(&atx, &la_top);
	Vector3 pb1 = transform_mulVector(&btx, &lb_bot);
	Vector3 pb2 = transform_mulVector(&btx, &lb_top);

	/* Iterate closest-point both ways a couple of times — good enough here. */
	Vector3 ca = pa1;
	Vector3 cb = segment_closestToPoint(&pb1, &pb2, &ca);
	ca = segment_closestToPoint(&pa1, &pa2, &cb);
	cb = segment_closestToPoint(&pb1, &pb2, &ca);

	Vector3 d    = vector3_difference(&cb, &ca);
	float   rsum = ra + rb;
	float   d2   = vector3_dot(&d, &d);
	if (d2 > rsum * rsum) return;

	float dist = sqrtf(d2);
	Vector3 n = (dist > 1.0e-6f)
		? vector3_scaled(&d, 1.0f / dist)
		: vector3_create(0.0f, 0.0f, 1.0f);

	m->normal        = n;
	m->contact_count = 1;
	ContactPoint *c  = m->contacts;
	Vector3 off      = vector3_scaled(&n, ra);
	c->position      = vector3_sum(&ca, &off);
	c->penetration   = dist - rsum;
	c->fp.key        = 0;
}


/* Without a RigidBody: static geometry placed by a world transform. Same
   logic as capsuleToBox. Capsule is A, box is B. */
void capsuleToStaticBox(ContactManifold *m, const Capsule *capsule, const Transform *capsule_world,
                        const Box *box, const Transform *box_world)
{
	float   r = capsule->radius;
	float   h = capsule->half_height;
	Vector3 local_top    = { 0.0f, 0.0f,  h };
	Vector3 local_bottom = { 0.0f, 0.0f, -h };

	/* Capsule segment endpoints in box-local space. */
	Vector3 top_world = transform_mulVector(capsule_world, &local_top);
	Vector3 bot_world = transform_mulVector(capsule_world, &local_bottom);
	Vector3 top_local = transform_mulVectorTransposed(box_world, &top_world);
	Vector3 bot_local = transform_mulVectorTransposed(box_world, &bot_world);

	Vector3 e = box->e;
	AABB    box_local = { { -e.x, -e.y, -e.z }, { e.x, e.y, e.z } };

	Vector3 c_on_box = aabb_closestToSegment(&box_local, &bot_local, &top_local);
	Vector3 c_on_seg = segment_closestToPoint(&bot_local, &top_local, &c_on_box);
	Vector3 d_local  = vector3_difference(&c_on_seg, &c_on_box);
	float   dist2    = vector3_dot(&d_local, &d_local);
	if (dist2 > r * r) return;

	/* Contact lives on the box surface; normal goes capsule→box. */
	Vector3 box_point = transform_mulVector(box_world, &c_on_box);
	Vector3 seg_point = transform_mulVector(box_world, &c_on_seg);
	Vector3 to_box    = vector3_difference(&box_point, &seg_point);
	float   len       = vector3_magnitude(&to_box);
	Vector3 normal;
	if (len > 1.0e-6f) normal = vector3_scaled(&to_box, 1.0f / len);
	else               normal = vector3_create(0.0f, 0.0f, 1.0f);

	m->normal        = normal;
	m->contact_count = 1;
	ContactPoint *c  = m->contacts;
	c->position      = box_point;
	c->penetration   = len - r;
	c->fp.key        = 0;
}


/* Without a RigidBody. Capsule is A, sphere is B. */
void capsuleToStaticSphere(ContactManifold *m, const Capsule *capsule, const Transform *capsule_world,
                           const Sphere *sphere, const Transform *sphere_world)
{
	Vector3 top, bot;
	capsule_getSegment(capsule, capsule_world, &bot, &top);

	Vector3 on_seg = segment_closestToPoint(&bot, &top, &sphere_world->position);
	Vector3 d      = vector3_difference(&sphere_world->position, &on_seg);
	float   rsum   = capsule->radius + sphere->radius;
	float   dist2  = vector3_dot(&d, &d);
	if (dist2 > rsum * rsum) return;

	float dist = sqrtf(dist2);
	Vector3 n = (dist > 1.0e-6f)
		? vector3_scaled(&d, 1.0f / dist)
		: vector3_create(0.0f, 0.0f, 1.0f);

	m->normal        = n;
	m->contact_count = 1;
	ContactPoint *c  = m->contacts;
	Vector3 off      = vector3_scaled(&n, -sphere->radius);
	c->position      = vector3_sum(&sphere_world->position, &off);
	c->penetration   = dist - rsum;
	c->fp.key        = 0;
}


/* Without a RigidBody. A is the moving capsule. */
void capsuleToStaticCapsule(ContactManifold *m, const Capsule *capsule, const Transform *capsule_world,
                            const Capsule *other, const Transform *other_world)
{
	Vector3 pa1, pa2, pb1, pb2;
	capsule_getSegment(capsule, capsule_world, &pa1, &pa2);
	capsule_getSegment(other, other_world, &pb1, &pb2);

	Vector3 ca, cb;
	segment_closestToSegment(&pa1, &pa2, &pb1, &pb2, &ca, &cb);

	Vector3 d    = vector3_difference(&cb, &ca);
	float   rsum = capsule->radius + other->radius;
	float   d2   = vector3_dot(&d, &d);
	if (d2 > rsum * rsum) return;

	float dist = sqrtf(d2);
	Vector3 n = (dist > 1.0e-6f)
		? vector3_scaled(&d, 1.0f / dist)
		: vector3_create(0.0f, 0.0f, 1.0f);

	m->normal        = n;
	m->contact_count = 1;
	ContactPoint *c  = m->contacts;
	Vector3 off      = vector3_scaled(&n, -other->radius);
	c->position      = vector3_sum(&cb, &off);
	c->penetration   = dist - rsum;
	c->fp.key        = 0;
}


/* The capsule is A, the triangle is B. Reference point on the segment via
   plane intersection, then one closest-point refinement. */
void capsuleToTriangle(ContactManifold *m, const Capsule *capsule, const Transform *world,
                       const Triangle *triangle)
{
	const Vector3 *vertices        = triangle->vertices;
	const Vector3 *triangle_normal = &triangle->normal;

	float r = capsule->radius;
	float h = capsule->half_height;

	Vector3 local_top    = { 0.0f, 0.0f,  h };
	Vector3 local_bottom = { 0.0f, 0.0f, -h };
	Vector3 top = transform_mulVector(world, &local_top);
	Vector3 bot = transform_mulVector(world, &local_bottom);

	/* Reference point: where the segment crosses the triangle plane. */
	Vector3 seg   = vector3_difference(&top, &bot);
	Vector3 to_v0 = vector3_difference(&vertices[0], &bot);
	float   denom = vector3_dot(triangle_normal, &seg);
	float   t     = 0.0f;
	if (fabsf(denom) > 1.0e-6f)
		t = clampf(vector3_dot(triangle_normal, &to_v0) / denom, 0.0f, 1.0f);
	Vector3 ref = bot;
	vector3_addScaledVector(&ref, &seg, t);

	Vector3 tri_pt = triangle_closestToPoint(&vertices[0], &vertices[1], &vertices[2], &ref);
	Vector3 center = segment_closestToPoint(&bot, &top, &tri_pt);
	tri_pt = triangle_closestToPoint(&vertices[0], &vertices[1], &vertices[2], &center);

	Vector3 d     = vector3_difference(&tri_pt, &center);
	float   dist2 = vector3_dot(&d, &d);
	if (dist2 > r * r) return;

	float dist = sqrtf(dist2);
	Vector3 n = (dist > 1.0e-6f)
		? vector3_scaled(&d, 1.0f / dist)
		: vector3_inverted(triangle_normal);

	m->normal        = n;
	m->contact_count = 1;
	ContactPoint *c  = m->contacts;
	Vector3 off      = vector3_scaled(&n, r);
	c->position      = vector3_sum(&center, &off);
	c->penetration   = dist - r;
	c->fp.key        = 0;
}


/*
	Contact normal correction on inactive triangle edges, ported from Jolt's
	ActiveEdges::FixNormal and ClosestPoint::GetBaryCentricCoordinates
	(JoltPhysics, MIT licensed). A contact landing on an internal seam of the
	mesh carries a normal pointing from the edge to the capsule axis instead
	of the surface normal; here it is replaced by the face normal, so sliding
	over a triangulated floor or ramp does not read as hitting a wall.
*/

/* Barycentric coordinates of the origin inside triangle (a, b, c), built on
   the two shortest edges for accuracy. False on a degenerate triangle — the
   importer drops those, so it cannot trigger on baked meshes. */
static bool collision_baryCentric(const Vector3 *a, const Vector3 *b, const Vector3 *c,
                                  float *u, float *v, float *w)
{
	Vector3 v0 = vector3_difference(b, a);
	Vector3 v1 = vector3_difference(c, a);
	Vector3 v2 = vector3_difference(c, b);

	float d00 = vector3_dot(&v0, &v0);
	float d11 = vector3_dot(&v1, &v1);
	float d22 = vector3_dot(&v2, &v2);

	if (d00 <= d22) {
		float d01 = vector3_dot(&v0, &v1);
		float denominator = d00 * d11 - d01 * d01;
		if (denominator < 1.0e-12f) return false;

		float a0 = vector3_dot(a, &v0);
		float a1 = vector3_dot(a, &v1);
		*v = (d01 * a1 - d11 * a0) / denominator;
		*w = (d01 * a0 - d00 * a1) / denominator;
		*u = 1.0f - *v - *w;
	}
	else {
		float d12 = vector3_dot(&v1, &v2);
		float denominator = d11 * d22 - d12 * d12;
		if (denominator < 1.0e-12f) return false;

		float c1 = vector3_dot(c, &v1);
		float c2 = vector3_dot(c, &v2);
		*u = (d22 * c1 - d12 * c2) / denominator;
		*v = (d11 * c2 - d12 * c1) / denominator;
		*w = 1.0f - *u - *v;
	}
	return true;
}

/* normal and the returned normal follow the manifold convention: unit, from
   the capsule toward the surface. point is the contact point on the triangle,
   in the same space as the triangle. movement_direction is the capsule's
   motion and may be zero; it separates sliding over a seam (use the face
   normal) from grazing a real wall through an inactive edge (keep the
   contact normal, the face normal would bounce the capsule back). */
Vector3 collision_fixTriangleNormal(const Triangle *triangle, const Vector3 *point,
                                    const Vector3 *normal, const Vector3 *movement_direction)
{
	/* All edges active: the normal is already correct. */
	if ((triangle->active_edges & 0x7) == 0x7) return *normal;

	/* Face normal in the manifold's convention: toward the surface. */
	Vector3 face_normal = vector3_inverted(&triangle->normal);

	/* If normal would affect movement less than the face normal, keep it. */
	if (vector3_dot(movement_direction, normal) < vector3_dot(movement_direction, &face_normal))
		return *normal;

	/* None of the edges are active: the face normal is the only real one. */
	if (triangle->active_edges == 0) return face_normal;

	/* Some edges are active. Parallel to the face: no need to check them. */
	if (vector3_dot(&face_normal, normal) > 0.999848f)   /* cos(1 degree) */
		return *normal;

	const float epsilon = 1.0e-4f;
	const float one_minus_epsilon = 1.0f - epsilon;

	/* Where the contact point sits in the triangle: vertex, edge or interior.
	   The coordinates are of the origin relative to the shifted vertices. */
	Vector3 a = vector3_difference(&triangle->vertices[0], point);
	Vector3 b = vector3_difference(&triangle->vertices[1], point);
	Vector3 c = vector3_difference(&triangle->vertices[2], point);

	float u, v, w;
	if (!collision_baryCentric(&a, &b, &c, &u, &v, &w)) return face_normal;

	uint8_t colliding_edge;
	if      (u > one_minus_epsilon) colliding_edge = 0x5;   /* vertex v0: edge 0 or 2 */
	else if (v > one_minus_epsilon) colliding_edge = 0x3;   /* vertex v1: edge 0 or 1 */
	else if (w > one_minus_epsilon) colliding_edge = 0x6;   /* vertex v2: edge 1 or 2 */
	else if (u < epsilon)           colliding_edge = 0x2;   /* edge v1v2 */
	else if (v < epsilon)           colliding_edge = 0x4;   /* edge v2v0 */
	else if (w < epsilon)           colliding_edge = 0x1;   /* edge v0v1 */
	else return face_normal;                                /* interior hit */

	return (triangle->active_edges & colliding_edge) ? *normal : face_normal;
}


/* Convex shape against a static triangle mesh.

   A mesh is not a convex piece, so it cannot be fed to the SAT routines above.
   Instead the shape's AABB queries the mesh tree and every triangle it touches
   is resolved on its own, with the results merged into the one manifold the
   contact holds. That manifold carries a single normal, so the deepest contact
   sets it: on a floor or a ramp all the triangles agree anyway, and where they
   do not, the deepest one is the constraint that matters. */

#define MESH_QUERY_MAX 24

typedef struct {
	const CollisionMesh *mesh;
	int32_t              triangle[MESH_QUERY_MAX];
	int32_t              count;
} MeshQuery;


static int collision_collectTriangle(void *cb, int32_t id)
{
	MeshQuery *query = cb;
	if (query->count >= MESH_QUERY_MAX) return 0;

	query->triangle[query->count++] =
		(int32_t)(intptr_t)dynamicAABBTree_getUserData(&query->mesh->tree, id);
	return 1;
}


/* Closest point on the triangle to the sphere centre: exact, and the only
   test a sphere needs. */
static void sphereToTriangle(ContactManifold *m, const Sphere *sphere, const Vector3 *center,
                             const Triangle *triangle)
{
	Vector3 closest = triangle_closestToPoint(&triangle->vertices[0], &triangle->vertices[1],
	                                          &triangle->vertices[2], center);
	Vector3 d     = vector3_difference(&closest, center);
	float   dist2 = vector3_dot(&d, &d);
	float   r     = sphere->radius;

	if (dist2 > r * r) return;

	float dist = sqrtf(dist2);
	Vector3 n = (dist > 1.0e-6f) ? vector3_scaled(&d, 1.0f / dist)
	                             : vector3_inverted(&triangle->normal);

	m->normal        = n;
	m->contact_count = 1;
	m->contacts[0].position    = closest;
	m->contacts[0].penetration = dist - r;
	m->contacts[0].fp.key      = 0;
}


/* Box corners against the triangle's plane. A resting box meets a floor
   triangle with its whole face, so this yields the several points a stack
   needs to stay up; a box caught on a bare edge gets a coarser answer than
   full SAT would give, which is the trade for keeping this cheap. */
static void boxToTriangle(ContactManifold *m, const Box *box, const Transform *world,
                          const Triangle *triangle)
{
	m->contact_count = 0;
	m->normal        = vector3_inverted(&triangle->normal);

	const Vector3 *v0 = &triangle->vertices[0];

	for (int i = 0; i < 8; i++) {
		Vector3 local = {
			(i & 1) ? box->e.x : -box->e.x,
			(i & 2) ? box->e.y : -box->e.y,
			(i & 4) ? box->e.z : -box->e.z,
		};
		Vector3 corner = transform_mulVector(world, &local);

		/* Signed distance to the plane; only corners behind it touch. */
		Vector3 to_corner = vector3_difference(&corner, v0);
		float   depth     = vector3_dot(&triangle->normal, &to_corner);
		if (depth >= 0.0f) continue;

		/* Behind the plane is not enough: it has to be behind the face. */
		Vector3 on_plane = corner;
		Vector3 lift     = vector3_scaled(&triangle->normal, -depth);
		on_plane = vector3_sum(&on_plane, &lift);

		Vector3 closest = triangle_closestToPoint(&triangle->vertices[0], &triangle->vertices[1],
		                                          &triangle->vertices[2], &on_plane);
		Vector3 slip  = vector3_difference(&closest, &on_plane);
		if (vector3_dot(&slip, &slip) > 1.0e-6f) continue;

		if (m->contact_count >= 8) break;

		ContactPoint *c = &m->contacts[m->contact_count++];
		c->position    = corner;
		c->penetration = depth;
		c->fp.key      = (uint32_t)i;
	}
}


/* Runs the shape against every triangle its AABB reaches and merges the hits.
   The mesh tree is in mesh-local space, so the shape moves by -origin going in
   and the contact points move back on the way out. */
static void shapeToMesh(ContactManifold *m, PhysicsShape *shape, PhysicsShape *mesh_shape)
{
	/* The contact manager runs computeBasis on this normal without checking
	   the contact count, so it must always hold a unit vector: uninitialised
	   memory blows up on the normalise, and so does a zero vector. */
	m->contact_count = 0;
	m->normal        = (Vector3){ 0.0f, 0.0f, 1.0f };

	const CollisionMesh *mesh = mesh_shape->mesh;
	if (mesh == NULL) return;

	Vector3 origin = mesh_shape->local.position;
	if (mesh_shape->body) {
		Transform world = transform_product(&mesh_shape->body->tx, &mesh_shape->local);
		origin = world.position;
	}

	Transform shape_world = shape->local;
	if (shape->body) shape_world = transform_product(&shape->body->tx, &shape->local);

	Transform local_world = shape_world;
	local_world.position  = vector3_difference(&shape_world.position, &origin);

	PhysicsShape probe = *shape;
	probe.body  = NULL;
	probe.local = local_world;

	AABB aabb;
	Transform identity;
	transform_init(&identity);
	physicsShape_computeAABB(&probe, &identity, &aabb);

	MeshQuery query = { .mesh = mesh };
	collisionMesh_queryAABB(mesh, &query, collision_collectTriangle, aabb);

	/* Starts at infinity, not at zero: a contact that merely touches still has
	   to set the normal, or the deepest-wins test below never fires. */
	float deepest = FLT_MAX;

	for (int32_t t = 0; t < query.count; t++) {
		Triangle triangle;
		collisionMesh_getTriangle(mesh, query.triangle[t], &triangle);

		ContactManifold hit = {0};

		switch (shape->type) {
			case SHAPE_SPHERE:
				sphereToTriangle(&hit, &shape->sphere, &local_world.position, &triangle);
				break;
			case SHAPE_BOX:
				boxToTriangle(&hit, &shape->box, &local_world, &triangle);
				break;
			case SHAPE_CAPSULE:
				capsuleToTriangle(&hit, &shape->capsule, &local_world, &triangle);
				break;
			default:
				break;
		}

		for (int32_t i = 0; i < hit.contact_count; i++) {
			if (m->contact_count >= 8) break;

			ContactPoint *c = &m->contacts[m->contact_count];
			*c = hit.contacts[i];
			c->position = vector3_sum(&c->position, &origin);
			/* Key by triangle so warm starting can match points across steps. */
			c->fp.key   = ((uint32_t)query.triangle[t] << 4) | (c->fp.key & 0xF);
			m->contact_count++;

			if (c->penetration < deepest) {
				deepest  = c->penetration;
				m->normal = hit.normal;
			}
		}
	}
}


/* Type-based dispatcher. */
void collision(ContactManifold *m, PhysicsShape *a, PhysicsShape *b)
{
	ShapeType ta = a->type;
	ShapeType tb = b->type;

	if (ta == SHAPE_BOX && tb == SHAPE_BOX) {
		boxToBox(m, a, b);
	}
	else if (ta == SHAPE_SPHERE && tb == SHAPE_SPHERE) {
		sphereToSphere(m, a, b);
	}
	else if (ta == SHAPE_CAPSULE && tb == SHAPE_CAPSULE) {
		capsuleToCapsule(m, a, b);
	}
	else if (ta == SHAPE_SPHERE && tb == SHAPE_BOX) {
		sphereToBox(m, a, b);
	}
	else if (ta == SHAPE_BOX && tb == SHAPE_SPHERE) {
		sphereToBox(m, b, a);
		m->normal = vector3_inverted(&m->normal);
	}
	else if (ta == SHAPE_SPHERE && tb == SHAPE_CAPSULE) {
		sphereToCapsule(m, a, b);
	}
	else if (ta == SHAPE_CAPSULE && tb == SHAPE_SPHERE) {
		sphereToCapsule(m, b, a);
		m->normal = vector3_inverted(&m->normal);
	}
	else if (ta == SHAPE_CAPSULE && tb == SHAPE_BOX) {
		capsuleToBox(m, a, b);
	}
	else if (ta == SHAPE_BOX && tb == SHAPE_CAPSULE) {
		capsuleToBox(m, b, a);
		m->normal = vector3_inverted(&m->normal);
	}
	/* Mesh is static-only, so the pair always has one convex side. */
	else if (tb == SHAPE_MESH) {
		shapeToMesh(m, a, b);
	}
	else if (ta == SHAPE_MESH) {
		shapeToMesh(m, b, a);
		/* No contact means no normal was written: inverting it would be
		   reading whatever the manifold happened to hold. */
		if (m->contact_count) m->normal = vector3_inverted(&m->normal);
	}
}
