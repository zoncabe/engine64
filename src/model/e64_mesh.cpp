
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>

#include "model/e64_mesh.h"
#include "model/e64_mesh_deform.h"
#include "resource/e64_resource.h"
#include "engine/e64_common.h"
#include "math/e64_math.h"
#include "math/e64_quaternion.h"

namespace e64 {
namespace mesh {

/* ------------------------------------------------------------------------ */
/* Material table                                                            */
/* ------------------------------------------------------------------------ */

namespace material {

/* One recorded block per distinct material, whatever model it came from.
   Every object using it holds one user; the block goes when the last one
   leaves. A material is the RDP state its block emits, so two materials are
   the same when every field that reaches the block is the same. */
typedef struct {
	const T3DMaterial *material; /* the first one registered, NULL = free */
	rspq_block_t *block;
	uint16_t users;
	bool vertex_fx;
	bool deferred;
} Entry;

static Entry table[Mesh::MAX_MATERIALS];

/* A material state that matches nothing, so a material recorded against it
   emits every setting it carries. t3d's own fresh state starts at zero and
   skips a value equal to zero (vertex FX none, draw flags none), which in a
   block of its own would make the material inherit whatever ran before. */
static T3DModelState fullState(void)
{
	T3DModelState state = t3d_model_state_create();
	state.lastRenderFlags = 0xFFFFFFFF;
	state.lastVertFXFunc = 0xFF;
	state.lastTextureHashA = 0xFFFFFFFF;
	state.lastTextureHashB = 0xFFFFFFFF;
	state.lastPrimColor = RGBA32(1, 2, 3, 4);
	state.lastEnvColor = RGBA32(1, 2, 3, 4);
	state.lastBlendColor = RGBA32(1, 2, 3, 4);
	return state;
}

/* The texture as the block uploads it: which one (the hash of its ROM path,
   the same identity t3d caches the sprite by) and how its tile is set. The
   path string and the sprite pointer say nothing the hash does not. */
static bool textureEquals(const T3DMaterialTexture *a, const T3DMaterialTexture *b)
{
	return a->texReference == b->texReference
	    && a->textureHash == b->textureHash
	    && a->texWidth == b->texWidth
	    && a->texHeight == b->texHeight
	    && memcmp(&a->s, &b->s, sizeof(T3DMaterialAxis)) == 0
	    && memcmp(&a->t, &b->t, sizeof(T3DMaterialAxis)) == 0;
}

static bool equals(const T3DMaterial *a, const T3DMaterial *b)
{
	return a->colorCombiner == b->colorCombiner
	    && a->otherModeValue == b->otherModeValue
	    && a->otherModeMask == b->otherModeMask
	    && a->blendMode == b->blendMode
	    && a->renderFlags == b->renderFlags
	    && a->fogMode == b->fogMode
	    && a->setColorFlags == b->setColorFlags
	    && a->vertexFxFunc == b->vertexFxFunc
	    && color_to_packed32(a->primColor) == color_to_packed32(b->primColor)
	    && color_to_packed32(a->envColor) == color_to_packed32(b->envColor)
	    && color_to_packed32(a->blendColor) == color_to_packed32(b->blendColor)
	    && textureEquals(&a->textureA, &b->textureA)
	    && textureEquals(&a->textureB, &b->textureB);
}

/* The pointer is tried first: objects of one file share their material
   chunk, and every entity of one model shares the file. */
uint8_t acquire(const T3DMaterial *material)
{
	if (!material) return Mesh::MATERIAL_NONE;

	Entry *free_slot = NULL;

	for (uint8_t i = 0; i < Mesh::MAX_MATERIALS; i++) {
		Entry *entry = &table[i];
		if (!entry->material) {
			if (!free_slot) free_slot = entry;
			continue;
		}
		if (entry->material == material) {
			entry->users++;
			return i;
		}
	}

	for (uint8_t i = 0; i < Mesh::MAX_MATERIALS; i++) {
		Entry *entry = &table[i];
		if (entry->material && equals(entry->material, material)) {
			entry->users++;
			return i;
		}
	}

	assert(free_slot);

	T3DModelState state = fullState();
	rspq_block_begin();
	t3d_model_draw_material((T3DMaterial *)material, &state);
	free_slot->block = rspq_block_end();

	free_slot->material = material;
	free_slot->users = 1;
	free_slot->vertex_fx = material->vertexFxFunc != T3D_VERTEX_FX_NONE;

	/* Alpha blending reads what is already in the framebuffer, a decal reads
	   the depth of the surface under it: both need their scene order. The
	   exporter sorts a file's objects that way; across entities the render
	   has to. */
	free_slot->deferred = material->blendMode != 0
	                   || (material->otherModeValue & SOM_ZMODE_MASK) == SOM_ZMODE_DECAL;

	return (uint8_t)(free_slot - table);
}

void release(uint8_t id)
{
	if (id == Mesh::MATERIAL_NONE) return;

	Entry *entry = &table[id];
	assert(entry->users > 0);

	if (--entry->users > 0) return;

	rspq_block_free(entry->block);
	entry->block = NULL;
	entry->material = NULL;
}

rspq_block_t *block(uint8_t id)
{
	return id == Mesh::MATERIAL_NONE ? NULL : table[id].block;
}

bool hasVertexFx(uint8_t id)
{
	return id != Mesh::MATERIAL_NONE && table[id].vertex_fx;
}

bool isDeferred(uint8_t id)
{
	return id != Mesh::MATERIAL_NONE && table[id].deferred;
}

}


/* ------------------------------------------------------------------------ */
/* Bounds and culling                                                        */
/* ------------------------------------------------------------------------ */

/* A skinned model's boxes are written per bone, so none of them says where the
   mesh ends up. Two things bound that for good: a vertex never leaves its own
   bone by more than the model box reaches, and a bone never leaves the root by
   more than its chain is long. The box that holds both holds every pose the
   rig can take, so it is measured once and never again. */
static void skinnedBound(Mesh *mesh, const T3DChunkSkeleton *skeleton)
{
	float reach = 0.0f;
	for (int i = 0; i < 3; i++) {
		float lo = -mesh->model->aabbMin[i];
		float hi = mesh->model->aabbMax[i];
		if (lo > reach) reach = lo;
		if (hi > reach) reach = hi;
	}

	float chain[skeleton->boneCount];
	float longest = 0.0f;

	for (int b = 0; b < skeleton->boneCount; b++) {
		const T3DChunkBone *bone = &skeleton->bones[b];
		T3DVec3 p = bone->position;

		chain[b] = sqrtf(p.v[0]*p.v[0] + p.v[1]*p.v[1] + p.v[2]*p.v[2]);
		if (bone->parentIdx < b) chain[b] += chain[bone->parentIdx];
		if (chain[b] > longest) longest = chain[b];
	}

	int16_t half = (int16_t)(longest + reach);
	for (int i = 0; i < 3; i++) {
		mesh->local_min[i] = -half;
		mesh->local_max[i] = half;
	}
}

void initBounds(Mesh *mesh)
{
	mesh->transform = transform3d::identity();
	mesh->culled = false;

	const T3DChunkSkeleton *skeleton = t3d_model_get_skeleton(mesh->model);
	if (skeleton) {
		skinnedBound(mesh, skeleton);
		return;
	}

	for (int i = 0; i < 3; i++) {
		mesh->local_min[i] = mesh->model->aabbMin[i];
		mesh->local_max[i] = mesh->model->aabbMax[i];
	}
}

/* The frustum in the space of a placement. A world plane n·x + w, for a point
   placed by scale S, rotation R and translation t, reads (S·Rᵀn)·x + (w + n·t)
   on the model's own coordinates. The box test only looks at the sign, so the
   planes need no renormalising. The frustum and the boxes are in render
   units, so the placement's position is taken there. */
static void localFrustum(T3DFrustum *out, const T3DFrustum *world, const Transform3D *placement)
{
	const Quaternion *rotation = &placement->rotation;
	const Vector3 *scale = &placement->scale;

	Quaternion inverse = { -rotation->x, -rotation->y, -rotation->z, rotation->w };
	Vector3 position = {
		placement->position.x * RENDER_SCALE,
		placement->position.y * RENDER_SCALE,
		placement->position.z * RENDER_SCALE,
	};

	for (int i = 0; i < 6; i++) {
		const float *plane = world->planes[i].v;
		Vector3 n = { plane[0], plane[1], plane[2] };
		Vector3 r = quaternion::rotateVector(&inverse, &n);

		out->planes[i].v[0] = r.x * scale->x;
		out->planes[i].v[1] = r.y * scale->y;
		out->planes[i].v[2] = r.z * scale->z;
		out->planes[i].v[3] = plane[3] + n.x * position.x + n.y * position.y + n.z * position.z;
	}
}

void cull(Mesh *mesh, const T3DViewport *viewport)
{
	T3DFrustum frustum;
	localFrustum(&frustum, &viewport->viewFrustum, &mesh->transform);

	bool in_box = t3d_frustum_vs_aabb_s16(&frustum, mesh->local_min, mesh->local_max);

	/* The model's own box only holds what was modelled inside it, so a model
	   with a part drawn somewhere else cannot be cut by it: that part would go
	   down with a box it was never in. Those go to the per object test below,
	   which is the only one that knows where the pieces ended up. */
	mesh->culled = mesh->part_offset ? false : !in_box;
	if (mesh->culled) return;

	/* A static file with a BVH, drawn where it was modelled, walks its boxes
	   by node and marks only what it reaches, so everything starts off. */
	const T3DBvh *bvh = mesh->skeleton ? NULL : t3d_model_bvh_get(mesh->model);
	if (bvh && !mesh->part_offset) {
		for (uint16_t i = 0; T3DObject *o = object(mesh, i); i++) o->isVisible = false;
		t3d_model_bvh_query_frustum(bvh, &frustum);
		return;
	}

	/* A part drawn away from the rest of the model brings the frustum into
	   its own space, or it would be cut by where it was modelled instead of
	   where it ends up. Part 0 stays where the model is. */
	T3DFrustum part_frustum[Mesh::MAX_PARTS];
	const T3DFrustum *test[Mesh::MAX_PARTS];

	test[0] = &frustum;
	for (int p = 1; p <= mesh->part_count; p++) {
		test[p] = &frustum;
		if (!mesh->part_offset) continue;

		Transform3D placed = transform3d::product(&mesh->transform, &mesh->part_offset[p - 1]);
		localFrustum(&part_frustum[p], &viewport->viewFrustum, &placed);
		test[p] = &part_frustum[p];
	}

	for (uint16_t i = 0; T3DObject *o = object(mesh, i); i++) {
		uint8_t part = mesh->object_part[i];

		/* A skinned body's object boxes are written per bone and say nothing
		   about the pose: it rides the whole-model box tested above. Its named
		   parts keep their own box, where the file modelled them. */
		if (mesh->skeleton && part == 0) {
			o->isVisible = in_box;
			continue;
		}

		o->isVisible = t3d_frustum_vs_aabb_s16(test[part], o->aabbMin, o->aabbMax);
	}
}


/* ------------------------------------------------------------------------ */
/* Placement                                                                 */
/* ------------------------------------------------------------------------ */

/* An offset part is drawn with the model's placement carrying its own on
   top, so it follows whatever the model does and keeps its displacement
   inside it. */
static void setPartMatrices(Mesh *mesh, uint8_t fb_index)
{
	if (!mesh->part_matrix) return;

	for (int i = 0; i < mesh->part_count; i++) {
		Transform3D placed = transform3d::product(&mesh->transform, &mesh->part_offset[i]);
		transform3d::toMatrix(&placed, &mesh->part_matrix[i * Viewport::FB_COUNT + fb_index]);
	}
}


/* The placement is kept as the mesh's own transform: the RSP matrix is
   written from it, and the culling moves the frustum with it. */
void setMatrix(Mesh *mesh, const Transform3D *transform, uint8_t fb_index)
{
	mesh->transform = *transform;

	transform3d::toMatrix(&mesh->transform, &mesh->matrix_buffer[fb_index]);
	setPartMatrices(mesh, fb_index);
}


/* ------------------------------------------------------------------------ */
/* Recording                                                                 */
/* ------------------------------------------------------------------------ */

/* Every object gets a block with its geometry alone: vertices, bone matrix
   pushes, triangles. No material inside, so the block runs under whatever
   material the render put up before it. The block hangs off the object,
   which belongs to the model: entities of one model share it, the first one
   records it and t3d_model_free releases it with the model. */
void recordGeometry(Mesh *mesh)
{
	for (uint16_t i = 0; T3DObject *o = object(mesh, i); i++) {
		if (o->userBlock) continue;
		rspq_block_begin();
		t3d_model_draw_object(o, mesh->bone_matrices);
		o->userBlock = rspq_block_end();
	}
}

void record(Mesh *mesh, const char *const *names, uint8_t count, const T3DMat4FP *matrices)
{
	assert(1 + count <= Mesh::MAX_PARTS);

	/* mesh::object reads the objects from the start of the chunk table, where
	   the exporter writes them. A file laid out otherwise is caught here. */
	assert(mesh->model->chunkCount > 0 && mesh->model->chunkOffsets[0].type == T3D_CHUNK_TYPE_OBJECT);

	uint16_t total = 0;
	while (object(mesh, total)) total++;

	mesh->object_part = (uint8_t *)malloc(total);
	mesh->bone_matrices = matrices;
	assert(mesh->object_part);

	/* The names are kept so a part can be found by the name it was declared
	   with; only the pointers are copied, the strings stay where they are. */
	mesh->part_count = count;
	mesh->part_name = count ? (const char **)malloc(sizeof(char *) * count) : NULL;
	mesh->part_offset = NULL;
	mesh->part_matrix = NULL;
	for (int i = 0; i < count; i++) mesh->part_name[i] = names[i];

	/* Each object: its material through the table, kept in the object, and
	   its part by name, part 0 for everything the list did not claim. Every
	   mesh of the file holds its own user of each material; the id comes out
	   the same for all of them. */
	for (uint16_t n = 0; T3DObject *o = object(mesh, n); n++) {
		o->userValue0 = material::acquire(o->material);
		mesh->object_part[n] = 0;

		for (int p = 0; p < count; p++)
			if (o->name && strcmp(o->name, names[p]) == 0)
				mesh->object_part[n] = (uint8_t)(1 + p);
	}

	recordGeometry(mesh);

	/* Part 0 starts on screen; the named parts are the caller's to turn on. */
	mesh->visible = 1;
}

/* The geometry blocks stay with the model, which other entities may still
   be drawing; only the materials are given back. */
void release(Mesh *mesh)
{
	for (uint16_t i = 0; T3DObject *o = object(mesh, i); i++)
		material::release(o->userValue0);

	free(mesh->object_part);
	free(mesh->part_name);
	free(mesh->part_offset);
	if (mesh->part_matrix) free_uncached(mesh->part_matrix);

	mesh->object_part = NULL;
	mesh->part_name = NULL;
	mesh->part_count = 0;
	mesh->part_offset = NULL;
	mesh->part_matrix = NULL;
}


/* ------------------------------------------------------------------------ */
/* Lifetime                                                                  */
/* ------------------------------------------------------------------------ */

Mesh *create(const Mesh::Def *def)
{
	/* Zeroed: what the def leaves out stays NULL, and destroy frees every
	   pointer it finds. */
	Mesh *mesh = (Mesh *)calloc(1, sizeof(Mesh));
	assert(mesh);
	mesh->model = (T3DModel *)resource::load(def->model, Resource::MODEL, NULL);
	mesh->matrix_buffer = (T3DMat4FP *)malloc_uncached(sizeof(T3DMat4FP) * Viewport::FB_COUNT);
	assert(mesh->matrix_buffer);
	initBounds(mesh);

	/* An animated model records against the skeleton segment, filled each
	   frame from the animation's pose; a static one records with none. */
	const T3DMat4FP *bones = NULL;
	if (def->animation) {
		mesh->animation = (Animation *)malloc(sizeof(Animation));
		assert(mesh->animation);
		animation::init(mesh->animation, def->animation, mesh->model);
		mesh->skeleton = &mesh->animation->main;
		bones = (const T3DMat4FP *)t3d_segment_placeholder(T3D_SEGMENT_SKELETON);
	}

	/* Every part starts on screen: the model shows whole until the game
	   decides to hide something. */
	record(mesh, def->part, def->part_count, bones);
	mesh->visible = (uint8_t)((2u << mesh->part_count) - 1);

	/* A part declared away from where it was modelled gets its offset here,
	   so it is already in place the first time it is drawn. */
	for (int i = 0; def->part_position && i < def->part_count; i++) {
		const Vector3 *position = &def->part_position[i];
		if (position->x == 0.0f && position->y == 0.0f && position->z == 0.0f)
			continue;

		Transform3D offset = transform3d::identity();
		offset.position = *position;
		part::setOffset(mesh, 1 + i, &offset);
	}

	/* The model stands where its def says from the start, in every
	   framebuffer: whatever never moves is never written again. The parts
	   are already offset, so their matrices come out with it. */
	Transform3D placed = transform3d::fromDeclaration(&def->transform);
	for (int fb = 0; fb < Viewport::FB_COUNT; fb++)
		setMatrix(mesh, &placed, fb);

	return mesh;
}

void destroy(Mesh *mesh)
{
	release(mesh);
	deform::destroy(mesh);

	if (mesh->animation) {
		animation::destroy(mesh->animation);
		free(mesh->animation);
	}

	free_uncached(mesh->matrix_buffer);
	resource::unload(mesh->model);
	free(mesh);
}


/* ------------------------------------------------------------------------ */
/* Parts                                                                     */
/* ------------------------------------------------------------------------ */

namespace part {

uint8_t find(const Mesh *mesh, const char *name)
{
	for (int i = 0; i < mesh->part_count; i++)
		if (strcmp(mesh->part_name[i], name) == 0) return 1 + i;
	return 0;
}

void setVisible(Mesh *mesh, uint8_t part, bool visible)
{
	assert(part <= mesh->part_count);

	if (visible) mesh->visible |= (uint8_t)(1u << part);
	else mesh->visible &= (uint8_t)~(1u << part);
}

bool isVisible(const Mesh *mesh, uint8_t part)
{
	return (mesh->visible & (1u << part)) != 0;
}

void setOffset(Mesh *mesh, uint8_t part, const Transform3D *offset)
{
	assert(part > 0 && part <= mesh->part_count);

	/* Nothing is reserved until a part is actually moved: a model whose parts
	   only ever show and hide pays for none of this. */
	if (!mesh->part_offset) {
		mesh->part_offset = (Transform3D *)malloc(sizeof(Transform3D) * mesh->part_count);
		assert(mesh->part_offset);

		/* The parts not being moved have to come out where they already were,
		   so they start at no offset and original size, not at zero. */
		for (int i = 0; i < mesh->part_count; i++)
			mesh->part_offset[i] = transform3d::identity();

		mesh->part_matrix = (T3DMat4FP *)malloc_uncached(sizeof(T3DMat4FP) * mesh->part_count * Viewport::FB_COUNT);
		assert(mesh->part_matrix);
		for (int i = 0; i < mesh->part_count * Viewport::FB_COUNT; i++)
			t3d_mat4fp_identity(&mesh->part_matrix[i]);
	}

	mesh->part_offset[part - 1] = transform3d::fromDeclaration(offset);
}

const Transform3D *getOffset(const Mesh *mesh, uint8_t part)
{
	if (!mesh->part_offset || part == 0 || part > mesh->part_count) return NULL;
	return &mesh->part_offset[part - 1];
}

const T3DMat4FP *getMatrix(const Mesh *mesh, uint8_t part, uint8_t fb_index)
{
	if (!mesh->part_matrix || part == 0 || part > mesh->part_count)
		return mesh->matrix_buffer ? &mesh->matrix_buffer[fb_index] : NULL;

	return &mesh->part_matrix[(part - 1) * Viewport::FB_COUNT + fb_index];
}

}

}
}
