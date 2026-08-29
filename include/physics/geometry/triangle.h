/*
	Triangle primitive for static collision meshes. Vertices gathered by index
	from a CollisionMesh, normal precomputed.
*/
#ifndef PHYSICS_GEOMETRY_TRIANGLE_H
#define PHYSICS_GEOMETRY_TRIANGLE_H

#include <stdint.h>

#include "physics/math/vector3.h"
#include "physics/geometry/raycast.h"


typedef struct Triangle {
	Vector3 vertices[3];
	Vector3 normal;
	uint8_t active_edges;   /* bit 0 = v0v1, bit 1 = v1v2, bit 2 = v2v0; baked by the importer */
} Triangle;


int triangle_raycast(const Triangle *t, RaycastData *raycast);


#endif
