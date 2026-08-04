
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <t3d/t3dmodel.h>

#include "graphics/mesh.h"
#include "physics/math/math_common.h"


void mesh_setMatrix(Mesh *mesh, const RenderTransform *transform, uint8_t fb_index)
{
	t3d_mat4fp_from_srt_euler(

		&mesh->matrix_buffer[fb_index],

		(float[3]){transform->scale.x,         transform->scale.y,         transform->scale.z},
		(float[3]){deg_to_rad(transform->rotation.x), deg_to_rad(transform->rotation.y), deg_to_rad(transform->rotation.z)},
		(float[3]){transform->position.x,      transform->position.y,      transform->position.z}
	);
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
