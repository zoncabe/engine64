#include <math.h>
#include <fmath.h>

#include "physics/math/math_common.h"
#include "camera/camera.h"
#include "camera/spring_arm.h"
#include "control/camera_control.h"


/* x and y arrive normalized: how hard the camera is being pushed, whatever
   the game read to get there. */
static void cameraControl_setSpringArmInput(Camera *camera, float x, float y)
{
	CameraSpringArmData *data = &camera->spring_arm.data;
	const CameraSpringArmSettings *settings = &camera->spring_arm.settings;

	data->target_velocity.x = x * settings->max_velocity.x * settings->direction.x;
	data->target_velocity.y = y * settings->max_velocity.y * settings->direction.y;
}


static void (*cameraControl_handler[CAMERA_TYPE_COUNT])(Camera *, float, float) = {
	[CAMERA_TYPE_SPRING_ARM] = cameraControl_setSpringArmInput,
};


static void cameraControl_setInput(Camera *camera, float x, float y)
{
	if (camera->type == CAMERA_TYPE_NONE) return;
	cameraControl_handler[camera->type](camera, x, y);
}


/* Opposite binds cancel out, so a controller that has the C stick under both
   of them hands over the axis with its own magnitude. */
void cameraControl_update(Camera *camera, const Controller *controller, const CameraControlBinding *binding, float dt)
{
	float x = button_getPressed(controller, &controller->held, binding->pan_right)
	        - button_getPressed(controller, &controller->held, binding->pan_left);

	float y = button_getPressed(controller, &controller->held, binding->tilt_up)
	        - button_getPressed(controller, &controller->held, binding->tilt_down);

	cameraControl_setInput(camera, x, y);

	if (camera->type != CAMERA_TYPE_SPRING_ARM) return;

	const CameraSpringArmSettings *settings = &camera->spring_arm.settings;

	float distance = button_getPressed(controller, &controller->held, binding->distance_out)
	               - button_getPressed(controller, &controller->held, binding->distance_in);

	float fov = button_getPressed(controller, &controller->held, binding->fov_out)
	          - button_getPressed(controller, &controller->held, binding->fov_in);

	camera->spring_arm.data.arm_length += distance * settings->distance_speed * dt;
	camera->field_of_view              += fov      * settings->fov_speed      * dt;
}


void cameraControl_setDistance(Camera *camera, float distance, float dt)
{
	if (camera->type != CAMERA_TYPE_SPRING_ARM) return;

	CameraSpringArmData *data = &camera->spring_arm.data;
	float rate = camera->spring_arm.settings.zoom_response_rate;

	data->arm_length = lerpf(data->arm_length, distance, 1.0f - fm_expf(-rate * dt));
}


void cameraControl_setFieldOfView(Camera *camera, float field_of_view, float dt)
{
	if (camera->type != CAMERA_TYPE_SPRING_ARM) return;

	float rate = camera->spring_arm.settings.zoom_response_rate;

	camera->field_of_view = lerpf(camera->field_of_view, field_of_view, 1.0f - fm_expf(-rate * dt));
}
