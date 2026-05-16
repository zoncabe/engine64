/*
	contact_solver.c — sequential impulse solver (PGS). Ports qu3e
	q3ContactSolver.cpp 1-a-1.
*/
#include "physics/collision/contact_solver.h"
#include "physics/collision/contact.h"
#include "physics/world/physics_island.h"
#include "physics/body/rigid_body.h"


static inline float clampf(float lo, float hi, float v)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}


static inline float invert_or_zero(float x)
{
	return x != 0.0f ? 1.0f / x : 0.0f;
}


void contactSolver_initialize(ContactSolver *s, PhysicsIsland *island)
{
	s->island          = island;
	s->contact_count   = island->contact_count;
	s->contacts        = island->contact_states;
	s->velocities      = island->velocities;
	s->enable_friction = island->enable_friction;
}


void contactSolver_shutdown(ContactSolver *s)
{
	for (int32_t i = 0; i < s->contact_count; ++i) {
		ContactConstraintState *c  = s->contacts + i;
		ContactConstraint      *cc = s->island->contacts[i];

		for (int32_t j = 0; j < c->contact_count; ++j) {
			ContactPoint *oc = cc->manifold.contacts + j;
			ContactState *cs = c->contacts + j;
			oc->normal_impulse     = cs->normal_impulse;
			oc->tangent_impulse[0] = cs->tangent_impulse[0];
			oc->tangent_impulse[1] = cs->tangent_impulse[1];
		}
	}
}


void contactSolver_preSolve(ContactSolver *s, float dt)
{
	for (int32_t i = 0; i < s->contact_count; ++i) {
		ContactConstraintState *cs = s->contacts + i;

		Vector3 vA = s->velocities[cs->index_a].v;
		Vector3 wA = s->velocities[cs->index_a].w;
		Vector3 vB = s->velocities[cs->index_b].v;
		Vector3 wB = s->velocities[cs->index_b].w;

		for (int32_t j = 0; j < cs->contact_count; ++j) {
			ContactState *c = cs->contacts + j;

			Vector3 raCn = vector3_cross(&c->ra, &cs->normal);
			Vector3 rbCn = vector3_cross(&c->rb, &cs->normal);
			float nm = cs->mA + cs->mB;
			float tm[2] = { nm, nm };

			Vector3 iA_raCn = matrix3_transformVector(&cs->iA, &raCn);
			Vector3 iB_rbCn = matrix3_transformVector(&cs->iB, &rbCn);
			nm += vector3_dot(&raCn, &iA_raCn) + vector3_dot(&rbCn, &iB_rbCn);
			c->normal_mass = invert_or_zero(nm);

			for (int32_t k = 0; k < 2; ++k) {
				Vector3 raCt = vector3_cross(&cs->tangent_vectors[k], &c->ra);
				Vector3 rbCt = vector3_cross(&cs->tangent_vectors[k], &c->rb);
				Vector3 iA_raCt = matrix3_transformVector(&cs->iA, &raCt);
				Vector3 iB_rbCt = matrix3_transformVector(&cs->iB, &rbCt);
				tm[k] += vector3_dot(&raCt, &iA_raCt) + vector3_dot(&rbCt, &iB_rbCt);
				c->tangent_mass[k] = invert_or_zero(tm[k]);
			}

			float pen_bias = c->penetration + PHYSICS_PENETRATION_SLOP;
			if (pen_bias > 0.0f) pen_bias = 0.0f;
			c->bias = -PHYSICS_BAUMGARTE * (1.0f / dt) * pen_bias;

			Vector3 P = vector3_scaled(&cs->normal, c->normal_impulse);

			if (s->enable_friction) {
				Vector3 t0 = vector3_scaled(&cs->tangent_vectors[0], c->tangent_impulse[0]);
				Vector3 t1 = vector3_scaled(&cs->tangent_vectors[1], c->tangent_impulse[1]);
				P = vector3_sum(&P, &t0);
				P = vector3_sum(&P, &t1);
			}

			Vector3 P_a = vector3_scaled(&P, cs->mA);
			vA = vector3_difference(&vA, &P_a);
			Vector3 cross_ra_P = vector3_cross(&c->ra, &P);
			Vector3 iA_cross_a = matrix3_transformVector(&cs->iA, &cross_ra_P);
			wA = vector3_difference(&wA, &iA_cross_a);

			Vector3 P_b = vector3_scaled(&P, cs->mB);
			vB = vector3_sum(&vB, &P_b);
			Vector3 cross_rb_P = vector3_cross(&c->rb, &P);
			Vector3 iB_cross_b = matrix3_transformVector(&cs->iB, &cross_rb_P);
			wB = vector3_sum(&wB, &iB_cross_b);

			/* rel = (vB + wB × rb) - vA - wA × ra */
			Vector3 wb_rb  = vector3_cross(&wB, &c->rb);
			Vector3 vb_rel = vector3_sum(&vB, &wb_rb);
			Vector3 wa_ra  = vector3_cross(&wA, &c->ra);
			Vector3 va_rel = vector3_sum(&vA, &wa_ra);
			Vector3 rel    = vector3_difference(&vb_rel, &va_rel);
			float dv = vector3_dot(&rel, &cs->normal);
			if (dv < -1.0f) c->bias += -(cs->restitution) * dv;
		}

		s->velocities[cs->index_a].v = vA;
		s->velocities[cs->index_a].w = wA;
		s->velocities[cs->index_b].v = vB;
		s->velocities[cs->index_b].w = wB;
	}
}


