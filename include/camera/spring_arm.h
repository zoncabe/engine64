#ifndef SPRING_ARM_H
#define SPRING_ARM_H

#include <stdint.h>
#include "physics/math/vector2.h"
#include "physics/math/vector3.h"


typedef enum {

	CAMERA_SPRING_ARM_DEFAULT,
	CAMERA_SPRING_ARM_AIMING,

} CameraSpringArmState;

typedef struct Camera Camera;

typedef struct CameraSpringArmSettings {

	Vector2 response_rate;
	Vector2 max_velocity;
	Vector2 direction;

	float zoom_response_rate;
	float offset_response_rate;

	float arm_length_default;
	float arm_length_aim;

	float field_of_view_default;
	float field_of_view_aim;

	float shoulder_offset_default;
	float shoulder_offset_aim;

	float max_pitch;

} CameraSpringArmSettings;


typedef struct CameraSpringArmData {

	float arm_length;
	float shoulder_offset;

	float yaw;
	float pitch;

	float pivot_height;

	Vector2 velocity;
	Vector2 target_velocity;

} CameraSpringArmData;


void cameraSpringArm_init(Camera *camera, const CameraSpringArmData *data);
void cameraSpringArm_update(Camera *camera, Vector3 *center, float dt);

#endif
