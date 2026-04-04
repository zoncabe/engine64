#include <stdint.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"
#include "../../include/camera/camera_states.h"



void camera_setOrbitalSpeed(Camera *camera)
{
    if (camera->state == ORBITAL) {
        if (camera->field_of_view < camera->settings.field_of_view) camera->zoom_acceleration = camera->settings.zoom_acceleration_rate * (camera->settings.zoom_max_speed - camera->zoom_speed);
        else camera->zoom_acceleration = (camera->settings.zoom_deceleration_rate + 10) * (0 - camera->zoom_speed);
        
        camera->zoom_direction = 1;
        
        if (camera->offset_angle > camera->settings.offset_angle) 
            camera->offset_acceleration = camera->settings.offset_acceleration_rate * (camera->settings.offset_max_speed  - camera->offset_speed);
        
        else camera->offset_acceleration = camera->settings.offset_deceleration_rate * (0 - camera->offset_speed);
        
        camera->offset_direction = -1;

        camera->orbitational_acceleration.x = camera->settings.orbitational_acceleration_rate * (camera->orbitational_target_velocity.x - camera->orbitational_velocity.x);
        camera->orbitational_acceleration.y = camera->settings.orbitational_acceleration_rate * (camera->orbitational_target_velocity.y - camera->orbitational_velocity.y);

        camera->orbitational_velocity.x += camera->orbitational_acceleration.x * time_get()->delta;
        camera->orbitational_velocity.y += camera->orbitational_acceleration.y * time_get()->delta;
        camera->zoom_speed += camera->zoom_acceleration * time_get()->delta;
        camera->offset_speed += camera->offset_acceleration * time_get()->delta;
    }
    
    else if (camera->state == AIMING) {

        if (camera->field_of_view > camera->settings.field_of_view_aim) camera->zoom_acceleration = (camera->settings.zoom_acceleration_rate + 10) * (camera->settings.zoom_max_speed - camera->zoom_speed);
        else camera->zoom_acceleration = camera->settings.zoom_deceleration_rate * (0 - camera->zoom_speed);
        
        camera->zoom_direction = -1;

        if (camera->offset_angle < camera->settings.offset_angle_aim) camera->offset_acceleration = camera->settings.offset_acceleration_rate * (camera->settings.offset_max_speed  - camera->offset_speed);
        else camera->offset_acceleration = camera->settings.offset_deceleration_rate * (0 - camera->offset_speed);

        camera->offset_direction = 1;

        camera->orbitational_acceleration.x = camera->settings.orbitational_acceleration_rate * ((camera->orbitational_target_velocity.x / 2) - camera->orbitational_velocity.x);
        camera->orbitational_acceleration.y = camera->settings.orbitational_acceleration_rate * ((camera->orbitational_target_velocity.y / 2) - camera->orbitational_velocity.y);

        camera->orbitational_velocity.x += camera->orbitational_acceleration.x * time_get()->delta;
        camera->orbitational_velocity.y += camera->orbitational_acceleration.y * time_get()->delta;
        camera->zoom_speed += camera->zoom_acceleration * time_get()->delta;
        camera->offset_speed += camera->offset_acceleration * time_get()->delta;
    }

    if (fabs(camera->orbitational_velocity.x) < 1 && fabs(camera->orbitational_velocity.y) < 1 && fabs(camera->zoom_speed) < 1 && fabs(camera->offset_speed) < 1){
        camera->orbitational_velocity.x = 0;
        camera->orbitational_velocity.y = 0;
        camera->zoom_speed = 0;
        camera->offset_speed = 0;
    }
}

void camera_setOrbitalPosition(Camera *camera, Vector3* barycenter)
{
    camera->pitch += camera->orbitational_velocity.y * time_get()->delta;
	camera->angle_around_barycenter += camera->orbitational_velocity.x * time_get()->delta;
	
	camera->field_of_view += camera->zoom_direction * camera->zoom_speed * time_get()->delta;
	camera->offset_angle += camera->offset_direction * camera->offset_speed * time_get()->delta;

	if (camera->angle_around_barycenter > 360) camera->angle_around_barycenter -= 360;
    if (camera->angle_around_barycenter < 0) camera->angle_around_barycenter  += 360;

    if (camera->pitch > camera->settings.max_pitch) camera->pitch = camera->settings.max_pitch;
    if (camera->pitch < -camera->settings.max_pitch + 30) camera->pitch = -camera->settings.max_pitch + 30; 
	
    camera->horizontal_barycenter_distance = camera->distance_from_barycenter * cosf(rad(camera->pitch));
	camera->vertical_barycenter_distance = camera->distance_from_barycenter * sinf(rad(camera->pitch));

	camera-> horizontal_target_distance = camera->target_distance * cosf(rad(camera->pitch));
	camera->vertical_target_distance = camera->target_distance * sinf(rad(camera->pitch + 180));

    camera->position.x = barycenter->x - (camera->horizontal_barycenter_distance * sinf(rad(camera->angle_around_barycenter - camera->offset_angle)));
    camera->position.y = barycenter->y - (camera->horizontal_barycenter_distance * cosf(rad(camera->angle_around_barycenter - camera->offset_angle)));
    camera->position.z = barycenter->z + camera->offset_height + camera->vertical_barycenter_distance;

	camera->target.x = barycenter->x - camera-> horizontal_target_distance * sinf(rad(camera->angle_around_barycenter + 180));
	camera->target.y = barycenter->y - camera-> horizontal_target_distance * cosf(rad(camera->angle_around_barycenter + 180));
	camera->target.z = barycenter->z + camera->offset_height + camera->vertical_target_distance;
}

void camera_setOrbitalMotion(Camera *camera, Vector3* barycenter)
{
    camera_setOrbitalSpeed(camera);
    camera_setOrbitalPosition(camera, barycenter);
}