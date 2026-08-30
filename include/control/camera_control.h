#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include "controller.h"
#include "camera/camera.h"

typedef struct CameraControlBinding {

	PlayerID player;

	ButtonID pan_left;
	ButtonID pan_right;
	ButtonID tilt_up;
	ButtonID tilt_down;

	ButtonID distance_in;
	ButtonID distance_out;
	ButtonID fov_in;
	ButtonID fov_out;

} CameraControlBinding;


/* Reads the pad of the player the binding names. */
void cameraControl_update(Camera *camera, const CameraControlBinding *binding, float dt);
void cameraControl_setDistance(Camera *camera, float distance, float dt);
void cameraControl_setFieldOfView(Camera *camera, float field_of_view, float dt);

#endif
