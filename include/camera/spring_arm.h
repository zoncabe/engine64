#ifndef SPRING_ARM_H
#define SPRING_ARM_H

#include <stdint.h>
#include "physics/math/vector2.h"
#include "physics/math/vector3.h"


typedef struct Camera Camera;

typedef struct CameraSpringArmSettings {

	Vector2 response_rate;
	Vector2 max_velocity;
	Vector2 direction;

	float zoom_response_rate;

	float distance_speed;
	float fov_speed;

	float max_pitch;
	float min_pitch;

} CameraSpringArmSettings;


/* Where the arm is right now: seeded from the def, moved by the engine. */
typedef struct CameraSpringArmData {

	float arm_length;
	float side_offset;

	float yaw;
	float pitch;

	float pivot_height;

	Vector2 velocity;
	Vector2 target_velocity;

} CameraSpringArmData;


typedef struct CameraSpringArmDef {

	float arm_length;
	float side_offset;

	float yaw;
	float pitch;
	float pivot_height;

	CameraSpringArmSettings settings;

} CameraSpringArmDef;


void cameraSpringArm_init(Camera *camera, const CameraSpringArmDef *def);
void cameraSpringArm_update(Camera *camera, Vector3 *center, float dt);

#endif
