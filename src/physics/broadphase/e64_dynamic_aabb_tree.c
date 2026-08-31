/*
	Ported from qu3e q3DynamicAABBTree.cpp — altered source, not the original software.

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
	BVH for broadphase. Nodes live in a pool with a free list, insert uses a
	surface-area heuristic, and balancing is single-rotation AVL-style.
*/
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#include "physics/broadphase/e64_dynamic_aabb_tree.h"
#include "physics/memory/e64_physics_memory.h"
#include "physics/e64_physics_settings.h"


static inline int imax(int a, int b) { return a > b ? a : b; }


static inline void fattenAABB(AABB *aabb)
{
	Vector3 v = { PHYSICS_AABB_FATTENER, PHYSICS_AABB_FATTENER, PHYSICS_AABB_FATTENER };
	aabb->min = vector3_difference(&aabb->min, &v);
	aabb->max = vector3_sum(&aabb->max, &v);
}


static void addToFreeList(DynamicAABBTree *t, int32_t index)
{
	for (int32_t i = index; i < t->capacity - 1; ++i) {
		t->nodes[i].parent_or_next = i + 1;
		t->nodes[i].height         = PHYSICS_TREE_NULL;
	}
	t->nodes[t->capacity - 1].parent_or_next = PHYSICS_TREE_NULL;
	t->nodes[t->capacity - 1].height         = PHYSICS_TREE_NULL;
	t->free_list = index;
}


static int32_t allocateNode(DynamicAABBTree *t)
{
	if (t->free_list == PHYSICS_TREE_NULL) {
		t->capacity *= 2;
		DynamicAABBTreeNode *new_nodes = (DynamicAABBTreeNode *)
			physics_alloc((int32_t)(sizeof(DynamicAABBTreeNode) * t->capacity));
		memcpy(new_nodes, t->nodes, sizeof(DynamicAABBTreeNode) * (size_t)t->count);
		physics_free(t->nodes);
		t->nodes = new_nodes;
		addToFreeList(t, t->count);
	}

	int32_t free_node = t->free_list;
	t->free_list = t->nodes[t->free_list].parent_or_next;
	t->nodes[free_node].height         = 0;
	t->nodes[free_node].left           = PHYSICS_TREE_NULL;
	t->nodes[free_node].right          = PHYSICS_TREE_NULL;
	t->nodes[free_node].parent_or_next = PHYSICS_TREE_NULL;
	t->nodes[free_node].user_data      = NULL;
	++t->count;
	return free_node;
}


static void deallocateNode(DynamicAABBTree *t, int32_t index)
{
	assert(index >= 0 && index < t->capacity);
	t->nodes[index].parent_or_next = t->free_list;
	t->nodes[index].height         = PHYSICS_TREE_NULL;
	t->free_list = index;
	--t->count;
}


