/*
	collision.c — narrowphase. Ports qu3e q3Collide.cpp (OBB-vs-OBB SAT +
	Sutherland-Hodgman clip) 1-a-1 and adds a type-based dispatcher so
	box / sphere / capsule pairs all land here.
*/
#include <float.h>
#include <math.h>

#include "physics/collision/collision.h"
#include "physics/body/rigid_body.h"
#include "physics/math/math_functions.h"    /* segment_closestToPoint */


/* --- Axis tracking helpers. --- */
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


/* --- Reference edge and basis for the reference face. --- */
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


/* --- Incident face corners for the box opposing the reference face. --- */
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


/* --- Sutherland-Hodgman one-plane clip. --- */
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


/* --- Clip the incident face against the reference face. --- */
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


/* --- Closest points between two line segments (edge-edge contact). --- */
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


/* --- Supporting edge of a box for the given direction n (world space). --- */
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


/* --- boxToBox: OBB-vs-OBB SAT + face/edge clipping. 1-a-1 port from qu3e. --- */
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


/* --- Sphere vs Sphere. --- */
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


/* --- Sphere vs Box. Sphere is A, Box is B. --- */
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


/* --- Capsule vs Box. Capsule is A, Box is B. --- */
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


/* --- Sphere vs Capsule. Sphere is A, capsule is B. Treat capsule as its
   segment + radius; closest-point-on-segment reduces to sphere-vs-sphere. --- */
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


/* --- Capsule vs Capsule. Closest point between the two inner segments. --- */
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


/* --- Capsule vs Box without RigidBody: static geometry placed by a
   world transform. Same logic as capsuleToBox. Capsule is A, Box is B. --- */
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


/* --- Capsule vs Sphere without RigidBody. Capsule is A, sphere is B. --- */
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


/* --- Capsule vs Capsule without RigidBody. A is the moving capsule. --- */
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


/* --- Capsule vs Triangle. The capsule is A, the triangle is B.
   Reference point on the segment via plane intersection, then one
   closest-point refinement. --- */
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


/* --- collision: type-based dispatcher. --- */
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
}
