#ifndef ENGINE64_MESH_H
#define ENGINE64_MESH_H

#include <libdragon.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "render/e64_render.h"
#include "math/e64_vector2.h"
#include "math/e64_vector3.h"
#include "math/e64_quaternion.h"
#include "model/e64_animation.h"

namespace e64 {

namespace mesh { class Deform; }

class Mesh {
public:

	/* Parts are selected through one bitmask, and that is what caps them. */
	static constexpr uint8_t MAX_PARTS = 8;

	/* Distinct materials alive at once, across every loaded model. */
	static constexpr uint8_t MAX_MATERIALS = 64;

	/* The id of an object that carries no material: nothing runs ahead of it. */
	static constexpr uint8_t MATERIAL_NONE = 0xFF;


	struct Def {

		const char *model;

		/* The model's clips and the graph that blends them. NULL: the model
		   is static. */
		const Animation::Def *animation;

		/* Objects inside the model the game drives on its own, named as they
		   are named in the model, so it can show and hide each one. Left out,
		   the model is drawn whole. */
		const char *const *part;

		/* Where each of those objects is drawn, in the entity's own space and
		   in the same order as the names. A part left at zero stays where it
		   was modelled. */
		const Vector3 *part_position;

		uint8_t part_count;

	};


	/* Every model draws through its objects, read from the model's own chunk
	   table (mesh::object). Each object owns a block with its geometry alone
	   (userBlock) and keeps its material's table id in userValue0; the
	   material lives in the global material table, shared with every other
	   object of the same material, whatever model it came from. object_part
	   is the part of each object, in the model's object order. */
	uint8_t *object_part; /* 0 = unnamed remainder, 1.. = named part */

	/* The bone matrices the geometry was recorded against: the skeleton
	   segment placeholder for a skinned model, NULL for a static one. Kept
	   for a later re-recording. */
	const T3DMat4FP *bone_matrices;

	uint8_t visible; /* bitmask: parts to render */

	/* The names the parts were recorded from, one per named part. Part 0 has
	   no name: it is everything the list did not claim. */
	const char **part_name;
	uint8_t part_count;

	/* A named part can be drawn away from the rest of the model. Its offset is
	   held in the entity's own space and turned into a matrix of its own every
	   frame, one per framebuffer. Both stay NULL until a part is actually
	   offset, and a part left at zero keeps using the entity's matrix.
	   Indexed by named part, so part 1 is the first entry. */
	Render::Transform *part_offset;
	T3DMat4FP *part_matrix;

	T3DMat4FP *matrix_buffer; /* NULL = matrix baked in dl (static mesh) */
	T3DModel *model;
	T3DSkeleton *skeleton; /* NULL = static mesh; the animation's pose otherwise */

	/* The clips playing on the model and the pose they leave, opened with the
	   mesh when its def declares an animation. NULL = static mesh. */
	Animation *animation;

	/* Where the vertices come from when something else drives them. The
	   binding lives in its own module, so the mesh only needs to know it is
	   there. NULL = vertices come straight from the model. */
	mesh::Deform *deform;

	/* Scroll of the two texture tiles, s/t in texels, added on top of the
	   file's own translate each frame after the shared material runs: the
	   material stays in its block and only SET_TILE_SIZE is reissued. NULL
	   for everything that does not scroll. */
	const Vector2 *texture_scroll;

	/* Model-space box of the whole mesh, taken once. */
	int16_t local_min[3];
	int16_t local_max[3];

