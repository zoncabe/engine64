/*
	Drives a t3d vertex buffer from an external set of points.

	Binds every vertex slot of a model to the source point sitting at the same
	rest position, then rewrites the buffer from those points on demand. The
	source is a plain array, so anything that moves points can drive a mesh:
	a collision mesh under simulation, a morph target, a spline.

	Binding by position rather than by index is what makes this work: t3d
	chunks, pads and reorders vertices when it builds the model, and glTF
	splits them again per UV seam and material border, so one source point
	usually lands on several slots and never on a predictable one.
*/
#ifndef ENGINE64_MESH_DEFORM_H
#define ENGINE64_MESH_DEFORM_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dmodel.h>

#include "viewport/e64_viewport.h" /* Viewport::FB_COUNT */
#include "math/e64_vector3.h"
#include "model/e64_mesh.h"

namespace e64 {

class Entity;

namespace mesh {

class Deform {
public:

	static constexpr uint16_t UNBOUND = 0xFFFF;

	/* One vertex buffer per framebuffer, same reason the matrices are buffered. */
	static constexpr uint8_t BUFFERS = Viewport::FB_COUNT;

	/* First t3d segment free for deformed vertices (valid ids are 1-7). Segment 1
	   is the one the skinning placeholder uses, so the pool starts at 2; each
	   binding takes the lowest free id and keeps it until deleted. */
	static constexpr uint8_t SEGMENT = 2;


	uint16_t *slot_source; /* source index per vertex slot, or UNBOUND */
	uint16_t slot_count; /* vertex slots in the model */
	uint16_t bound_count; /* slots that found a source point */
	float scale; /* source units -> render units */
	T3DModel *model;
	const Vector3 *source; /* the points driving the mesh; kept, not owned */

	/* The RSP runs behind the CPU, so writing one buffer every frame lets it
	   read vertices half-overwritten. One copy per framebuffer, addressed
	   through a t3d segment, keeps the frame being drawn intact. */
	T3DVertPacked *vertex_buffer[BUFFERS];
	uint32_t vertex_bytes;
	uint8_t segment;

	/* Optional: one normal per source point, in the same order. The source
	   decides its own winding, which may run against the model's, so the
	   binding measures the two at rest and keeps the sign that agrees. */
	const Vector3 *source_normal;
	float normal_sign;

	/* Optional: one RGBA per source point, written into the vertex colors.
	   NULL leaves the model's own colors alone. */
	const uint8_t *source_rgba;
};


namespace deform {

/* Hands the mesh's vertices over to an external set of points, matched by
   rest position; both must still be at rest. The source array is kept, so it
   has to outlive the binding and keep its order. `scale` converts source units
   to render units. Pass source_normal to have the shading follow, and
   source_rgba to drive the vertex colors too; NULL keeps the model's own.
   Returns false only if the binding could not be allocated; a partial match
   counts as success and leaves the unmatched slots frozen. */
bool set(Mesh *mesh, const Vector3 *source, const Vector3 *source_normal,
         const uint8_t *source_rgba, uint16_t source_count, float scale);

/* Writes the current source positions, and normals if bound, into this
   frame's vertex buffer of every deformed mesh among these entities, and
   points each mesh's t3d segment at it. Runs once per frame, after whatever
   drives the sources has moved. */
void update(Entity *const *entity, uint8_t count, uint8_t fb_index);

/* Releases the binding and its buffers. No-op when the mesh is not deformed. */
void destroy(Mesh *mesh);

}

}

}

#endif
