
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <t3d/t3dmodel.h>

#include "graphics/mesh.h"
#include "shaders/mesh_deform.h"
#include "physics/math/math_common.h"
#include "physics/math/quaternion.h"


/* A skinned model's boxes are written per bone, so none of them says where the
   mesh ends up. Two things bound that for good: a vertex never leaves its own
   bone by more than the model box reaches, and a bone never leaves the root by
   more than its chain is long. The box that holds both holds every pose the
   rig can take, so it is measured once and never again. */
static void mesh_skinnedBound(Mesh *mesh, const T3DChunkSkeleton *skeleton)
{
	float reach = 0.0f;
	for (int i = 0; i < 3; i++) {
		float lo = -mesh->model->aabbMin[i];
		float hi =  mesh->model->aabbMax[i];
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
		mesh->local_max[i] =  half;
	}
}

void mesh_initBounds(Mesh *mesh)
{
	uint8_t count = 0;
	T3DModelIter it = t3d_model_iter_create(mesh->model, T3D_CHUNK_TYPE_OBJECT);
	while (t3d_model_iter_next(&it)) count++;

	mesh->bound_count = 1 + count;
	mesh->bound = calloc(mesh->bound_count, sizeof(MeshBound));
	assert(mesh->bound);
	mesh->culled = false;

	const T3DChunkSkeleton *skeleton = t3d_model_get_skeleton(mesh->model);
	if (skeleton) {
		mesh_skinnedBound(mesh, skeleton);
		return;
	}

	for (int i = 0; i < 3; i++) {
		mesh->local_min[i] = mesh->model->aabbMin[i];
		mesh->local_max[i] = mesh->model->aabbMax[i];
	}
}

/* Places a model-space box in the world without walking its corners: the
   centre goes through the matrix, and the half-extent through its absolute
   value, which is the axis-aligned box that still contains it after any
   rotation. */
static void mesh_placeBound(MeshBound *bound, const int16_t *min, const int16_t *max, const T3DMat4 *m)
{
	float centre[3], extent[3];
	for (int i = 0; i < 3; i++) {
		centre[i] = (max[i] + min[i]) * 0.5f;
		extent[i] = (max[i] - min[i]) * 0.5f;
	}

	for (int i = 0; i < 3; i++) {
		float c = m->m[3][i];
		float e = 0.0f;

		for (int k = 0; k < 3; k++) {
			c += centre[k] * m->m[k][i];
			e += extent[k] * fabsf(m->m[k][i]);
		}

		bound->min.v[i] = c - e;
		bound->max.v[i] = c + e;
	}
}

static void mesh_updateBounds(Mesh *mesh, const T3DMat4 *matrix)
{
	if (mesh->bound == NULL) return;

	mesh_placeBound(&mesh->bound[0], mesh->local_min, mesh->local_max, matrix);

	uint8_t i = 1;
	T3DModelIter it = t3d_model_iter_create(mesh->model, T3D_CHUNK_TYPE_OBJECT);
	while (t3d_model_iter_next(&it) && i < mesh->bound_count)
		mesh_placeBound(&mesh->bound[i++], it.object->aabbMin, it.object->aabbMax, matrix);
}


/* A simulated body tumbles, and euler angles cannot describe that without
   picking an order and losing the tumble at the poles. The body already keeps
   a quaternion, so it goes straight to the matrix. */
void mesh_setMatrixFromBody(Mesh *mesh, const Vector3 *position, const Quaternion *rotation,
                            const Vector3 *scale, uint8_t fb_index)
{
	T3DMat4 matrix;

	t3d_mat4_from_srt(
		&matrix,
		(float[3]){ scale->x, scale->y, scale->z },
		(float[4]){ rotation->x, rotation->y, rotation->z, rotation->w },
		(float[3]){ position->x * RENDER_SCALE, position->y * RENDER_SCALE, position->z * RENDER_SCALE }
	);

	t3d_mat4_to_fixed_3x4(&mesh->matrix_buffer[fb_index], &matrix);
	mesh_updateBounds(mesh, &matrix);
}


/* The matrix is built in float because that is what moves the culling bounds
   into the world; the fixed-point one is only for the RSP. */
