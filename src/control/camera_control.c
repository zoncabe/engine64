#include <t3d/t3d.h>

#include "../../include/control/control.h"
#include "../../include/physics/physics.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"
#include "../../include/control/camera_control.h"
#include "../../include/camera/camera_states.h"


/* input
 auxiliary function for 8 directional movement*/

int input(int input){
    if (input == 0) {return 0;}
    else {return 1;}
}


/* camera_move_stick
changes the camera variables depending on controller input*/

void cameraControl_orbitWithStick(Camera *camera, ControllerData *controller)
{
    float stick_x = 0;
    float stick_y = 0;

    if (fabs(controller->input.cstick_x) >= CAMERA_STICK_DEADZONE || fabs(controller->input.cstick_y) >= CAMERA_STICK_DEADZONE) {
        stick_x = controller->input.cstick_x;
        stick_y = controller->input.cstick_y;
    }

    if (stick_x == 0 && stick_y == 0) {
        camera->orbitational_target_velocity.x = 0;
        camera->orbitational_target_velocity.y = 0;
    }
    
    else if (stick_x != 0 || stick_y != 0) {
        camera->orbitational_target_velocity.x = stick_x * camera->settings.yaw_sensitivity * camera->settings.yaw_direction;
        camera->orbitational_target_velocity.y = stick_y * camera->settings.pitch_sensitivity * camera->settings.pitch_direction;
    }
}

void cameraControl_aim(Camera *camera, ControllerData *controller)
{
    if (controller->held.z) camera_setState (camera, AIMING);
    else camera_setState (camera, ORBITAL);
}


void cameraControl_setOrbitalInput(Camera *camera, ControllerData *controller)
{
    cameraControl_orbitWithStick(camera, controller);
    cameraControl_aim(camera, controller);
}
