/*
	Ported from qu3e q3DynamicAABBTree.h — altered source, not the original software.

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
	Bounding-volume hierarchy for broadphase queries. The C++ template
	Query<T> becomes a function-pointer callback.
*/
#ifndef PHYSICS_BROADPHASE_DYNAMIC_AABB_TREE_H
#define PHYSICS_BROADPHASE_DYNAMIC_AABB_TREE_H

#include <stdint.h>

#include "physics/geometry/aabb.h"
#include "physics/geometry/raycast.h"


#define PHYSICS_TREE_NULL (-1)


typedef struct DynamicAABBTreeNode {
	AABB    aabb;
	int32_t parent_or_next;   /* parent (active) / next pointer (free list) */
	int32_t left;
	int32_t right;
	void   *user_data;
	int32_t height;            /* leaf = 0, free = -1 */
} DynamicAABBTreeNode;


typedef struct DynamicAABBTree {
	int32_t              root;
	DynamicAABBTreeNode *nodes;
	int32_t              count;
	int32_t              capacity;
	int32_t              free_list;
} DynamicAABBTree;


static inline int dynamicAABBTreeNode_isLeaf(const DynamicAABBTreeNode *n) {
	return n->right == PHYSICS_TREE_NULL;
}


void dynamicAABBTree_init    (DynamicAABBTree *t);
void dynamicAABBTree_shutdown(DynamicAABBTree *t);

int32_t dynamicAABBTree_insert(DynamicAABBTree *t, AABB aabb, void *user_data);
void    dynamicAABBTree_remove(DynamicAABBTree *t, int32_t id);
int     dynamicAABBTree_update(DynamicAABBTree *t, int32_t id, AABB aabb);

void *dynamicAABBTree_getUserData(const DynamicAABBTree *t, int32_t id);
AABB  dynamicAABBTree_getFatAABB (const DynamicAABBTree *t, int32_t id);


typedef int (*PhysicsQueryCallback)(void *cb, int32_t id);

void dynamicAABBTree_queryAABB(const DynamicAABBTree *t, void *cb, PhysicsQueryCallback callback, AABB aabb);
void dynamicAABBTree_queryRay (const DynamicAABBTree *t, void *cb, PhysicsQueryCallback callback, RaycastData *raycast);


#endif
