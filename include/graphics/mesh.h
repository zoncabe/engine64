#ifndef RENDER_MESH_H
#define RENDER_MESH_H

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>

#include "render/render.h"


typedef struct {
	rspq_block_t *dl;
	T3DMat4FP    *matrix_buffer;  // NULL = matrix baked in dl (static mesh)
	T3DModel     *model;
} Mesh;


void mesh_setMatrix(Mesh *mesh, const RenderTransform *transform, uint8_t fb_index);

#endif
