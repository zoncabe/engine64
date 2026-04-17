#ifndef SPHERICAL_H
#define SPHERICAL_H

#include <stdint.h>
#include "physics/math/vector2.h"
#include "physics/math/vector3.h"


typedef enum {

	CAMERA_SPHERICAL_DEFAULT,
	CAMERA_SPHERICAL_AIMING,

} CameraSphericalState;

typedef struct Camera Camera;

typedef struct CameraSphericalSettings {

	Vector2 response_rate;
	Vector2 max_velocity;
	Vector2 direction;

	float zoom_response_rate;
	float offset_response_rate;

	float distance_from_center_default;
	float distance_from_center_aim;

	float field_of_view_default;
	float field_of_view_aim;

	float offset_angle_default;
	float offset_angle_aim;

	float max_pitch;

} CameraSphericalSettings;


typedef struct CameraSphericalData {

	float distance_from_center;
	float distance_center_to_target;
	
	float angle_around_center;
	float offset_angle;
	
	float pitch;
	float offset_height;

	Vector2 velocity;
	Vector2 target_velocity;

} CameraSphericalData;


void cameraSpherical_init(Camera *camera, const CameraSphericalData *data);
void cameraSpherical_update(Camera *camera, Vector3 *center, float dt);

#endif
