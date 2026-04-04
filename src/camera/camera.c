#include <t3d/t3d.h>

#include "../../include/physics/physics.h"
#include "../../include/camera/camera.h"


// function implementations

void camera_init(Camera* camera)
{
        camera->distance_from_barycenter = 200;
        camera->target_distance = 300;
        camera->angle_around_barycenter = 20;
        camera->pitch = 8;
        camera->offset_angle = 30;
        camera->offset_height = 130;
		camera->field_of_view = 60;
		camera->near_clipping = 100;
		camera->far_clipping = 4000;
		
        camera->settings.orbitational_acceleration_rate = 10;
        camera->settings.zoom_acceleration_rate = 30;
        camera->settings.zoom_deceleration_rate = 10;
        camera->settings.zoom_max_speed = 150;
        camera->settings.distance_from_baricenter = 200;
        camera->settings.field_of_view = 59;
	    camera->settings.field_of_view_aim = 50;
        camera->settings.offset_acceleration_rate = 25;
        camera->settings.offset_deceleration_rate = 45;
        camera->settings.offset_max_speed = 100;
        camera->settings.offset_angle = 30;
        camera->settings.offset_angle_aim = 20;
        camera->settings.max_pitch = 70;
        
        camera->settings.yaw_sensitivity = 1.8f;
        camera->settings.yaw_direction = 1;
        camera->settings.pitch_sensitivity = 1.6f;
        camera->settings.pitch_direction = -1;
}

