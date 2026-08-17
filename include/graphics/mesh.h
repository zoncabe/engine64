#ifndef RENDER_MESH_H
#define RENDER_MESH_H

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "render/render.h"
#include "physics/math/vector3.h"
#include "physics/math/quaternion.h"


struct MeshDeform;

typedef struct {
	rspq_block_t **dl;            /* one block per part */
	uint8_t        dl_count;
	uint8_t        visible;       /* bitmask: parts to render */
	T3DMat4FP     *matrix_buffer; /* NULL = matrix baked in dl (static mesh) */
	T3DModel      *model;
	T3DSkeleton   *skeleton;      /* NULL = static mesh (set by character_create) */

	/* Where the vertices come from when something else drives them. The
	   binding lives in its own module, so the mesh only needs to know it is
	   there. NULL = vertices come straight from the model. */
	struct MeshDeform *deform;
} Mesh;


void mesh_setMatrix(Mesh *mesh, const RenderTransform *transform, uint8_t fb_index);

/* Same, but from a simulated body: position in metres and a quaternion, which
   is what a tumbling body actually has. */
void mesh_setMatrixFromBody(Mesh *mesh, const Vector3 *position, const Quaternion *rotation,
                            const Vector3 *scale, uint8_t fb_index);

/* Records part 0 (every object not in the list) plus one part per named
   object, in list order. Pass the skeleton segment placeholder as matrices
   for skinned models, NULL for static ones. */
void mesh_recordParts(Mesh *mesh, const char *const *names, uint8_t count, const T3DMat4FP *matrices);

/* Hands the vertices over to an external set of points, matched by rest
   position. `scale` converts source units to render units. Pass source_normal
   to have the shading follow the deformation, or NULL to keep the model's. */
bool mesh_setDeform(Mesh *mesh, const Vector3 *source, const Vector3 *source_normal,
                    uint16_t source_count, float scale);

/* Pushes the current source positions and normals into the vertex buffer.
   No-op when the mesh is not deformed. */
void mesh_updateDeform(Mesh *mesh, uint8_t fb_index);

/* Points the deformed mesh at this frame's vertices. Call right before its
   display list runs. No-op when the mesh is not deformed. */
void mesh_bindDeformFrame(Mesh *mesh, uint8_t fb_index);

#endif