static int32_t balance(DynamicAABBTree *t, int32_t iA)
{
	DynamicAABBTreeNode *A = t->nodes + iA;
	if (dynamicAABBTreeNode_isLeaf(A) || A->height == 1) return iA;

	int32_t iB = A->left;
	int32_t iC = A->right;
	DynamicAABBTreeNode *B = t->nodes + iB;
	DynamicAABBTreeNode *C = t->nodes + iC;

	int32_t bal = C->height - B->height;

	if (bal > 1) {
		int32_t iF = C->left;
		int32_t iG = C->right;
		DynamicAABBTreeNode *F = t->nodes + iF;
		DynamicAABBTreeNode *G = t->nodes + iG;

		if (A->parent_or_next != PHYSICS_TREE_NULL) {
			if (t->nodes[A->parent_or_next].left == iA)
				t->nodes[A->parent_or_next].left = iC;
			else
				t->nodes[A->parent_or_next].right = iC;
		} else {
			t->root = iC;
		}

		C->left           = iA;
		C->parent_or_next = A->parent_or_next;
		A->parent_or_next = iC;

		if (F->height > G->height) {
			C->right          = iF;
			A->right          = iG;
			G->parent_or_next = iA;
			A->aabb           = aabb_combine(&B->aabb, &G->aabb);
			C->aabb           = aabb_combine(&A->aabb, &F->aabb);
			A->height         = 1 + imax(B->height, G->height);
			C->height         = 1 + imax(A->height, F->height);
		} else {
			C->right          = iG;
			A->right          = iF;
			F->parent_or_next = iA;
			A->aabb           = aabb_combine(&B->aabb, &F->aabb);
			C->aabb           = aabb_combine(&A->aabb, &G->aabb);
			A->height         = 1 + imax(B->height, F->height);
			C->height         = 1 + imax(A->height, G->height);
		}
		return iC;
	}
	else if (bal < -1) {
		int32_t iD = B->left;
		int32_t iE = B->right;
		DynamicAABBTreeNode *D = t->nodes + iD;
		DynamicAABBTreeNode *E = t->nodes + iE;

		if (A->parent_or_next != PHYSICS_TREE_NULL) {
			if (t->nodes[A->parent_or_next].left == iA)
				t->nodes[A->parent_or_next].left = iB;
			else
				t->nodes[A->parent_or_next].right = iB;
		} else {
			t->root = iB;
		}

		B->right          = iA;
		B->parent_or_next = A->parent_or_next;
		A->parent_or_next = iB;

		if (D->height > E->height) {
			B->left           = iD;
			A->left           = iE;
			E->parent_or_next = iA;
			A->aabb           = aabb_combine(&C->aabb, &E->aabb);
			B->aabb           = aabb_combine(&A->aabb, &D->aabb);
			A->height         = 1 + imax(C->height, E->height);
			B->height         = 1 + imax(A->height, D->height);
		} else {
			B->left           = iE;
			A->left           = iD;
			D->parent_or_next = iA;
			A->aabb           = aabb_combine(&C->aabb, &D->aabb);
			B->aabb           = aabb_combine(&A->aabb, &E->aabb);
			A->height         = 1 + imax(C->height, D->height);
			B->height         = 1 + imax(A->height, E->height);
		}
		return iB;
	}
	return iA;
}


static void syncHierarchy(DynamicAABBTree *t, int32_t index)
{
	while (index != PHYSICS_TREE_NULL) {
		index = balance(t, index);
		int32_t left  = t->nodes[index].left;
		int32_t right = t->nodes[index].right;

		t->nodes[index].height = 1 + imax(t->nodes[left].height, t->nodes[right].height);
		t->nodes[index].aabb   = aabb_combine(&t->nodes[left].aabb, &t->nodes[right].aabb);

		index = t->nodes[index].parent_or_next;
	}
}


