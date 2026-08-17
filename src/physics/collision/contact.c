/*
	Ported from qu3e q3Contact.cpp — altered source, not the original software.

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
	Manifold and constraint implementation. Dispatches narrowphase through
	collide().
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
