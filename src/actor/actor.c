#include <t3d/t3danim.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/actor/actor.h"
#include "../../include/actor/actor_states.h"
#include "../../include/player/player.h"
#include "../../include/control/player_control.h"
#include "../../include/player/player_animation.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"


// function implemenations

void actor_init(Actor *actor, ActorMotionSettings* motion_settings, ActorAnimationSettings* animation_settings)
{
    *actor = (Actor){

		.motion.settings = *motion_settings,
		
		.animation.settings = *animation_settings,
		
		.model_scale = {1.0f, 1.0f, 1.0f},
		
		.state.current = STANDING_IDLE,
		
		.body = {
		
			.position = {0.0f, 0.0f, 0.0f},
			.velocity = {0.0f, 0.0f, 0.0f},
			.rotation = {0.0f, 0.0f, 0.0f},
		},

		.motion.data = {

			.target_yaw = 0.0f,

			.horizontal_target_speed = 0.0f,
			.horizontal_speed = 0.0f,

			.roll_timer = 0.0f,

			.jump_initial_velocity = {0.0f, 0.0f, 0.0f},
			.jump_force = 0.0f,
			.jump_timer = 0.0f,

			.grounded = true,
			.grounding_height = 0.0f,
		},

		.animation.data = {
			
			.current = 0,
			.previous = 0,
			
			.locomotion_blending_ratio = 0.0f,
			.jump_blending_ratio = 0.0f,
			.land_blending_ratio = 0.0f,
			.roll_blending_ratio = 0.0f,
			.footing_phase = 0.5f,

			.speed = 0.0f,
		},
	};
}

void actor_draw(Actor* actor, RigidBody* body)
{
	t3d_mat4fp_from_srt_euler(&actor->t3d_matrix[viewport.fb_index],
		(float[3]){actor->model_scale.x, actor->model_scale.y, actor->model_scale.z},
		(float[3]){rad(body->rotation.x), rad(body->rotation.y), rad(body->rotation.z)},
		(float[3]){body->position.x, body->position.y, body->position.z}
	);
    t3d_matrix_push(&actor->t3d_matrix[viewport.fb_index]);
	rspq_block_run(actor->dl);
    t3d_matrix_pop(1);
}
