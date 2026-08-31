/*
	Ported from qu3e q3BroadPhase.cpp — altered source, not the original software.

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
	Pair buffer on top of the dynamic AABB tree.
*/
#include <stdlib.h>
#include <string.h>

#include "physics/broadphase/e64_broad_phase.h"
#include "physics/collision/e64_contact_manager.h"
#include "physics/shapes/e64_physics_shape.h"
#include "physics/memory/e64_physics_memory.h"


/* Forward declaration — defined in contact_manager.c. */
void contactManager_addContact(ContactManager *m, PhysicsShape *A, PhysicsShape *B);


static inline int32_t i32_min(int32_t a, int32_t b) { return a < b ? a : b; }
static inline int32_t i32_max(int32_t a, int32_t b) { return a > b ? a : b; }


static void bufferMove(BroadPhase *bp, int32_t id)
{
	if (bp->move_count == bp->move_capacity) {
		int32_t *old = bp->move_buffer;
		bp->move_capacity *= 2;
		bp->move_buffer = (int32_t *)physics_alloc((int32_t)(bp->move_capacity * sizeof(int32_t)));
		memcpy(bp->move_buffer, old, (size_t)bp->move_count * sizeof(int32_t));
		physics_free(old);
	}
	bp->move_buffer[bp->move_count++] = id;
}


int broadPhase_treeCallback(void *bp_void, int32_t index)
{
	BroadPhase *bp = (BroadPhase *)bp_void;

	if (index == bp->current_index) return 1;

	if (bp->pair_count == bp->pair_capacity) {
		ContactPair *old = bp->pair_buffer;
		bp->pair_capacity *= 2;
		bp->pair_buffer = (ContactPair *)physics_alloc((int32_t)(bp->pair_capacity * sizeof(ContactPair)));
		memcpy(bp->pair_buffer, old, (size_t)bp->pair_count * sizeof(ContactPair));
		physics_free(old);
	}

	int32_t iA = i32_min(index, bp->current_index);
	int32_t iB = i32_max(index, bp->current_index);

	bp->pair_buffer[bp->pair_count].A = iA;
	bp->pair_buffer[bp->pair_count].B = iB;
	++bp->pair_count;

	return 1;
}


void broadPhase_init(BroadPhase *bp, ContactManager *manager)
{
	bp->manager = manager;

	bp->pair_count    = 0;
	bp->pair_capacity = 64;
	bp->pair_buffer   = (ContactPair *)physics_alloc((int32_t)(bp->pair_capacity * sizeof(ContactPair)));

	bp->move_count    = 0;
	bp->move_capacity = 64;
	bp->move_buffer   = (int32_t *)physics_alloc((int32_t)(bp->move_capacity * sizeof(int32_t)));

	dynamicAABBTree_init(&bp->tree);
	bp->current_index = 0;
}


void broadPhase_shutdown(BroadPhase *bp)
{
	physics_free(bp->move_buffer);
	physics_free(bp->pair_buffer);
	dynamicAABBTree_shutdown(&bp->tree);
	bp->move_buffer = NULL;
	bp->pair_buffer = NULL;
}


void broadPhase_insertShape(BroadPhase *bp, PhysicsShape *shape, AABB aabb)
{
	int32_t id = dynamicAABBTree_insert(&bp->tree, aabb, shape);
	shape->broadphase_index = id;
	bufferMove(bp, id);
}


void broadPhase_removeShape(BroadPhase *bp, const PhysicsShape *shape)
{
	dynamicAABBTree_remove(&bp->tree, shape->broadphase_index);
}


static int contactPair_cmp(const void *a, const void *b)
{
	const ContactPair *lhs = (const ContactPair *)a;
	const ContactPair *rhs = (const ContactPair *)b;
	if (lhs->A < rhs->A) return -1;
	if (lhs->A > rhs->A) return 1;
	if (lhs->B < rhs->B) return -1;
	if (lhs->B > rhs->B) return 1;
	return 0;
}


void broadPhase_updatePairs(BroadPhase *bp)
{
	bp->pair_count = 0;

	for (int32_t i = 0; i < bp->move_count; ++i) {
		bp->current_index = bp->move_buffer[i];
		AABB aabb = dynamicAABBTree_getFatAABB(&bp->tree, bp->current_index);
		dynamicAABBTree_queryAABB(&bp->tree, bp, broadPhase_treeCallback, aabb);
	}

	bp->move_count = 0;

	qsort(bp->pair_buffer, (size_t)bp->pair_count, sizeof(ContactPair), contactPair_cmp);

	int32_t i = 0;
	while (i < bp->pair_count) {
		ContactPair *pair = bp->pair_buffer + i;
		PhysicsShape *A = (PhysicsShape *)dynamicAABBTree_getUserData(&bp->tree, pair->A);
		PhysicsShape *B = (PhysicsShape *)dynamicAABBTree_getUserData(&bp->tree, pair->B);
		contactManager_addContact(bp->manager, A, B);

		++i;

		while (i < bp->pair_count) {
			ContactPair *dup = bp->pair_buffer + i;
			if (pair->A != dup->A || pair->B != dup->B) break;
			++i;
		}
	}
}


void broadPhase_update(BroadPhase *bp, int32_t id, AABB aabb)
{
	if (dynamicAABBTree_update(&bp->tree, id, aabb)) bufferMove(bp, id);
}


int broadPhase_testOverlap(const BroadPhase *bp, int32_t A, int32_t B)
{
	AABB a = dynamicAABBTree_getFatAABB(&bp->tree, A);
	AABB b = dynamicAABBTree_getFatAABB(&bp->tree, B);
	return aabb_overlaps(&a, &b);
}
