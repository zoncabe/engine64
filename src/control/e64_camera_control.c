#include <math.h>
#include <fmath.h>

#include "physics/math/e64_math_common.h"
#include "camera/e64_camera.h"
#include "camera/e64_spring_arm.h"
#include "control/e64_camera_control.h"
#include "player/e64_player.h"


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

/* The aiming pose: in over the shoulder, narrower view, slower swing. It only
   ever adds its offsets to the targets, so what the player set is still there
   when the aim lets go. Runs at the tail of the update, after the control
   wrote the velocity this scales. */
static void cameraControl_setAiming(Camera *camera, bool aiming, float dt)
{
	if (camera->type != CAMERA_TYPE_SPRING_ARM) return;

	const CameraSpringArmSettings *settings = &camera->spring_arm.settings;
	CameraSpringArmData *data = &camera->spring_arm.data;

	float arm  = data->target_arm_length;
	float fov  = camera->target_field_of_view;
	float side = data->target_side_offset;

	if (aiming) {
		arm  += settings->aim_arm_length;
		fov  += settings->aim_field_of_view;
		side += settings->aim_side_offset;
	}

	cameraControl_setDistance(camera, arm, dt);
	cameraControl_setFieldOfView(camera, fov, dt);
	cameraControl_setSideOffset(camera, side, dt);

	/* The swing was already written this frame: aiming asks for a share of
	   what the stick pushed, not for a different push. */
	if (aiming) {
		data->velocity.x *= settings->aim_velocity_scale;
		data->velocity.y *= settings->aim_velocity_scale;
	}
}


/* Opposite binds cancel out, so a controller that has the C stick under both
   of them hands over the axis with its own magnitude. */
void cameraControl_update(Camera *camera, const CameraControlBinding *binding, float dt)
{
	const Controller *controller = &controller_get()[binding->player];

	float x = button_getPressed(controller, &controller->held, binding->pan_right)
	        - button_getPressed(controller, &controller->held, binding->pan_left);

	float y = button_getPressed(controller, &controller->held, binding->tilt_up)
	        - button_getPressed(controller, &controller->held, binding->tilt_down);

	cameraControl_setInput(camera, x, y);

	if (camera->type == CAMERA_TYPE_SPRING_ARM) {
		const CameraSpringArmSettings *settings = &camera->spring_arm.settings;

		float distance = button_getPressed(controller, &controller->held, binding->distance_out)
		               - button_getPressed(controller, &controller->held, binding->distance_in);

		float fov = button_getPressed(controller, &controller->held, binding->fov_out)
		          - button_getPressed(controller, &controller->held, binding->fov_in);

		/* The stick moves what the arm is asked for, never where it is: the
		   aim rides on top of this and the two never fight over one field. */
		camera->spring_arm.data.target_arm_length += distance * settings->distance_speed * dt;
		camera->target_field_of_view              += fov      * settings->fov_speed      * dt;
	}

	/* The binding names the player, so the camera knows what to follow on its
	   own. A seat with nobody in it leaves the camera where it was: a game
	   that frames something else calls viewport_updateCamera instead. */
	const Player *player = &player_get()[binding->player];

	/* The body it follows is also the body that aims, so the pose needs no
	   call of its own. */
	cameraControl_setAiming(camera,
		player->character && player->character->movement.data.aiming, dt);

	if (player->entity)
		camera_update(camera, &player->entity->transform.position, dt);
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


void cameraControl_setSideOffset(Camera *camera, float side_offset, float dt)
{
	if (camera->type != CAMERA_TYPE_SPRING_ARM) return;

	CameraSpringArmData *data = &camera->spring_arm.data;
	float rate = camera->spring_arm.settings.zoom_response_rate;

	data->side_offset = lerpf(data->side_offset, side_offset, 1.0f - fm_expf(-rate * dt));
}


