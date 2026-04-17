#ifndef CAMERA_H
#define CAMERA_H

#include "physics/math/vector3.h"
#include "spherical.h"


typedef enum {

	CAMERA_TYPE_SPHERICAL,
	CAMERA_TYPE_COUNT,
	CAMERA_TYPE_NONE,
	
} CameraType;


typedef struct Camera {

	Vector3 position;
	Vector3 target;

	float field_of_view;
	float near_clipping;
	float far_clipping;

	CameraType  type;

	union {
		struct {
			CameraSphericalState state;
			CameraSphericalSettings settings;
			CameraSphericalData     data;
		} spherical;
	};

} Camera;


void camera_init(Camera *camera);
void camera_reset(Camera *camera);
void camera_update(Camera *camera, Vector3 *center, float dt);


#endif
