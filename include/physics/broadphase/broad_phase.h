/*
	broad_phase.h — pair-finding broadphase over a dynamic AABB tree.
	Ported from qu3e q3BroadPhase.
*/
#ifndef PHYSICS_BROADPHASE_H
#define PHYSICS_BROADPHASE_H

#include <stdint.h>

#include "physics/geometry/aabb.h"
#include "physics/broadphase/dynamic_aabb_tree.h"


struct ContactManager;
struct PhysicsShape;


typedef struct ContactPair {
	int32_t A;
	int32_t B;
} ContactPair;


typedef struct BroadPhase {
	struct ContactManager *manager;

	ContactPair *pair_buffer;
	int32_t      pair_count;
	int32_t      pair_capacity;

	int32_t     *move_buffer;
	int32_t      move_count;
	int32_t      move_capacity;

	DynamicAABBTree tree;
	int32_t         current_index;
} BroadPhase;


void broadPhase_init    (BroadPhase *bp, struct ContactManager *manager);
void broadPhase_shutdown(BroadPhase *bp);

void broadPhase_insertShape(BroadPhase *bp, struct PhysicsShape *shape, AABB aabb);
void broadPhase_removeShape(BroadPhase *bp, const struct PhysicsShape *shape);
void broadPhase_updatePairs(BroadPhase *bp);
void broadPhase_update     (BroadPhase *bp, int32_t id, AABB aabb);
int  broadPhase_testOverlap(const BroadPhase *bp, int32_t A, int32_t B);

int  broadPhase_treeCallback(void *bp_void, int32_t index);


#endif
