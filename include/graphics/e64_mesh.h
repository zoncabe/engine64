#ifndef ENGINE64_MESH_H
#define ENGINE64_MESH_H

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "render/e64_render.h"
#include "physics/math/e64_vector3.h"
#include "physics/math/e64_quaternion.h"


struct MeshDeform;

typedef struct {
	T3DVec3 min, max;
} MeshBound;

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

	/* Handed to the per-frame material setup when the mesh draws through the
	   object path; lets the owner scroll tiles or swap textures. NULL for
	   everything that has no business there. */
	T3DModelDrawConf *draw_conf;

	/* Model-space box of the whole mesh, taken once. */
	int16_t local_min[3];
	int16_t local_max[3];

	/* World box per model object, rebuilt with the matrix. Index 0 is the
	   whole mesh. */
	MeshBound *bound;
	uint8_t    bound_count;
	bool       culled;
} Mesh;


void mesh_initBounds(Mesh *mesh);

void mesh_setMatrix(Mesh *mesh, const RenderTransform *transform, uint8_t fb_index);

/* Same, but from a simulated body: position in metres and a quaternion, which
   is what a tumbling body actually has. */
void mesh_setMatrixFromBody(Mesh *mesh, const Vector3 *position, const Quaternion *rotation,
                            const Vector3 *scale, uint8_t fb_index);

/* Records part 0 (every object not in the list) plus one part per named
   object, in list order. Pass the skeleton segment placeholder as matrices
   for skinned models, NULL for static ones. */
void mesh_recordParts(Mesh *mesh, const char *const *names, uint8_t count, const T3DMat4FP *matrices);

/* Records one block per model object, in the object's own userBlock. */
void mesh_recordObjects(Mesh *mesh);

/* Hands the vertices over to an external set of points, matched by rest
   position. `scale` converts source units to render units. Pass source_normal
   to have the shading follow the deformation, and source_rgba to drive the
   vertex colors too; NULL keeps the model's own. */
bool mesh_setDeform(Mesh *mesh, const Vector3 *source, const Vector3 *source_normal,
                    const uint8_t *source_rgba, uint16_t source_count, float scale);

/* Pushes the current source positions and normals into the vertex buffer.
   No-op when the mesh is not deformed. */
void mesh_updateDeform(Mesh *mesh, uint8_t fb_index);

/* Points the deformed mesh at this frame's vertices. Call right before its
   display list runs. No-op when the mesh is not deformed. */
void mesh_bindDeformFrame(Mesh *mesh, uint8_t fb_index);

#endif
