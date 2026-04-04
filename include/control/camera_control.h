#ifndef CAMERA_CONTROLS_H
#define CAMERA_CONTROLS_H


#define CAMERA_STICK_DEADZONE 8


int input(int input);

void cameraControl_orbit_withStick(Camera *camera, ControllerData *data);
void cameraControl_orbit_withCButtons(Camera *camera, ControllerData *data);
void cameraControl_aim(Camera *camera, ControllerData *data);

void cameraControl_setOrbitalInput(Camera *camera, ControllerData *data);


#endif