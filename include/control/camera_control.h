#ifndef CAMERA_CONTROLS_H
#define CAMERA_CONTROLS_H

#include "controller.h"
#include "../camera/camera.h"

#define CAMERA_STICK_DEADZONE 8

void cameraControl_setOrbitalInput(Camera *camera, const ControllerActions *actions);

#endif