static void insertLeaf(DynamicAABBTree *t, int32_t id)
{
	if (t->root == PHYSICS_TREE_NULL) {
		t->root = id;
		t->nodes[t->root].parent_or_next = PHYSICS_TREE_NULL;
		return;
	}

	int32_t search = t->root;
	AABB leaf_aabb = t->nodes[id].aabb;
	while (!dynamicAABBTreeNode_isLeaf(&t->nodes[search])) {
		AABB  combined      = aabb_combine(&leaf_aabb, &t->nodes[search].aabb);
		float combined_area = aabb_surfaceArea(&combined);
		float branch_cost   = 2.0f * combined_area;
		float inherited     = 2.0f * (combined_area - aabb_surfaceArea(&t->nodes[search].aabb));

		int32_t left  = t->nodes[search].left;
		int32_t right = t->nodes[search].right;

		float left_cost;
		if (dynamicAABBTreeNode_isLeaf(&t->nodes[left])) {
			AABB c = aabb_combine(&leaf_aabb, &t->nodes[left].aabb);
			left_cost = aabb_surfaceArea(&c) + inherited;
		} else {
			AABB  c = aabb_combine(&leaf_aabb, &t->nodes[left].aabb);
			float inflated = aabb_surfaceArea(&c);
			float branch   = aabb_surfaceArea(&t->nodes[left].aabb);
			left_cost = inflated - branch + inherited;
		}

		float right_cost;
		if (dynamicAABBTreeNode_isLeaf(&t->nodes[right])) {
			AABB c = aabb_combine(&leaf_aabb, &t->nodes[right].aabb);
			right_cost = aabb_surfaceArea(&c) + inherited;
		} else {
			AABB  c = aabb_combine(&leaf_aabb, &t->nodes[right].aabb);
			float inflated = aabb_surfaceArea(&c);
			float branch   = aabb_surfaceArea(&t->nodes[right].aabb);
			right_cost = inflated - branch + inherited;
		}

		if (branch_cost < left_cost && branch_cost < right_cost) break;
		search = (left_cost < right_cost) ? left : right;
	}

	int32_t sibling    = search;
	int32_t old_parent = t->nodes[sibling].parent_or_next;
	int32_t new_parent = allocateNode(t);

	t->nodes[new_parent].parent_or_next = old_parent;
	t->nodes[new_parent].user_data      = NULL;
	t->nodes[new_parent].aabb           = aabb_combine(&leaf_aabb, &t->nodes[sibling].aabb);
	t->nodes[new_parent].height         = t->nodes[sibling].height + 1;

	if (old_parent == PHYSICS_TREE_NULL) {
		t->nodes[new_parent].left        = sibling;
		t->nodes[new_parent].right       = id;
		t->nodes[sibling].parent_or_next = new_parent;
		t->nodes[id].parent_or_next      = new_parent;
		t->root = new_parent;
	} else {
		if (t->nodes[old_parent].left == sibling)
			t->nodes[old_parent].left = new_parent;
		else
			t->nodes[old_parent].right = new_parent;

		t->nodes[new_parent].left        = sibling;
		t->nodes[new_parent].right       = id;
		t->nodes[sibling].parent_or_next = new_parent;
		t->nodes[id].parent_or_next      = new_parent;
	}

	syncHierarchy(t, t->nodes[id].parent_or_next);
}


static void removeLeaf(DynamicAABBTree *t, int32_t id)
{
	if (id == t->root) {
		t->root = PHYSICS_TREE_NULL;
		return;
	}

	int32_t parent      = t->nodes[id].parent_or_next;
	int32_t grandparent = t->nodes[parent].parent_or_next;
	int32_t sibling     = (t->nodes[parent].left == id)
		? t->nodes[parent].right : t->nodes[parent].left;

	if (grandparent != PHYSICS_TREE_NULL) {
		if (t->nodes[grandparent].left == parent)
			t->nodes[grandparent].left = sibling;
		else
			t->nodes[grandparent].right = sibling;

		t->nodes[sibling].parent_or_next = grandparent;
	} else {
		t->root = sibling;
		t->nodes[sibling].parent_or_next = PHYSICS_TREE_NULL;
	}

	deallocateNode(t, parent);
	syncHierarchy(t, grandparent);
}


void dynamicAABBTree_init(DynamicAABBTree *t)
{
	t->root     = PHYSICS_TREE_NULL;
	t->capacity = 1024;
	t->count    = 0;
	t->nodes    = (DynamicAABBTreeNode *)physics_alloc((int32_t)(sizeof(DynamicAABBTreeNode) * t->capacity));
	addToFreeList(t, 0);
}


void dynamicAABBTree_shutdown(DynamicAABBTree *t)
{
	if (t->nodes) physics_free(t->nodes);
	t->nodes = NULL;
}


int32_t dynamicAABBTree_insert(DynamicAABBTree *t, AABB aabb, void *user_data)
{
	int32_t id = allocateNode(t);
	t->nodes[id].aabb = aabb;
	fattenAABB(&t->nodes[id].aabb);
	t->nodes[id].user_data = user_data;
	t->nodes[id].height    = 0;
	insertLeaf(t, id);
	return id;
}


void dynamicAABBTree_remove(DynamicAABBTree *t, int32_t id)
{
	assert(id >= 0 && id < t->capacity);
	assert(dynamicAABBTreeNode_isLeaf(&t->nodes[id]));
	removeLeaf(t, id);
	deallocateNode(t, id);
}


