/*
	Triangle primitive for static collision meshes. Vertices gathered by index
	from a CollisionMesh, normal precomputed.
*/
#ifndef PHYSICS_GEOMETRY_TRIANGLE_H
#define PHYSICS_GEOMETRY_TRIANGLE_H

#include "physics/math/vector3.h"


typedef struct Triangle {
	Vector3 vertices[3];
	Vector3 normal;
} Triangle;


#endif
