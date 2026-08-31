/*
	Ported from qu3e q3ContactManager.h — altered source, not the original software.

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
	Owns the list of ContactConstraints and the BroadPhase.
*/
#ifndef ENGINE64_CONTACT_MANAGER_H
#define ENGINE64_CONTACT_MANAGER_H

#include <stdint.h>

#include "physics/broadphase/e64_broad_phase.h"
#include "physics/memory/e64_physics_paged_allocator.h"


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
