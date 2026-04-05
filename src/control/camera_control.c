#include <t3d/t3d.h>

#include "../../include/physics/physics.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"
#include "../../include/control/camera_control.h"
#include "../../include/camera/camera_states.h"


static void cameraControl_orbitWithStick(Camera *camera, const ControllerActions *actions)
{
    float stick_x = 0;
    float stick_y = 0;

    if (fabs(actions->cstick_x) >= CAMERA_STICK_DEADZONE || fabs(actions->cstick_y) >= CAMERA_STICK_DEADZONE) {
        stick_x = actions->cstick_x;
        stick_y = actions->cstick_y;
    }

    if (stick_x == 0 && stick_y == 0) {
        camera->orbitational_target_velocity.x = 0;
        camera->orbitational_target_velocity.y = 0;
    } else {
        camera->orbitational_target_velocity.x = stick_x * camera->settings.yaw_sensitivity   * camera->settings.yaw_direction;
        camera->orbitational_target_velocity.y = stick_y * camera->settings.pitch_sensitivity * camera->settings.pitch_direction;
    }
}

static void cameraControl_aim(Camera *camera, const ControllerActions *actions)
{
    if (actions->camera_aim) camera_setState(camera, AIMING);
    else                     camera_setState(camera, ORBITAL);
}


void cameraControl_setOrbitalInput(Camera *camera, const ControllerActions *actions)
{
    cameraControl_orbitWithStick(camera, actions);
    cameraControl_aim(camera, actions);
}
