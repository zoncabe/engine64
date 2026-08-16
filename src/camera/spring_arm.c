#include <math.h>
#include <fmath.h>
#include <stdbool.h>

#include "physics/math/math_common.h"
#include "camera/camera.h"
#include "camera/spring_arm.h"


static const CameraSpringArmSettings default_spring_arm_settings = {

	.response_rate = { .x = 10.0f, .y = 10.0f },
	.max_velocity  = { .x = 1.8f,  .y = 1.6f  },
	.direction     = { .x = 1.0f,  .y = -1.0f },

	.zoom_response_rate    = 6.0f,
	.offset_response_rate  = 6.0f,

	.arm_length_default = 200.0f,
	.arm_length_aim     = 260.0f,

	.field_of_view_default = 60.0f,
	.field_of_view_aim     = 38.0f,

	.shoulder_offset_default = 50.0f,
	.shoulder_offset_aim     = 60.0f,

	.max_pitch = 80.0f,
};


static void cameraSpringArm_setVelocity(Camera *camera, float dt)
{
	CameraSpringArmData *data = &camera->spring_arm.data;
	const CameraSpringArmSettings *settings = &camera->spring_arm.settings;

	bool aiming           = (camera->spring_arm.state == CAMERA_SPRING_ARM_AIMING);
	float velocity_scale  = aiming ? 0.5f : 1.0f;
	float target_fov      = aiming ? settings->field_of_view_aim     : settings->field_of_view_default;
	float target_shoulder = aiming ? settings->shoulder_offset_aim   : settings->shoulder_offset_default;
	float target_length   = aiming ? settings->arm_length_aim        : settings->arm_length_default;

	float factor_x = fm_expf(-settings->response_rate.x * dt);
	float factor_y = fm_expf(-settings->response_rate.y * dt);
	data->velocity.x = data->velocity.x * factor_x + (data->target_velocity.x * velocity_scale) * (1.0f - factor_x);
	data->velocity.y = data->velocity.y * factor_y + (data->target_velocity.y * velocity_scale) * (1.0f - factor_y);

	camera->field_of_view = lerpf(camera->field_of_view, target_fov,      1.0f - fm_expf(-settings->zoom_response_rate   * dt));
	data->shoulder_offset = lerpf(data->shoulder_offset, target_shoulder, 1.0f - fm_expf(-settings->offset_response_rate * dt));
	data->arm_length      = lerpf(data->arm_length,      target_length,   1.0f - fm_expf(-settings->zoom_response_rate   * dt));
}


static void cameraSpringArm_setPosition(Camera *camera, Vector3 *center, float dt)
{
	CameraSpringArmData *data = &camera->spring_arm.data;
	const CameraSpringArmSettings *settings = &camera->spring_arm.settings;

	data->pitch += data->velocity.y * dt;
	data->yaw   += data->velocity.x * dt;

	data->yaw = angle_wrap(data->yaw);

	if (data->pitch >  settings->max_pitch)      data->pitch =  settings->max_pitch;
	if (data->pitch < -settings->max_pitch + 30) data->pitch = -settings->max_pitch + 30;

	float yaw   = deg_to_rad(data->yaw);
	float pitch = deg_to_rad(data->pitch);

	float sin_yaw   = fm_sinf(yaw),   cos_yaw   = fm_cosf(yaw);
	float sin_pitch = fm_sinf(pitch), cos_pitch = fm_cosf(pitch);

	/* forward points from the camera toward the pivot; right is its horizontal perpendicular */
	Vector3 forward = { cos_pitch * sin_yaw, cos_pitch * cos_yaw, -sin_pitch };
	Vector3 right   = { cos_yaw, -sin_yaw, 0.0f };

	Vector3 pivot = { center->x, center->y, center->z + data->pivot_height };

	camera->position.x = pivot.x - forward.x * data->arm_length + right.x * data->shoulder_offset;
	camera->position.y = pivot.y - forward.y * data->arm_length + right.y * data->shoulder_offset;
	camera->position.z = pivot.z - forward.z * data->arm_length;

	camera->target.x = pivot.x + right.x * data->shoulder_offset;
	camera->target.y = pivot.y + right.y * data->shoulder_offset;
	camera->target.z = pivot.z;
}


void cameraSpringArm_init(Camera *camera, const CameraSpringArmData *data)
{
	camera->type = CAMERA_TYPE_SPRING_ARM;
	camera->spring_arm.state    = CAMERA_SPRING_ARM_DEFAULT;
	camera->spring_arm.settings = default_spring_arm_settings;
	camera->spring_arm.data     = *data;
}


void cameraSpringArm_update(Camera *camera, Vector3 *center, float dt)
{
	cameraSpringArm_setVelocity(camera, dt);
	cameraSpringArm_setPosition(camera, center, dt);
}
