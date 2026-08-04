#include <math.h>
#include <fmath.h>
#include <stdbool.h>

#include "physics/math/math_common.h"
#include "camera/camera.h"
#include "camera/spherical.h"


static const CameraSphericalSettings default_spherical_settings = {

	.response_rate = { .x = 10.0f, .y = 10.0f },
	.max_velocity  = { .x = 1.8f,  .y = 1.6f  },
	.direction     = { .x = 1.0f,  .y = -1.0f },

	.zoom_response_rate    = 6.0f,
	.offset_response_rate  = 6.0f,

	.distance_from_center_default = 200.0f,
	.distance_from_center_aim     = 260.0f,

	.field_of_view_default = 60.0f,
	.field_of_view_aim     = 38.0f,

	.offset_angle_default = 30.0f,
	.offset_angle_aim     = 34.0f,

	.max_pitch = 70.0f,
};


static void cameraSpherical_setVelocity(Camera *camera, float dt)
{
	CameraSphericalData *data = &camera->spherical.data;
	const CameraSphericalSettings *settings = &camera->spherical.settings;

	bool aiming             = (camera->spherical.state == CAMERA_SPHERICAL_AIMING);
	float velocity_scale    = aiming ? 0.5f : 1.0f;
	float target_fov        = aiming ? settings->field_of_view_aim        : settings->field_of_view_default;
	float target_offset     = aiming ? settings->offset_angle_aim         : settings->offset_angle_default;
	float target_distance   = aiming ? settings->distance_from_center_aim : settings->distance_from_center_default;

	float factor_x = fm_expf(-settings->response_rate.x * dt);
	float factor_y = fm_expf(-settings->response_rate.y * dt);
	data->velocity.x = data->velocity.x * factor_x + (data->target_velocity.x * velocity_scale) * (1.0f - factor_x);
	data->velocity.y = data->velocity.y * factor_y + (data->target_velocity.y * velocity_scale) * (1.0f - factor_y);

	camera->field_of_view      = lerpf(camera->field_of_view,      target_fov,      1.0f - fm_expf(-settings->zoom_response_rate   * dt));
	data->offset_angle         = lerpf(data->offset_angle,         target_offset,   1.0f - fm_expf(-settings->offset_response_rate * dt));
	data->distance_from_center = lerpf(data->distance_from_center, target_distance, 1.0f - fm_expf(-settings->zoom_response_rate   * dt));
}


static void cameraSpherical_setPosition(Camera *camera, Vector3 *center, float dt)
{
	CameraSphericalData *data = &camera->spherical.data;
	const CameraSphericalSettings *settings = &camera->spherical.settings;

	data->pitch               += data->velocity.y * dt;
	data->angle_around_center += data->velocity.x * dt;

	data->angle_around_center = angle_wrap(data->angle_around_center);

	if (data->pitch >  settings->max_pitch)      data->pitch =  settings->max_pitch;
	if (data->pitch < -settings->max_pitch + 30) data->pitch = -settings->max_pitch + 30;

	float horizontal_center_distance = data->distance_from_center * fm_cosf(deg_to_rad(data->pitch));
	float vertical_center_distance   = data->distance_from_center * fm_sinf(deg_to_rad(data->pitch));
	float horizontal_target_distance = data->distance_center_to_target * fm_cosf(deg_to_rad(data->pitch));
	float vertical_target_distance   = data->distance_center_to_target * fm_sinf(deg_to_rad(data->pitch + 180));

	float half_offset = data->offset_angle * 0.5f;

	camera->position.x = center->x - horizontal_center_distance * fm_sinf(deg_to_rad(data->angle_around_center - half_offset));
	camera->position.y = center->y - horizontal_center_distance * fm_cosf(deg_to_rad(data->angle_around_center - half_offset));
	camera->position.z = center->z + data->offset_height + vertical_center_distance;

	camera->target.x = center->x - horizontal_target_distance * fm_sinf(deg_to_rad(data->angle_around_center + 180 + half_offset));
	camera->target.y = center->y - horizontal_target_distance * fm_cosf(deg_to_rad(data->angle_around_center + 180 + half_offset));
	camera->target.z = center->z + data->offset_height + vertical_target_distance;
}


void cameraSpherical_init(Camera *camera, const CameraSphericalData *data)
{
	camera->type = CAMERA_TYPE_SPHERICAL;
	camera->spherical.state    = CAMERA_SPHERICAL_DEFAULT;
	camera->spherical.settings = default_spherical_settings;
	camera->spherical.data     = *data;
}


void cameraSpherical_update(Camera *camera, Vector3 *center, float dt)
{
	cameraSpherical_setVelocity(camera, dt);
	cameraSpherical_setPosition(camera, center, dt);
}
