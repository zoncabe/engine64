#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <libdragon.h>

#include "physics/collision/collision_mesh.h"


typedef struct RawCollisionHeader {
	uint32_t tri_count;
	uint32_t vert_count;
	float    coll_scale;
	uint32_t vertex_ptr;   /* reserved, written as 0 by the importer */
	uint32_t normals_ptr;  /* reserved, written as 0 by the importer */
	uint32_t bvh_ptr;      /* reserved, written as 0 by the importer */
} RawCollisionHeader;


static char *alignPtr(char *ptr, size_t alignment)
{
	return (char *)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
}


CollisionMesh *collisionMesh_load(const char *path)
{
	int size = 0;
	void *buffer = asset_load(path, &size);
	assert(buffer && size > (int)sizeof(RawCollisionHeader));

	const RawCollisionHeader *header = buffer;
	assert(header->tri_count > 0 && header->tri_count <= 0xFFFFu);
	assert(header->vert_count > 0 && header->vert_count <= 0xFFFFu);

	char *data = (char *)(header + 1);

	const uint16_t *indices = (const uint16_t *)data;
	data += header->tri_count * 3 * sizeof(uint16_t);
	data = alignPtr(data, 4);

	const int16_t *packed_normals = (const int16_t *)data;
	data += header->tri_count * 3 * sizeof(int16_t);
	data = alignPtr(data, 4);

	const Vector3 *vertices = (const Vector3 *)data;

	CollisionMesh *mesh = malloc(sizeof(CollisionMesh));
	mesh->triangle_count = (uint16_t)header->tri_count;
	mesh->vertex_count   = (uint16_t)header->vert_count;
	mesh->indices        = indices;
	mesh->packed_normals = packed_normals;
	mesh->vertices       = vertices;
	mesh->asset          = buffer;

	dynamicAABBTree_init(&mesh->tree);

	for (int32_t t = 0; t < mesh->triangle_count; t++) {
		Triangle triangle;
		collisionMesh_getTriangle(mesh, t, &triangle);
		const Vector3 *v = triangle.vertices;

		AABB aabb;
		aabb.min = v[0];
		aabb.max = v[0];
		for (int i = 1; i < 3; i++) {
			if (v[i].x < aabb.min.x) aabb.min.x = v[i].x;
			if (v[i].y < aabb.min.y) aabb.min.y = v[i].y;
			if (v[i].z < aabb.min.z) aabb.min.z = v[i].z;
			if (v[i].x > aabb.max.x) aabb.max.x = v[i].x;
			if (v[i].y > aabb.max.y) aabb.max.y = v[i].y;
			if (v[i].z > aabb.max.z) aabb.max.z = v[i].z;
		}

		dynamicAABBTree_insert(&mesh->tree, aabb, (void *)(intptr_t)t);
	}

	return mesh;
}

void collisionMesh_delete(CollisionMesh *mesh)
{
	if (!mesh) return;
	dynamicAABBTree_shutdown(&mesh->tree);
	free(mesh->asset);
	free(mesh);
}

void collisionMesh_getTriangle(const CollisionMesh *mesh, int32_t index, Triangle *out)
{
	const float scale = 1.0f / 32767.0f;
	const uint16_t *idx = &mesh->indices[index * 3];
	const int16_t  *n   = &mesh->packed_normals[index * 3];

	out->vertices[0] = mesh->vertices[idx[0]];
	out->vertices[1] = mesh->vertices[idx[1]];
	out->vertices[2] = mesh->vertices[idx[2]];
	out->normal      = (Vector3){ n[0] * scale, n[1] * scale, n[2] * scale };
}

void collisionMesh_queryAABB(const CollisionMesh *mesh, void *cb, PhysicsQueryCallback callback, AABB aabb)
{
	dynamicAABBTree_queryAABB(&mesh->tree, cb, callback, aabb);
}