int dynamicAABBTree_update(DynamicAABBTree *t, int32_t id, AABB aabb)
{
	assert(id >= 0 && id < t->capacity);
	assert(dynamicAABBTreeNode_isLeaf(&t->nodes[id]));

	if (aabb_containsAABB(&t->nodes[id].aabb, &aabb)) return 0;

	removeLeaf(t, id);
	t->nodes[id].aabb = aabb;
	fattenAABB(&t->nodes[id].aabb);
	insertLeaf(t, id);
	return 1;
}


void *dynamicAABBTree_getUserData(const DynamicAABBTree *t, int32_t id)
{
	assert(id >= 0 && id < t->capacity);
	return t->nodes[id].user_data;
}


AABB dynamicAABBTree_getFatAABB(const DynamicAABBTree *t, int32_t id)
{
	assert(id >= 0 && id < t->capacity);
	return t->nodes[id].aabb;
}


#define TREE_QUERY_STACK_CAP 256


void dynamicAABBTree_queryAABB(const DynamicAABBTree *t, void *cb, PhysicsQueryCallback callback, AABB aabb)
{
	int32_t stack[TREE_QUERY_STACK_CAP];
	int32_t sp = 1;
	stack[0] = t->root;

	while (sp) {
		assert(sp < TREE_QUERY_STACK_CAP);
		int32_t id = stack[--sp];
		if (id == PHYSICS_TREE_NULL) continue;

		const DynamicAABBTreeNode *n = t->nodes + id;
		if (aabb_overlaps(&aabb, &n->aabb)) {
			if (dynamicAABBTreeNode_isLeaf(n)) {
				if (!callback(cb, id)) return;
			} else {
				stack[sp++] = n->left;
				stack[sp++] = n->right;
			}
		}
	}
}


void dynamicAABBTree_queryRay(const DynamicAABBTree *t, void *cb, PhysicsQueryCallback callback, RaycastData *raycast)
{
	const float k_epsilon = 1.0e-6f;
	int32_t stack[TREE_QUERY_STACK_CAP];
	int32_t sp = 1;
	stack[0] = t->root;

	Vector3 p0     = raycast->start;
	Vector3 dir_t  = vector3_scaled(&raycast->dir, raycast->t);
	Vector3 p1     = vector3_sum(&p0, &dir_t);

	while (sp) {
		assert(sp < TREE_QUERY_STACK_CAP);
		int32_t id = stack[--sp];
		if (id == PHYSICS_TREE_NULL) continue;

		const DynamicAABBTreeNode *n = t->nodes + id;

		Vector3 e = vector3_difference(&n->aabb.max, &n->aabb.min);
		Vector3 d = vector3_difference(&p1, &p0);
		Vector3 p_sum  = vector3_sum(&p0, &p1);
		Vector3 m_tmp  = vector3_difference(&p_sum, &n->aabb.min);
		Vector3 m      = vector3_difference(&m_tmp, &n->aabb.max);

		float adx = fabsf(d.x);
		if (fabsf(m.x) > e.x + adx) continue;
		float ady = fabsf(d.y);
		if (fabsf(m.y) > e.y + ady) continue;
		float adz = fabsf(d.z);
		if (fabsf(m.z) > e.z + adz) continue;

		adx += k_epsilon;
		ady += k_epsilon;
		adz += k_epsilon;

		if (fabsf(m.y * d.z - m.z * d.y) > e.y * adz + e.z * ady) continue;
		if (fabsf(m.z * d.x - m.x * d.z) > e.x * adz + e.z * adx) continue;
		if (fabsf(m.x * d.y - m.y * d.x) > e.x * ady + e.y * adx) continue;

		if (dynamicAABBTreeNode_isLeaf(n)) {
			if (!callback(cb, id)) return;
		} else {
			stack[sp++] = n->left;
			stack[sp++] = n->right;
		}
	}
}
