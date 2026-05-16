
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