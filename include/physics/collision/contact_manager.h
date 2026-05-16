/*
	contact_manager.h — owns the list of ContactConstraints and the BroadPhase.
	Ported from qu3e q3ContactManager, renamed to engine conventions.
*/
#ifndef PHYSICS_COLLISION_CONTACT_MANAGER_H
#define PHYSICS_COLLISION_CONTACT_MANAGER_H

#include <stdint.h>

#include "physics/broadphase/broad_phase.h"
#include "physics/memory/physics_paged_allocator.h"


struct ContactConstraint;
struct PhysicsShape;
struct RigidBody;
struct PhysicsStack;


typedef struct ContactManager {
	struct ContactConstraint *contact_list;
	int32_t                   contact_count;
	struct PhysicsStack      *stack;
	PhysicsPagedAllocator     allocator;
	BroadPhase                broadphase;
	void                     *contact_listener;
} ContactManager;


void contactManager_init    (ContactManager *m, struct PhysicsStack *stack);
void contactManager_shutdown(ContactManager *m);

void contactManager_addContact          (ContactManager *m, struct PhysicsShape *A, struct PhysicsShape *B);
void contactManager_findNewContacts     (ContactManager *m);
void contactManager_removeContact       (ContactManager *m, struct ContactConstraint *contact);
void contactManager_removeContactsFromBody(ContactManager *m, struct RigidBody *body);
void contactManager_removeFromBroadphase(ContactManager *m, struct RigidBody *body);
void contactManager_testCollisions      (ContactManager *m);


#endif
