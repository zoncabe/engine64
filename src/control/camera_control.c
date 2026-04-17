#include <math.h>

#include "camera/camera.h"
#include "camera/spherical.h"
#include "control/camera_control.h"


static void cameraControl_setSphericalInput(Camera *camera, const ControllerActions *actions)
{
	CameraSphericalData *data = &camera->spherical.data;
	const CameraSphericalSettings *settings = &camera->spherical.settings;

	if (fabsf(actions->cstick_x) >= CAMERA_STICK_DEADZONE
	 || fabsf(actions->cstick_y) >= CAMERA_STICK_DEADZONE) {
		data->target_velocity.x = actions->cstick_x * settings->max_velocity.x * settings->direction.x;
		data->target_velocity.y = actions->cstick_y * settings->max_velocity.y * settings->direction.y;
	} else {
		data->target_velocity.x = 0;
		data->target_velocity.y = 0;
	}

	camera->spherical.state = actions->camera_aim ? CAMERA_SPHERICAL_AIMING : CAMERA_SPHERICAL_DEFAULT;
}


static void (*cameraControl_handler[CAMERA_TYPE_COUNT])(Camera *, const ControllerActions *) = {
	[CAMERA_TYPE_SPHERICAL] = cameraControl_setSphericalInput,
};


void cameraControl_setInput(Camera *camera, const ControllerActions *actions)
{
	if (camera->type == CAMERA_TYPE_NONE) return;
	cameraControl_handler[camera->type](camera, actions);
}
