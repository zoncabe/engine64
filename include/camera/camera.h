#ifndef CAMERA_H
#define CAMERA_H

#include "physics/math/vector3.h"
#include "spring_arm.h"


typedef enum {

	CAMERA_TYPE_SPRING_ARM,
	CAMERA_TYPE_COUNT,
	CAMERA_TYPE_NONE,

} CameraType;


/* default duration of a view target transition, in seconds */
#define CAMERA_VIEW_TARGET_BLEND_TIME 0.4f


typedef struct {

	CameraType type;
	union {
		CameraSpringArmData spring_arm;
	};

} CameraDef;


typedef struct Camera {

	Vector3 position;
	Vector3 target;

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
			CameraSpringArmState state;
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
Vector3 camera_getRight(const Camera *camera);


#endif
