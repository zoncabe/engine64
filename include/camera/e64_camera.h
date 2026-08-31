#ifndef ENGINE64_CAMERA_H
#define ENGINE64_CAMERA_H

#include "physics/math/e64_vector3.h"
#include "e64_spring_arm.h"


typedef enum {

	CAMERA_TYPE_SPRING_ARM,
	CAMERA_TYPE_COUNT,
	CAMERA_TYPE_NONE,

} CameraType;


typedef struct {

	CameraType type;

	float field_of_view;
	float near_clipping;
	float far_clipping;

	union {
		CameraSpringArmDef spring_arm;
	};

} CameraDef;


typedef struct Camera {

	Vector3 position;
	Vector3 target;

	/* Same split as the arm: the stick writes the target, the aim adds its
	   offset, and the lens chases the sum. */
	float target_field_of_view;
	float field_of_view;
	float near_clipping;
	float far_clipping;

	/* view target transition: the outgoing center is frozen at switch time, so
	   the old target moving afterwards cannot disturb the blend */
	Vector3 blend_from;
	float   blend_elapsed;
	float   blend_duration;

	CameraType  type;

	union {
		struct {
			CameraSpringArmSettings settings;
			CameraSpringArmData     data;
		} spring_arm;
	};

} Camera;


void camera_init(Camera *camera);
void camera_reset(Camera *camera);
void camera_update(Camera *camera, Vector3 *center, float dt);
void camera_setViewTarget(Camera *camera, const Vector3 *from, float duration);
float camera_getAngleAround(const Camera *camera, const Vector3 *point);
float camera_getPitch(const Camera *camera);
Vector3 camera_getRight(const Camera *camera);


#endif