	/* Where the entity stands, in render units, as last handed to the
	   matrix. The frustum is moved into the model's space with these, so
	   every box stays the one the file wrote. */
	Vector3 scale;
	Quaternion rotation;
	Vector3 position;
	bool culled;
};


namespace mesh {

/* The model's object at index i, NULL past the last one. The exporter writes
   the object chunks first in the chunk table, one after the other, so the
   objects are entries 0..n-1 of it and the walk stops at the first entry of
   another type. */
inline T3DObject *object(const Mesh *mesh, uint16_t i)
{
	const T3DModel *model = mesh->model;
	if (i >= model->chunkCount || model->chunkOffsets[i].type != T3D_CHUNK_TYPE_OBJECT) return NULL;
	return (T3DObject *)((char *)model + (model->chunkOffsets[i].offset & 0x00FFFFFF));
}

/* Loads the model, opens its animation when the def has one and records it
   with every part on screen. */
Mesh *create(const Mesh::Def *def);
void destroy(Mesh *mesh);

void initBounds(Mesh *mesh);

/* Brings the frustum into the model's space and tests the file's own boxes
   against it: culled for the whole mesh, isVisible per model object. A static
   model tests every object, through the model's BVH when the file carries
   one; a skinned model only tests its named parts, since the rest of its
   boxes are written per bone and say nothing about the posed mesh. */
void cull(Mesh *mesh, const T3DViewport *viewport);

void setMatrix(Mesh *mesh, const Render::Transform *transform, uint8_t fb_index);

/* Same, but from a simulated body: position in metres and a quaternion, which
   is what a tumbling body actually has. */
void setMatrixFromBody(Mesh *mesh, const Vector3 *position, const Quaternion *rotation,
                       const Vector3 *scale, uint8_t fb_index);

/* Records the model: one geometry block per object, and its material shared
   through the global table. Objects named in the list become parts 1..count,
   in list order; every other object is part 0. Pass the skeleton segment
   placeholder as matrices for skinned models, NULL for static ones. The parts
   share one visibility bitmask, so a model can declare at most
   Mesh::MAX_PARTS - 1 names. */
void record(Mesh *mesh, const char *const *names, uint8_t count, const T3DMat4FP *matrices);

/* Records a geometry block for every object that has none: vertices, bone
   matrix pushes, triangles, no material. Part of record; deform runs it
   again after rewriting the vertex addresses. */
void recordGeometry(Mesh *mesh);

/* Gives the materials back to the table and frees the mesh's own lists. The
   geometry blocks belong to the model and go with it. */
void release(Mesh *mesh);


namespace part {

/* Part index for a recorded name, 0 when the mesh has no part by that name.
   Part 0 is the unnamed remainder, so it never comes back from a lookup. */
uint8_t find(const Mesh *mesh, const char *name);

/* Turns one recorded part on or off. Everything the parts left over sits in
   part 0, which is on unless something turns it off too. */
void setVisible(Mesh *mesh, uint8_t part, bool visible);

bool isVisible(const Mesh *mesh, uint8_t part);

/* Moves one named part away from the rest of the model, in the entity's own
   space: position in render units, rotation in degrees, and a zero scale left
   as original size. The part is given matrices of its own the first time it is
   offset, and follows the entity from then on. */
void setOffset(Mesh *mesh, uint8_t part, const Render::Transform *offset);

/* The offset a part is currently drawn with, NULL when it has none. */
const Render::Transform *getOffset(const Mesh *mesh, uint8_t part);

/* The matrix a part has to be drawn with: its own when it is offset, and the
   mesh's otherwise. Part 0 always draws with the mesh's. */
const T3DMat4FP *getMatrix(const Mesh *mesh, uint8_t part, uint8_t fb_index);

}


/* Global material table: one recorded block per distinct material, whatever
   model it came from, shared by every object that uses it. */
namespace material {

/* The id of a material, recording it the first time it is seen. Every call
   holds one user. Used by record. */
uint8_t acquire(const T3DMaterial *material);

/* Drops one user; the block goes with the last one. Used by mesh::release. */
void release(uint8_t id);

/* Read by the render. The block of a material id, NULL for
   Mesh::MATERIAL_NONE. */
rspq_block_t *block(uint8_t id);

/* Whether the material sets a vertex FX, which has to be reset after the
   last material of the frame ran. */
bool hasVertexFx(uint8_t id);

/* Whether the material depends on what was drawn before it: alpha blending
   and decals. Those keep their scene order instead of grouping by material. */
bool isDeferred(uint8_t id);

}

}

}

#endif