void mesh_setMatrix(Mesh *mesh, const RenderTransform *transform, uint8_t fb_index)
{
	T3DMat4 matrix;

	t3d_mat4_from_srt_euler(

		&matrix,

		(float[3]){transform->scale.x,         transform->scale.y,         transform->scale.z},
		(float[3]){deg_to_rad(transform->rotation.x), deg_to_rad(transform->rotation.y), deg_to_rad(transform->rotation.z)},
		(float[3]){transform->position.x,      transform->position.y,      transform->position.z}
	);

	t3d_mat4_to_fixed_3x4(&mesh->matrix_buffer[fb_index], &matrix);
	mesh_updateBounds(mesh, &matrix);
}


bool mesh_setDeform(Mesh *mesh, const Vector3 *source, const Vector3 *source_normal,
                    const uint8_t *source_rgba, uint16_t source_count, float scale)
{
	MeshDeform *deform = malloc(sizeof(MeshDeform));
	assert(deform);

	if (!meshDeform_bind(deform, mesh->model, source, source_normal, source_count, scale)) {
		free(deform);
		return false;
	}
	deform->source_rgba = source_rgba;

	/* The binding rewrote the model's vertex addresses as segment references,
	   and the display lists were recorded before that with the old absolute
	   ones. Record them again so the draw goes through the segment. */
	if (mesh->dl_count > 0) {
		for (int i = 0; i < mesh->dl_count; i++) rspq_block_free(mesh->dl[i]);

		rspq_block_begin();
		t3d_model_draw(mesh->model);
		mesh->dl[0]    = rspq_block_end();
		mesh->dl_count = 1;
	} else {
		/* Object path: the blocks live per object instead. */
		T3DModelIter it = t3d_model_iter_create(mesh->model, T3D_CHUNK_TYPE_OBJECT);
		while (t3d_model_iter_next(&it)) {
			if (it.object->userBlock) rspq_block_free(it.object->userBlock);
			rspq_block_begin();
			t3d_model_draw_object(it.object, NULL);
			it.object->userBlock = rspq_block_end();
		}
	}

	mesh->deform = deform;
	return true;
}


void mesh_updateDeform(Mesh *mesh, uint8_t fb_index)
{
	if (mesh->deform) meshDeform_apply(mesh->deform, fb_index);
}

/* Separate from the update because it has to run at draw time, in front of the
   display list, not when the vertices are written. */
void mesh_bindDeformFrame(Mesh *mesh, uint8_t fb_index)
{
	if (mesh->deform) meshDeform_bindFrame(mesh->deform, fb_index);
}


/* Part recording: objects named in the list get their own part (dl),
   every other object lands together in part 0. NULL name = part 0. */
typedef struct {
	const char *const *names;
	uint8_t count;
	const char *name;
} MeshPartFilter;

static bool mesh_filterPart(void *user, const T3DObject *obj)
{
	const MeshPartFilter *filter = user;

	if (filter->name) return obj->name && strcmp(obj->name, filter->name) == 0;

	for (int i = 0; i < filter->count; i++)
		if (obj->name && strcmp(obj->name, filter->names[i]) == 0) return false;
	return true;
}

static rspq_block_t *mesh_recordPart(Mesh *mesh, MeshPartFilter *filter, const T3DMat4FP *matrices)
{
	rspq_block_begin();
	t3d_model_draw_custom(mesh->model, (T3DModelDrawConf){
		.userData = filter,
		.filterCb = mesh_filterPart,
		.matrices = matrices,
	});
	return rspq_block_end();
}

void mesh_recordObjects(Mesh *mesh)
{
	T3DModelIter it = t3d_model_iter_create(mesh->model, T3D_CHUNK_TYPE_OBJECT);
	while (t3d_model_iter_next(&it)) {
		rspq_block_begin();
		t3d_model_draw_object(it.object, NULL);
		it.object->userBlock = rspq_block_end();
	}

	mesh->dl       = NULL;
	mesh->dl_count = 0;
	mesh->visible  = 1;
}

void mesh_recordParts(Mesh *mesh, const char *const *names, uint8_t count, const T3DMat4FP *matrices)
{
	MeshPartFilter filter = { .names = names, .count = count };

	mesh->dl_count = 1 + count;
	mesh->dl = malloc(sizeof(rspq_block_t *) * mesh->dl_count);
	assert(mesh->dl);

	filter.name = NULL;
	mesh->dl[0] = mesh_recordPart(mesh, &filter, matrices);

	for (int i = 0; i < count; i++) {
		filter.name = names[i];
		mesh->dl[1 + i] = mesh_recordPart(mesh, &filter, matrices);
	}

	mesh->visible = 1;
}
