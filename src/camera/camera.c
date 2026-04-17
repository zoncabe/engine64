#include "camera/camera.h"
#include "camera/spherical.h"


static void (*camera_handler[CAMERA_TYPE_COUNT])(Camera *, Vector3 *, float) = {
	[CAMERA_TYPE_SPHERICAL] = cameraSpherical_update,
};

void camera_init(Camera *camera)
{
	*camera = (Camera){
		.position      = { 0.0f, 1.0f, 0.0f },
		.target        = { 0.0f, 0.0f, 0.0f },
		.field_of_view = 60.0f,
		.near_clipping = 100.0f,
		.far_clipping  = 4000.0f,
		.type          = CAMERA_TYPE_NONE,
	};
}

void camera_reset(Camera *camera)
{
	camera_init(camera);
}

void camera_update(Camera *camera, Vector3 *center, float dt)
{
	if (camera->type == CAMERA_TYPE_NONE) return;
	camera_handler[camera->type](camera, center, dt);
}