void contactSolver_solve(ContactSolver *s)
{
	for (int32_t i = 0; i < s->contact_count; ++i) {
		ContactConstraintState *cs = s->contacts + i;

		Vector3 vA = s->velocities[cs->index_a].v;
		Vector3 wA = s->velocities[cs->index_a].w;
		Vector3 vB = s->velocities[cs->index_b].v;
		Vector3 wB = s->velocities[cs->index_b].w;

		for (int32_t j = 0; j < cs->contact_count; ++j) {
			ContactState *c = cs->contacts + j;

			/* dv = (vB + wB × rb) - vA - wA × ra */
			Vector3 wb_rb  = vector3_cross(&wB, &c->rb);
			Vector3 vb_rel = vector3_sum(&vB, &wb_rb);
			Vector3 wa_ra  = vector3_cross(&wA, &c->ra);
			Vector3 va_rel = vector3_sum(&vA, &wa_ra);
			Vector3 dv     = vector3_difference(&vb_rel, &va_rel);

			if (s->enable_friction) {
				for (int32_t k = 0; k < 2; ++k) {
					float lambda    = -vector3_dot(&dv, &cs->tangent_vectors[k]) * c->tangent_mass[k];
					float max_lambda = cs->friction * c->normal_impulse;
					float old_pt    = c->tangent_impulse[k];
					c->tangent_impulse[k] = clampf(-max_lambda, max_lambda, old_pt + lambda);
					lambda = c->tangent_impulse[k] - old_pt;

					Vector3 impulse = vector3_scaled(&cs->tangent_vectors[k], lambda);

					Vector3 imp_a = vector3_scaled(&impulse, cs->mA);
					vA = vector3_difference(&vA, &imp_a);
					Vector3 cross_ra = vector3_cross(&c->ra, &impulse);
					Vector3 iA_cra   = matrix3_transformVector(&cs->iA, &cross_ra);
					wA = vector3_difference(&wA, &iA_cra);

					Vector3 imp_b = vector3_scaled(&impulse, cs->mB);
					vB = vector3_sum(&vB, &imp_b);
					Vector3 cross_rb = vector3_cross(&c->rb, &impulse);
					Vector3 iB_crb   = matrix3_transformVector(&cs->iB, &cross_rb);
					wB = vector3_sum(&wB, &iB_crb);
				}
			}

			/* Recompute dv after friction. */
			wb_rb  = vector3_cross(&wB, &c->rb);
			vb_rel = vector3_sum(&vB, &wb_rb);
			wa_ra  = vector3_cross(&wA, &c->ra);
			va_rel = vector3_sum(&vA, &wa_ra);
			dv     = vector3_difference(&vb_rel, &va_rel);

			float vn     = vector3_dot(&dv, &cs->normal);
			float lambda = c->normal_mass * (-vn + c->bias);
			float temp_pn = c->normal_impulse;
			c->normal_impulse = (temp_pn + lambda > 0.0f) ? (temp_pn + lambda) : 0.0f;
			lambda = c->normal_impulse - temp_pn;

			Vector3 impulse = vector3_scaled(&cs->normal, lambda);

			Vector3 imp_a = vector3_scaled(&impulse, cs->mA);
			vA = vector3_difference(&vA, &imp_a);
			Vector3 cross_ra = vector3_cross(&c->ra, &impulse);
			Vector3 iA_cra   = matrix3_transformVector(&cs->iA, &cross_ra);
			wA = vector3_difference(&wA, &iA_cra);

			Vector3 imp_b = vector3_scaled(&impulse, cs->mB);
			vB = vector3_sum(&vB, &imp_b);
			Vector3 cross_rb = vector3_cross(&c->rb, &impulse);
			Vector3 iB_crb   = matrix3_transformVector(&cs->iB, &cross_rb);
			wB = vector3_sum(&wB, &iB_crb);
		}

		s->velocities[cs->index_a].v = vA;
		s->velocities[cs->index_a].w = wA;
		s->velocities[cs->index_b].v = vB;
		s->velocities[cs->index_b].w = wB;
	}
}
