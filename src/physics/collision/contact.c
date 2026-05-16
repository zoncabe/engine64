/*
	contact.c — manifold/constraint impl. Dispatches narrowphase via collide().
*/
#include "physics/collision/contact.h"


#include "physics/collision/collision.h"


void contactManifold_setPair(ContactManifold *m, PhysicsShape *a, PhysicsShape *b)
{
	m->A = a;
	m->B = b;
	m->sensor = a->sensor || b->sensor;
}


void contactConstraint_solveCollision(ContactConstraint *c)
{
	c->manifold.contact_count = 0;

	collision(&c->manifold, c->A, c->B);

	if (c->manifold.contact_count > 0) {
		if (c->flags & CONSTRAINT_COLLIDING) {
			c->flags |= CONSTRAINT_WAS_COLLIDING;
		} else {
			c->flags |= CONSTRAINT_COLLIDING;
		}
	} else {
		if (c->flags & CONSTRAINT_COLLIDING) {
			c->flags &= ~CONSTRAINT_COLLIDING;
			c->flags |= CONSTRAINT_WAS_COLLIDING;
		} else {
			c->flags &= ~CONSTRAINT_WAS_COLLIDING;
		}
	}
}
