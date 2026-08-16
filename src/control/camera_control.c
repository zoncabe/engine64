#include <math.h>

#include "camera/camera.h"
#include "camera/spring_arm.h"
#include "control/camera_control.h"


static void cameraControl_setSpringArmInput(Camera *camera, const ControllerActions *actions)
{
	CameraSpringArmData *data = &camera->spring_arm.data;
	const CameraSpringArmSettings *settings = &camera->spring_arm.settings;

	if (fabsf(actions->cstick_x) >= CAMERA_STICK_DEADZONE
	 || fabsf(actions->cstick_y) >= CAMERA_STICK_DEADZONE) {
		data->target_velocity.x = actions->cstick_x * settings->max_velocity.x * settings->direction.x;
		data->target_velocity.y = actions->cstick_y * settings->max_velocity.y * settings->direction.y;
	} else {
		data->target_velocity.x = 0;
		data->target_velocity.y = 0;
	}

	camera->spring_arm.state = actions->camera_aim ? CAMERA_SPRING_ARM_AIMING : CAMERA_SPRING_ARM_DEFAULT;
}


static void (*cameraControl_handler[CAMERA_TYPE_COUNT])(Camera *, const ControllerActions *) = {
	[CAMERA_TYPE_SPRING_ARM] = cameraControl_setSpringArmInput,
};


void cameraControl_setInput(Camera *camera, const ControllerActions *actions)
{
	if (camera->type == CAMERA_TYPE_NONE) return;
	cameraControl_handler[camera->type](camera, actions);
}
