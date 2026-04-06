
#include "../../include/graphics/mesh.h"
#include "../../include/physics/math/math_common.h"

void mesh_buildMatrix(Mesh *mesh, const Transform *transform, uint8_t fb_index)
{
    t3d_mat4fp_from_srt_euler(&mesh->matrix_buffer[fb_index],
        (float[3]){transform->scale.x,    transform->scale.y,    transform->scale.z},
        (float[3]){rad(transform->rotation.x), rad(transform->rotation.y), rad(transform->rotation.z)},
        (float[3]){transform->position.x,  transform->position.y,  transform->position.z}
    );
}