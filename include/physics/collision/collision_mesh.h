/*
	collision_mesh.h — static triangle mesh collision data.

	Loads the binary written by tools/collision_importer and builds a
	dynamicAABBTree with one leaf per triangle, in mesh-local space.
	File layout based on pyrite64's mesh collider (Max Bebök, Kevin Reier, MIT).
*/
#ifndef PHYSICS_COLLISION_MESH_H
#define PHYSICS_COLLISION_MESH_H

#include <stdint.h>

#include "physics/math/vector3.h"
#include "physics/geometry/aabb.h"
#include "physics/geometry/triangle.h"
#include "physics/broadphase/dynamic_aabb_tree.h"


typedef struct CollisionMesh {
	uint16_t        triangle_count;
	uint16_t        vertex_count;

	const uint16_t *indices;         /* 3 per triangle */
	const int16_t  *packed_normals;  /* 3 per triangle, scaled by 32767 */
	const Vector3  *vertices;

	void           *asset;           /* buffer from asset_load, owns the data above */
	DynamicAABBTree tree;            /* leaf per triangle, user_data = triangle index */
} CollisionMesh;


CollisionMesh *collisionMesh_load  (const char *path);
void           collisionMesh_delete(CollisionMesh *mesh);

void collisionMesh_getTriangle(const CollisionMesh *mesh, int32_t index, Triangle *out);

void collisionMesh_queryAABB(const CollisionMesh *mesh, void *cb, PhysicsQueryCallback callback, AABB aabb);


#endif
