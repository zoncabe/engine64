#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include "controller.h"
#include "camera/camera.h"

typedef struct CameraControlBinding {

	ButtonID pan_left;
	ButtonID pan_right;
	ButtonID tilt_up;
	ButtonID tilt_down;

	ButtonID distance_in;
	ButtonID distance_out;
	ButtonID fov_in;
	ButtonID fov_out;

} CameraControlBinding;


void cameraControl_update(Camera *camera, const Controller *controller, const CameraControlBinding *binding, float dt);
void cameraControl_setDistance(Camera *camera, float distance, float dt);
void cameraControl_setFieldOfView(Camera *camera, float field_of_view, float dt);

#endif
