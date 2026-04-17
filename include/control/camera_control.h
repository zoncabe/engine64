#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include "controller.h"
#include "camera/camera.h"

#define CAMERA_STICK_DEADZONE 8

void cameraControl_setInput(Camera *camera, const ControllerActions *actions);

#endif
