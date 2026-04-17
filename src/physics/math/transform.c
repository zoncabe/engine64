#include "../../../include/physics/math/transform.h"

void transform_init(Transform *transform)
{
	*transform = (Transform){
		.position = {0.0f, 0.0f, 0.0f},
		.rotation = {0.0f, 0.0f, 0.0f},
		.scale    = {1.0f, 1.0f, 1.0f},
	};
}
