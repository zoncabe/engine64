#ifndef ENGINE64_SPRING_ARM_H
#define ENGINE64_SPRING_ARM_H

#include <stdint.h>
#include "physics/math/e64_vector2.h"
#include "physics/math/e64_vector3.h"


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

	/* What the aim adds to whatever the player has the arm set to, so the free
	   distance and lens are never overwritten: negative pulls the arm in and
	   narrows the view. The swing is the one that scales instead of adding. */
	float aim_arm_length;
	float aim_side_offset;
	float aim_field_of_view;
	float aim_velocity_scale;

} CameraSpringArmSettings;


/* Where the arm is right now: seeded from the def, moved by the engine. */
typedef struct CameraSpringArmData {

	/* What the arm is asked for and where it actually is. The stick writes the
	   target, the aim adds its offset on top, and the arm chases the sum: the
	   free distance survives an aim because nothing ever writes over it. */
	float target_arm_length;
	float arm_length;

	float target_side_offset;
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

/* The arm's two control-driven angles, in degrees. Both answer zero on a
   camera that is not an arm, which is what a caller that needs them reads as
   nothing to do. */
float cameraSpringArm_getPitch(const Camera *camera);
float cameraSpringArm_getYaw(const Camera *camera);

#endif
