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
    int deadzone = 8;
    float sensitivity = 1.8f;
    float stick_x = 0;
    float stick_y = 0;

    if (fabs(controller->input.cstick_x) >= deadzone || fabs(controller->input.cstick_y) >= deadzone) {
        stick_x = controller->input.cstick_x;
        stick_y = - controller->input.cstick_y;
    }

    if (stick_x == 0 && stick_y == 0) {
        camera->orbitational_target_velocity.x = 0;
        camera->orbitational_target_velocity.y = 0;
    }
    
    else if (stick_x != 0 || stick_y != 0) {
        camera->orbitational_target_velocity.y = stick_y * sensitivity;
        camera->orbitational_target_velocity.x = stick_x * sensitivity;
    }
}

// rendered obsolete by libdragon's gamecube controller support
// leaving the function because the algorithm is reusable
void cameraControl_orbit_withCButtons(Camera *camera, ControllerData *controller)
{
    float input_x = 0;
    float input_y = 0;

    if (controller->pressed.c_right || controller->pressed.c_left || controller->pressed.c_up || controller->pressed.c_down){
        
        input_x = input(controller->pressed.c_right) - input(controller->pressed.c_left);
        input_y = input(controller->pressed.c_up) - input(controller->pressed.c_down);
    }
    else if (controller->held.c_right || controller->held.c_left || controller->held.c_up || controller->held.c_down){
        
        input_x = input(controller->held.c_right) - input(controller->held.c_left);
        input_y = input(controller->held.c_up) - input(controller->held.c_down);
    }

    if (input_y == 0) camera->orbitational_target_velocity.y = 0; 
    else camera->orbitational_target_velocity.y = input_y * camera->settings.orbitational_max_velocity.y;

	if (input_x == 0) camera->orbitational_target_velocity.x = 0; 
    else camera->orbitational_target_velocity.x = input_x * camera->settings.orbitational_max_velocity.x;

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
