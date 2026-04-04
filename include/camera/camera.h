#ifndef CAMERA_H
#define CAMERA_H

#include <t3d/t3d.h>
#include "../physics/math/vector2.h"
#include "../physics/math/vector3.h"


// structures

typedef enum {
    ORBITAL,
	AIMING,

} CameraState;

typedef struct {

	float orbitational_acceleration_rate;

	float yaw_sensitivity;
	int8_t yaw_direction;

	float pitch_sensitivity;
	int8_t pitch_direction;

	float zoom_acceleration_rate;
	float zoom_deceleration_rate;
	float zoom_max_speed;

	float distance_from_baricenter;
	
    float field_of_view;
	float field_of_view_aim;

	float offset_acceleration_rate;
	float offset_deceleration_rate;
	float offset_max_speed;

	float offset_angle;
	float offset_angle_aim;
	
	float max_pitch;

} CameraSettings;


typedef struct {

	T3DViewport viewport;

	CameraState state;
	CameraSettings settings;

	Vector3 position;
	Vector3 target;

	float field_of_view;
	float near_clipping;
	float far_clipping;

	// orbital camera parameters
	float offset_height;
	
	float distance_from_barycenter; // distance from the camera position to the barycenter
	float target_distance; // distance from the barycenter to the target position
	float angle_around_barycenter;
	float pitch;
	
	float horizontal_barycenter_distance;
	float vertical_barycenter_distance;
	
	float horizontal_target_distance;
	float vertical_target_distance;
	
	float offset_angle;

	// orbital camera physics
	Vector2 orbitational_target_velocity; // target as in intended velocity
	Vector2 orbitational_acceleration;
	Vector2 orbitational_velocity;

	float offset_acceleration;
	float offset_speed;
	int offset_direction;

	float zoom_acceleration;
	float zoom_speed;
	int zoom_direction;

} Camera;


// functions prototypes


void camera_init(Camera* camera);


#endif
