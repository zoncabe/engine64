/*
	A spring arm behind the body: the arm trails it, the C stick swings it
	around, and the shoulder buttons pull it in and out. What it follows is
	not written here: the control binding names the player, and the camera
	takes that player's body.
*/
#include "camera/e64_camera.h"


const CameraDef camera = {

	.type = CAMERA_TYPE_SPRING_ARM,

	.field_of_view = 60.0f,
	.near_clipping = 100.0f,
	.far_clipping  = 5000.0f,

	.spring_arm = {
		.arm_length   = 500.0f,
		.side_offset  = 0.0f,
		.yaw          = -45.0f,
		.pitch        = 12.0f,
		.pivot_height = 120.0f,

		.settings = {
			.response_rate = { 10.0f, 10.0f },
			.max_velocity  = { 60.0f, 40.0f },
			.direction     = {  1.0f, -1.0f },
			.zoom_response_rate = 6.0f,
			.distance_speed = 400.0f,
			.fov_speed      =  30.0f,
			.max_pitch     =  80.0f,
			.min_pitch     = -50.0f,
		},
	},
};
