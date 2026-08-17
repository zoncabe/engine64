/*
	Ported from qu3e q3ContactManager.cpp — altered source, not the original software.

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
	Maintains the list of active ContactConstraints, drives broadphase pair
	generation, and refreshes contacts each step.
*/
#include <stddef.h>

#include "physics/collision/contact_manager.h"
#include "physics/collision/contact.h"
#include "physics/body/rigid_body.h"
#include "physics/shapes/physics_shape.h"
#include "physics/geometry/half_space.h"   /* vector3_computeBasis */


void contactManager_init(ContactManager *m, struct PhysicsStack *stack)
{
	m->stack = stack;
	/* A ContactConstraint carries a whole manifold, so it is 624 bytes: the
	   qu3e page of 256 asks for 156 KB in one malloc, which fragments the heap
	   badly on a console with 4 MB. 32 keeps a page at 20 KB and the allocator
	   simply adds another when a busy scene needs it. */
	physicsPagedAllocator_init(&m->allocator, (int32_t)sizeof(ContactConstraint), 32);
	broadPhase_init(&m->broadphase, m);
	m->contact_list     = NULL;
	m->contact_count    = 0;
	m->contact_listener = NULL;
}


void contactManager_shutdown(ContactManager *m)
{
	broadPhase_shutdown(&m->broadphase);
	physicsPagedAllocator_shutdown(&m->allocator);
	m->contact_list  = NULL;
	m->contact_count = 0;
}


void contactManager_addContact(ContactManager *m, PhysicsShape *A, PhysicsShape *B)
{
	RigidBody *body_a = A->body;
	RigidBody *body_b = B->body;
	if (!rigidBody_canCollide(body_a, body_b)) return;

	/* Dedup. */
	ContactEdge *edge = body_a->contact_list;
	while (edge) {
		if (edge->other == body_b) {
			PhysicsShape *shape_a = edge->constraint->A;
			PhysicsShape *shape_b = edge->constraint->B;
			if (A == shape_a && B == shape_b) return;
		}
		edge = edge->next;
	}

	ContactConstraint *contact = (ContactConstraint *)physicsPagedAllocator_allocate(&m->allocator);
	contact->A            = A;
	contact->B            = B;
	contact->body_a       = A->body;
	contact->body_b       = B->body;
	contactManifold_setPair(&contact->manifold, A, B);
	contact->flags        = 0;
	contact->friction     = contact_mixFriction(A, B);
	contact->restitution  = contact_mixRestitution(A, B);
	contact->manifold.contact_count = 0;

	/* The allocator hands out raw malloc memory, and testCollisions runs
	   computeBasis on this normal whether or not the pair touched. Left as it
	   came, a fresh page from a previous scene makes it garbage — which is why
	   the first run survives and the second one does not. */
	contact->manifold.normal            = (Vector3){ 0.0f, 0.0f, 1.0f };
	contact->manifold.tangent_vectors[0] = (Vector3){ 1.0f, 0.0f, 0.0f };
	contact->manifold.tangent_vectors[1] = (Vector3){ 0.0f, 1.0f, 0.0f };

	for (int32_t i = 0; i < 8; ++i) contact->manifold.contacts[i].warm_started = 0;

	contact->prev = NULL;
	contact->next = m->contact_list;
	if (m->contact_list) m->contact_list->prev = contact;
	m->contact_list = contact;

	contact->edge_a.constraint = contact;
	contact->edge_a.other      = body_b;
	contact->edge_a.prev = NULL;
	contact->edge_a.next = body_a->contact_list;
	if (body_a->contact_list) body_a->contact_list->prev = &contact->edge_a;
	body_a->contact_list = &contact->edge_a;

	contact->edge_b.constraint = contact;
	contact->edge_b.other      = body_a;
	contact->edge_b.prev = NULL;
	contact->edge_b.next = body_b->contact_list;
	if (body_b->contact_list) body_b->contact_list->prev = &contact->edge_b;
	body_b->contact_list = &contact->edge_b;

	rigidBody_setToAwake(body_a);
	rigidBody_setToAwake(body_b);

	++m->contact_count;
}


void contactManager_findNewContacts(ContactManager *m)
{
	broadPhase_updatePairs(&m->broadphase);
}


