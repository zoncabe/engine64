#ifndef RENDER_MESH_H
#define RENDER_MESH_H

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "render/render.h"


typedef struct {
	rspq_block_t **dl;            // one block per part
	uint8_t        dl_count;
	uint8_t        visible;       // bitmask: parts to render
	T3DMat4FP     *matrix_buffer; // NULL = matrix baked in dl (static mesh)
	T3DModel      *model;
	T3DSkeleton   *skeleton;      // NULL = static mesh (set by character_create)
} Mesh;


void mesh_setMatrix(Mesh *mesh, const RenderTransform *transform, uint8_t fb_index);

/* Records part 0 (every object not in the list) plus one part per named
   object, in list order. Pass the skeleton segment placeholder as matrices
   for skinned models, NULL for static ones. */
void mesh_recordParts(Mesh *mesh, const char *const *names, uint8_t count, const T3DMat4FP *matrices);

#endif
