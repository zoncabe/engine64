#include <fmath.h>

#include "physics/math/math_common.h"
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
		.near_clipping = 50.0f,
		.far_clipping  = 5000.0f,
		.type          = CAMERA_TYPE_NONE,
	};
}

void camera_reset(Camera *camera)
{
	camera_init(camera);
}

/* horizontal angle of the camera around a point, in the yaw convention of the engine */
float camera_getAngleAround(const Camera *camera, const Vector3 *point)
{
	float dx = camera->position.x - point->x;
	float dy = camera->position.y - point->y;

	if (dx == 0.0f && dy == 0.0f) return 0.0f;

	return rad_to_deg(fm_atan2f(-dx, -dy));
}

void camera_update(Camera *camera, Vector3 *center, float dt)
{
	if (camera->type == CAMERA_TYPE_NONE) return;
	camera_handler[camera->type](camera, center, dt);
}