void contactManager_removeContact(ContactManager *m, ContactConstraint *contact)
{
	RigidBody *A = contact->body_a;
	RigidBody *B = contact->body_b;

	/* Remove from A. */
	if (contact->edge_a.prev) contact->edge_a.prev->next = contact->edge_a.next;
	if (contact->edge_a.next) contact->edge_a.next->prev = contact->edge_a.prev;
	if (&contact->edge_a == A->contact_list) A->contact_list = contact->edge_a.next;

	/* Remove from B. */
	if (contact->edge_b.prev) contact->edge_b.prev->next = contact->edge_b.next;
	if (contact->edge_b.next) contact->edge_b.next->prev = contact->edge_b.prev;
	if (&contact->edge_b == B->contact_list) B->contact_list = contact->edge_b.next;

	rigidBody_setToAwake(A);
	rigidBody_setToAwake(B);

	if (contact->prev) contact->prev->next = contact->next;
	if (contact->next) contact->next->prev = contact->prev;
	if (contact == m->contact_list) m->contact_list = contact->next;

	--m->contact_count;

	physicsPagedAllocator_free(&m->allocator, contact);
}


void contactManager_removeContactsFromBody(ContactManager *m, RigidBody *body)
{
	ContactEdge *edge = body->contact_list;
	while (edge) {
		ContactEdge *next = edge->next;
		contactManager_removeContact(m, edge->constraint);
		edge = next;
	}
}


void contactManager_removeFromBroadphase(ContactManager *m, RigidBody *body)
{
	PhysicsShape *shape = body->shapes;
	while (shape) {
		broadPhase_removeShape(&m->broadphase, shape);
		shape = shape->next;
	}
}


void contactManager_testCollisions(ContactManager *m)
{
	ContactConstraint *constraint = m->contact_list;

	while (constraint) {
		PhysicsShape *A = constraint->A;
		PhysicsShape *B = constraint->B;
		RigidBody *body_a = A->body;
		RigidBody *body_b = B->body;

		constraint->flags &= ~CONSTRAINT_ISLAND;

		if (!rigidBody_isAwake(body_a) && !rigidBody_isAwake(body_b)) {
			constraint = constraint->next;
			continue;
		}

		if (!rigidBody_canCollide(body_a, body_b)) {
			ContactConstraint *next = constraint->next;
			contactManager_removeContact(m, constraint);
			constraint = next;
			continue;
		}

		if (!broadPhase_testOverlap(&m->broadphase, A->broadphase_index, B->broadphase_index)) {
			ContactConstraint *next = constraint->next;
			contactManager_removeContact(m, constraint);
			constraint = next;
			continue;
		}

		ContactManifold *manifold    = &constraint->manifold;
		ContactManifold  old         = constraint->manifold;
		Vector3          ot0         = old.tangent_vectors[0];
		Vector3          ot1         = old.tangent_vectors[1];
		contactConstraint_solveCollision(constraint);
		vector3_computeBasis(&manifold->normal, &manifold->tangent_vectors[0], &manifold->tangent_vectors[1]);

		for (int32_t i = 0; i < manifold->contact_count; ++i) {
			ContactPoint *c = manifold->contacts + i;
			c->tangent_impulse[0] = 0.0f;
			c->tangent_impulse[1] = 0.0f;
			c->normal_impulse     = 0.0f;
			uint8_t old_warm      = c->warm_started;
			c->warm_started       = 0;

			for (int32_t j = 0; j < old.contact_count; ++j) {
				ContactPoint *oc = old.contacts + j;
				if (c->fp.key == oc->fp.key) {
					c->normal_impulse = oc->normal_impulse;

					Vector3 t0_imp     = vector3_scaled(&ot0, oc->tangent_impulse[0]);
					Vector3 t1_imp     = vector3_scaled(&ot1, oc->tangent_impulse[1]);
					Vector3 friction   = vector3_sum(&t0_imp, &t1_imp);
					c->tangent_impulse[0] = vector3_dot(&friction, &manifold->tangent_vectors[0]);
					c->tangent_impulse[1] = vector3_dot(&friction, &manifold->tangent_vectors[1]);
					uint8_t next_warm = (uint8_t)(old_warm + 1);
					c->warm_started = (old_warm > next_warm) ? old_warm : next_warm;
					break;
				}
			}
		}

		constraint = constraint->next;
	}
}
