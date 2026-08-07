
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <t3d/t3dmodel.h>

#include "graphics/mesh.h"
#include "shaders/mesh_deform.h"
#include "physics/math/math_common.h"
#include "physics/math/quaternion.h"


/* A simulated body tumbles, and euler angles cannot describe that without
   picking an order and losing the tumble at the poles. The body already keeps
   a quaternion, so it goes straight to the matrix. */
void mesh_setMatrixFromBody(Mesh *mesh, const Vector3 *position, const Quaternion *rotation,
                            const Vector3 *scale, uint8_t fb_index)
{
	t3d_mat4fp_from_srt(
		&mesh->matrix_buffer[fb_index],
		(float[3]){ scale->x, scale->y, scale->z },
		(float[4]){ rotation->x, rotation->y, rotation->z, rotation->w },
		(float[3]){ position->x * RENDER_SCALE, position->y * RENDER_SCALE, position->z * RENDER_SCALE }
	);
}


void mesh_setMatrix(Mesh *mesh, const RenderTransform *transform, uint8_t fb_index)
{
	t3d_mat4fp_from_srt_euler(

		&mesh->matrix_buffer[fb_index],

		(float[3]){transform->scale.x,         transform->scale.y,         transform->scale.z},
		(float[3]){deg_to_rad(transform->rotation.x), deg_to_rad(transform->rotation.y), deg_to_rad(transform->rotation.z)},
		(float[3]){transform->position.x,      transform->position.y,      transform->position.z}
	);
}


bool mesh_setDeform(Mesh *mesh, const Vector3 *source, const Vector3 *source_normal,
                    uint16_t source_count, float scale)
{
	MeshDeform *deform = malloc(sizeof(MeshDeform));
	assert(deform);

	if (!meshDeform_bind(deform, mesh->model, source, source_normal, source_count, scale)) {
		free(deform);
		return false;
	}

	/* The binding rewrote the model's vertex addresses as segment references,
	   and the display list was recorded before that with the old absolute
	   ones. Record it again so the draw goes through the segment. */
	for (int i = 0; i < mesh->dl_count; i++) rspq_block_free(mesh->dl[i]);

	rspq_block_begin();
	t3d_model_draw(mesh->model);
	mesh->dl[0]    = rspq_block_end();
	mesh->dl_count = 1;

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
