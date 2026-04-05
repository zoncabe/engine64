#include "../../include/time/time.h"
#include "../../include/actor/actor_states.h"
#include "../../include/actor/actor_animation.h"
#include "../../include/viewport/viewport.h"

void actorAnimation_addLayer(ActorAnimationBuffer *buffer, const T3DSkeleton *skel, float weight)
{
	buffer->layer[buffer->count] = skel;
	buffer->weight[buffer->count] = weight;
	buffer->count++;
}

void actorAnimation_blendLayers(const T3DSkeleton *main, const ActorAnimationBuffer *buffer)
{
	for (int i = 0; i < main->skeletonRef->boneCount; i++)
	{
		T3DBone *bone = &main->bones[i];
		bone->hasChanged = true;

		for (int j = 0; j < buffer->count; j++)
		{
			T3DBone *layer = &buffer->layer[j]->bones[i];
			t3d_quat_nlerp(&bone->rotation, &bone->rotation, &layer->rotation, buffer->weight[j]);
			t3d_vec3_lerp(&bone->position, &bone->position, &layer->position, buffer->weight[j]);
			t3d_vec3_lerp(&bone->scale, &bone->scale, &layer->scale, buffer->weight[j]);
		}
	}
}

void actorAnimation_initArmature(Entity *actor)
{
	actor->actor->armature.main = t3d_skeleton_create_buffered(actor->mesh->model, FB_COUNT);

	for (int i = 0; i < ANIM_SLOT_COUNT; i++)
		actor->actor->armature.buffer[i] = t3d_skeleton_clone(&actor->actor->armature.main, false);
}

void actorAnimation_initStandingLocomotionSet(Entity *actor)
{
	actor->actor->animation.standing_locomotion.standing_idle_left = t3d_anim_create(actor->mesh->model, "standing-idle-left");
	actor->actor->animation.standing_locomotion.standing_idle_right = t3d_anim_create(actor->mesh->model, "standing-idle-right");

	actor->actor->animation.standing_locomotion.walking = t3d_anim_create(actor->mesh->model, "walking");
	actor->actor->animation.standing_locomotion.running = t3d_anim_create(actor->mesh->model, "running");
	actor->actor->animation.standing_locomotion.sprinting = t3d_anim_create(actor->mesh->model, "sprinting");

	actor->actor->animation.standing_locomotion.walking_turn_left = t3d_anim_create(actor->mesh->model, "walking-turn-left");
	actor->actor->animation.standing_locomotion.walking_turn_right = t3d_anim_create(actor->mesh->model, "walking-turn-right");

	actor->actor->animation.standing_locomotion.running_turn_left = t3d_anim_create(actor->mesh->model, "running-turn-left");
	actor->actor->animation.standing_locomotion.running_turn_right = t3d_anim_create(actor->mesh->model, "running-turn-right");

	t3d_anim_attach(&actor->actor->animation.standing_locomotion.standing_idle_left, &actor->actor->armature.main);
	t3d_anim_attach(&actor->actor->animation.standing_locomotion.standing_idle_right, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R]);

	t3d_anim_attach(&actor->actor->animation.standing_locomotion.walking, &actor->actor->armature.buffer[ANIM_SLOT_WALK]);
	t3d_anim_attach(&actor->actor->animation.standing_locomotion.running, &actor->actor->armature.main);
	t3d_anim_attach(&actor->actor->animation.standing_locomotion.sprinting, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R]);
	
	t3d_anim_attach(&actor->actor->animation.standing_locomotion.walking_turn_left, &actor->actor->armature.buffer[ANIM_SLOT_TURN_WALK]);
	t3d_anim_attach(&actor->actor->animation.standing_locomotion.walking_turn_right, &actor->actor->armature.buffer[ANIM_SLOT_TURN_WALK]);
	t3d_anim_attach(&actor->actor->animation.standing_locomotion.running_turn_left, &actor->actor->armature.buffer[ANIM_SLOT_TURN_RUN]);
	t3d_anim_attach(&actor->actor->animation.standing_locomotion.running_turn_right, &actor->actor->armature.buffer[ANIM_SLOT_TURN_RUN]);
}

void actorAnimation_initJumpSet(Entity *actor)
{
	actor->actor->animation.jump.jump_left = t3d_anim_create(actor->mesh->model, "jump-left");
	actor->actor->animation.jump.jump_right = t3d_anim_create(actor->mesh->model, "jump-right");

	actor->actor->animation.jump.falling_left = t3d_anim_create(actor->mesh->model, "falling-idle-left");
	actor->actor->animation.jump.falling_right = t3d_anim_create(actor->mesh->model, "falling-idle-right");

	actor->actor->animation.jump.land_left = t3d_anim_create(actor->mesh->model, "land-left");
	actor->actor->animation.jump.land_right = t3d_anim_create(actor->mesh->model, "land-right");

	t3d_anim_set_looping(&actor->actor->animation.jump.jump_left, false);
	t3d_anim_set_looping(&actor->actor->animation.jump.jump_right, false);

	t3d_anim_set_looping(&actor->actor->animation.jump.land_left, false);
	t3d_anim_set_looping(&actor->actor->animation.jump.land_right, false);

	t3d_anim_attach(&actor->actor->animation.jump.jump_left, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L]);
	t3d_anim_attach(&actor->actor->animation.jump.jump_right, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R]);

	t3d_anim_attach(&actor->actor->animation.jump.falling_left, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L]);
	t3d_anim_attach(&actor->actor->animation.jump.falling_right, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R]);

	t3d_anim_attach(&actor->actor->animation.jump.land_left, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L]);
	t3d_anim_attach(&actor->actor->animation.jump.land_right, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R]);
	
	t3d_anim_set_playing(&actor->actor->animation.jump.jump_left, false);
	t3d_anim_set_playing(&actor->actor->animation.jump.jump_right, false);

	t3d_anim_set_playing(&actor->actor->animation.jump.land_left, false);
	t3d_anim_set_playing(&actor->actor->animation.jump.land_right, false);
}

void actorAnimation_initRollSet(Entity *actor)
{
	actor->actor->animation.roll.running_to_roll_right = t3d_anim_create(actor->mesh->model, "running-to-roll-right");
	actor->actor->animation.roll.running_to_roll_left = t3d_anim_create(actor->mesh->model, "running-to-roll-left");

	t3d_anim_set_looping(&actor->actor->animation.roll.running_to_roll_left, false);
	t3d_anim_set_looping(&actor->actor->animation.roll.running_to_roll_right, false);

	t3d_anim_attach(&actor->actor->animation.roll.running_to_roll_left, &actor->actor->armature.buffer[ANIM_SLOT_ROLL_L]);
	t3d_anim_attach(&actor->actor->animation.roll.running_to_roll_right, &actor->actor->armature.buffer[ANIM_SLOT_ROLL_R]);
}

uint8_t actorAnimation_getLocomotionSpeedState(Entity *actor)
{
	if (actor->actor->motion.data.horizontal_speed == 0) return STANDING_IDLE;

	else if (actor->actor->motion.data.horizontal_speed > 0 && actor->actor->motion.data.horizontal_speed <= actor->actor->motion.settings.walk_target_speed) return WALKING;

	else if (actor->actor->motion.data.horizontal_speed > actor->actor->motion.settings.walk_target_speed && actor->actor->motion.data.horizontal_speed <= actor->actor->motion.settings.run_target_speed) return RUNNING;

	else return SPRINTING;
}

void actorAnimation_setLocomotionBlendingRatio(Entity *actor)
{
	uint8_t speed_state = actorAnimation_getLocomotionSpeedState(actor);
 
	switch (speed_state)
	{
		case STANDING_IDLE:
			actor->actor->animation.data.locomotion_blending_ratio = 0;
			break;
 
		case WALKING:
			actor->actor->animation.data.locomotion_blending_ratio = (actor->actor->motion.data.horizontal_speed / actor->actor->motion.settings.walk_target_speed);
			break;
 
		case RUNNING:
			actor->actor->animation.data.locomotion_blending_ratio = 1.0f - ((actor->actor->motion.data.horizontal_speed - actor->actor->motion.settings.walk_target_speed) / (actor->actor->motion.settings.run_target_speed - actor->actor->motion.settings.walk_target_speed));
			break;
 
		case SPRINTING:
			actor->actor->animation.data.locomotion_blending_ratio = ((actor->actor->motion.data.horizontal_speed - actor->actor->motion.settings.run_target_speed) / (actor->actor->motion.settings.sprint_target_speed - actor->actor->motion.settings.run_target_speed));
			break;
	}
 
	if (actor->actor->animation.data.locomotion_blending_ratio > 1.0f) actor->actor->animation.data.locomotion_blending_ratio = 1.0f;
	if (actor->actor->animation.data.locomotion_blending_ratio < 0.0f) actor->actor->animation.data.locomotion_blending_ratio = 0.0f;
}

void actorAnimation_setTurningBlendingRatio(Entity *actor)
{
    float delta_yaw = actor->transform.rotation.z - actor->actor->motion.data.previous_yaw;

    if (delta_yaw > 180.0f) delta_yaw -= 360.0f;
    if (delta_yaw <= -180.0f) delta_yaw += 360.0f;

    actor->actor->animation.data.turning_blending_ratio = delta_yaw / 5.0f;

    if (actor->actor->animation.data.turning_blending_ratio > 1.0f) actor->actor->animation.data.turning_blending_ratio = 1.0f;
    if (actor->actor->animation.data.turning_blending_ratio < -1.0f) actor->actor->animation.data.turning_blending_ratio = -1.0f;
	if (fabsf(actor->actor->animation.data.turning_blending_ratio) < 0.001f) actor->actor->animation.data.turning_blending_ratio = 0.0f;
	
}

void actorAnimation_setFootingBlendingRatio(Entity *actor, float action_time, float action_length)
{
	float phase = 1 / action_length * action_time;
	float ratio = 0;

	if      (phase <= 0.25f) ratio = 0.5f - 2.0f * phase;
	else if (phase <= 0.75f) ratio = 2.0f * (phase - 0.25f);
	else if (phase <= 1.0f)  ratio = 1.0f - 2.0f * (phase - 0.75f);

	if (ratio >= 1.0f) ratio = 0.9999999f;
	if (ratio <= 0.0f) ratio = 0.0000001f;

	actor->actor->animation.data.footing_phase = ratio;
}

void actorAnimation_setRollingBlendingRatio(Entity *actor)
{
	float roll_anim_time = actor->actor->animation.data.footing_phase < 0.5f ? actor->actor->animation.roll.running_to_roll_left.time : actor->actor->animation.roll.running_to_roll_right.time;

	if (roll_anim_time < actor->actor->animation.settings.roll.run_to_rolling_anim_ground && actor->actor->animation.data.roll_blending_ratio <= 1.0f) 
		actor->actor->animation.data.roll_blending_ratio += (time_get()->delta / actor->actor->animation.settings.roll.run_to_rolling_anim_ground);
		
	if (roll_anim_time > actor->actor->animation.settings.roll.run_to_rolling_anim_stand && actor->actor->animation.data.roll_blending_ratio > 0.0f) 
		actor->actor->animation.data.roll_blending_ratio -= (time_get()->delta / (actor->actor->animation.settings.roll.run_to_rolling_anim_length - actor->actor->animation.settings.roll.run_to_rolling_anim_stand));
	
	if (actor->actor->animation.data.roll_blending_ratio > 1.0f) {
		
		actor->actor->animation.data.roll_blending_ratio = 1.0f;

		if (actor->actor->animation.data.footing_phase < 0.5f) 
			t3d_anim_set_time(&actor->actor->animation.standing_locomotion.running, actor->actor->animation.settings.standing_locomotion.running_anim_length);
		if (actor->actor->animation.data.footing_phase >= 0.5f) 
			t3d_anim_set_time(&actor->actor->animation.standing_locomotion.running, actor->actor->animation.settings.standing_locomotion.running_anim_length_half);

		t3d_anim_set_time(&actor->actor->animation.standing_locomotion.walking, (actor->actor->animation.standing_locomotion.running.time * actor->actor->animation.settings.standing_locomotion.run_to_walk_ratio));
		t3d_anim_set_time(&actor->actor->animation.standing_locomotion.sprinting, (actor->actor->animation.standing_locomotion.running.time * actor->actor->animation.settings.standing_locomotion.run_to_sprint_ratio));			
	}

	if (actor->actor->animation.data.roll_blending_ratio < 0.0f) actor->actor->animation.data.roll_blending_ratio = 0.0f;
}

void actorAnimation_setLandingBlendingRatio(Entity *actor)
{
    float rate = actor->actor->animation.settings.jump.jump_max_blending_ratio * time_get()->delta / actor->actor->animation.settings.jump.land_anim_crouch;

    if (actor->actor->animation.jump.land_left.time < actor->actor->animation.settings.jump.land_anim_crouch){

        if (actor->actor->animation.data.land_blending_ratio < actor->actor->animation.settings.jump.jump_max_blending_ratio) actor->actor->animation.data.land_blending_ratio += rate;
        if (actor->actor->animation.data.land_blending_ratio > actor->actor->animation.settings.jump.jump_max_blending_ratio) actor->actor->animation.data.land_blending_ratio = actor->actor->animation.settings.jump.jump_max_blending_ratio;
    }

    else if (actor->actor->animation.data.land_blending_ratio > 0){

        float stand_rate = actor->actor->animation.settings.jump.jump_max_blending_ratio * time_get()->delta / (actor->actor->animation.settings.jump.land_anim_length - actor->actor->animation.settings.jump.land_anim_crouch);

        actor->actor->animation.data.land_blending_ratio -= stand_rate;

        if (actor->actor->animation.data.land_blending_ratio < 0) actor->actor->animation.data.land_blending_ratio = 0;
    }

    if (actor->actor->animation.data.jump_blending_ratio > 0) actor->actor->animation.data.jump_blending_ratio -= rate;
    if (actor->actor->animation.data.jump_blending_ratio < 0)  actor->actor->animation.data.jump_blending_ratio = 0;
}

void actorAnimation_syncLandToJump(Entity *actor)
{
    float land_time = actor->actor->animation.jump.land_left.time;
    float land_crouch = actor->actor->animation.settings.jump.land_anim_crouch;
    float land_stand = actor->actor->animation.settings.jump.land_anim_stand;
    float jump_crouch = actor->actor->animation.settings.jump.jump_anim_crouch;

    float jump_time;

    if (land_time < land_crouch){
        float progress = land_time / land_crouch;
        jump_time = progress * jump_crouch;
    }
    else {
        float progress = (land_time - land_crouch) / (land_stand - land_crouch);
        jump_time = (1.0f - progress) * jump_crouch;
    }

    t3d_anim_set_time(&actor->actor->animation.jump.jump_left, jump_time);
    t3d_anim_set_time(&actor->actor->animation.jump.jump_right, jump_time);
}

void actorAnimation_setJumpBlendingRatio(Entity *actor)
{
	if (actor->actor->animation.jump.land_left.isPlaying){
		
		if (actor->actor->animation.jump.land_left.time < actor->actor->animation.settings.jump.land_anim_crouch){

			if (actor->actor->animation.data.land_blending_ratio < actor->actor->animation.settings.jump.jump_max_blending_ratio) actor->actor->animation.data.land_blending_ratio += actor->actor->animation.settings.jump.jump_max_blending_ratio * time_get()->delta / actor->actor->animation.settings.jump.land_anim_crouch;
			if (actor->actor->animation.data.land_blending_ratio > actor->actor->animation.settings.jump.jump_max_blending_ratio) actor->actor->animation.data.land_blending_ratio = actor->actor->animation.settings.jump.jump_max_blending_ratio;
		}

		else if (actor->actor->animation.data.land_blending_ratio > 0){

			float stand_rate = actor->actor->animation.settings.jump.jump_max_blending_ratio * time_get()->delta / (actor->actor->animation.settings.jump.land_anim_stand - actor->actor->animation.settings.jump.land_anim_crouch);

			if (actor->actor->animation.data.land_blending_ratio > 0) actor->actor->animation.data.land_blending_ratio -= stand_rate;
			if (actor->actor->animation.data.land_blending_ratio < 0) actor->actor->animation.data.land_blending_ratio = 0;
		
		}
	
	}
		
	if (actor->actor->animation.data.jump_blending_ratio >= actor->actor->animation.settings.jump.jump_max_blending_ratio || actor->actor->animation.data.current != JUMPING) return;

	actor->actor->animation.data.jump_blending_ratio += (actor->actor->animation.settings.jump.jump_max_blending_ratio * time_get()->delta / actor->actor->animation.settings.jump.jump_anim_crouch);
	
	if (actor->actor->animation.data.jump_blending_ratio > actor->actor->animation.settings.jump.jump_max_blending_ratio) actor->actor->animation.data.jump_blending_ratio = actor->actor->animation.settings.jump.jump_max_blending_ratio;

}

void actorAnimation_setWalkingSpeed(Entity *actor)
{
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.walking, actor->actor->animation.data.locomotion_blending_ratio);
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.walking_turn_left, actor->actor->animation.data.locomotion_blending_ratio);
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.walking_turn_right, actor->actor->animation.data.locomotion_blending_ratio);
}

void actorAnimation_setRunningSpeed(Entity *actor)
{
	actor->actor->animation.data.speed = (1.0f - ((1.0f - actor->actor->animation.settings.standing_locomotion.walk_to_run_ratio) * actor->actor->animation.data.locomotion_blending_ratio));

	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.running, actor->actor->animation.data.speed);
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.running_turn_left, actor->actor->animation.data.speed);
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.running_turn_right, actor->actor->animation.data.speed);

	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.walking, (actor->actor->animation.data.speed * actor->actor->animation.settings.standing_locomotion.run_to_walk_ratio));
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.walking_turn_left, actor->actor->animation.standing_locomotion.walking.speed);
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.walking_turn_right, actor->actor->animation.standing_locomotion.walking.speed);
}

void actorAnimation_setSprintingSpeed(Entity *actor)
{
	actor->actor->animation.data.speed = (actor->actor->animation.settings.standing_locomotion.run_to_sprint_ratio + ((1.0f - actor->actor->animation.settings.standing_locomotion.run_to_sprint_ratio) * actor->actor->animation.data.locomotion_blending_ratio));
	
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.running, (actor->actor->animation.data.speed * actor->actor->animation.settings.standing_locomotion.sprint_to_run_ratio));
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.running_turn_left, actor->actor->animation.standing_locomotion.running.speed);
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.running_turn_right, actor->actor->animation.standing_locomotion.running.speed);

	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.sprinting, actor->actor->animation.data.speed);

	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.walking, (actor->actor->animation.data.speed * actor->actor->animation.settings.standing_locomotion.sprint_to_walk_ratio));
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.walking_turn_left, actor->actor->animation.standing_locomotion.walking.speed);
	t3d_anim_set_speed(&actor->actor->animation.standing_locomotion.walking_turn_right, actor->actor->animation.standing_locomotion.walking.speed);
}


void actorAnimation_setStandingLocomotion(Entity *actor)
{
	ActorAnimationBuffer buffer;
	buffer.count = 0;

	uint8_t speed_state = actorAnimation_getLocomotionSpeedState(actor);
	actorAnimation_setTurningBlendingRatio(actor);
					

	switch (speed_state)
	{
		case STANDING_IDLE:
		{
			if (actor->actor->animation.data.current != STANDING_IDLE){
				actor->actor->animation.data.previous = actor->actor->animation.data.current;
				actor->actor->animation.data.current = STANDING_IDLE;
			}

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_right, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);

			if (actor->actor->animation.jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->actor->animation.data.jump_blending_ratio > 0){

					if ((actor->actor->animation.jump.jump_left.time + time_get()->delta) < actor->actor->animation.settings.jump.jump_anim_length){
						t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case WALKING:
		{
			if (actor->actor->animation.data.current != WALKING){
				
				actor->actor->animation.data.previous = actor->actor->animation.data.current;
				actor->actor->animation.data.current = WALKING;

				t3d_anim_set_time(&actor->actor->animation.standing_locomotion.walking_turn_left, actor->actor->animation.standing_locomotion.walking.time);
				t3d_anim_set_time(&actor->actor->animation.standing_locomotion.walking_turn_right, actor->actor->animation.standing_locomotion.walking.time);
			}

			actorAnimation_setFootingBlendingRatio(actor, actor->actor->animation.standing_locomotion.walking.time, actor->actor->animation.settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setWalkingSpeed(actor);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_right, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);
			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_WALK], actor->actor->animation.data.locomotion_blending_ratio);
			
			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){

				if (actor->actor->animation.data.turning_blending_ratio < 0.0f){
				
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_right, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_left, time_get()->delta);
				}
				else {
				
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_left, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_right, time_get()->delta);
				}

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_WALK], fabsf(actor->actor->animation.data.turning_blending_ratio * actor->actor->animation.data.locomotion_blending_ratio));
			}				
			else {
				
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_left, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_right, time_get()->delta);
			}

			if (actor->actor->animation.jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->actor->animation.data.jump_blending_ratio > 0){
					if ((actor->actor->animation.jump.jump_left.time + time_get()->delta) < actor->actor->animation.settings.jump.jump_anim_length){
						t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case RUNNING:
		{
			if (actor->actor->animation.data.current != RUNNING){
				actor->actor->animation.data.previous = actor->actor->animation.data.current;
				actor->actor->animation.data.current = RUNNING;

				if (actor->actor->animation.data.previous == WALKING || actor->actor->animation.data.previous == JUMPING || actor->actor->animation.data.previous == ROLLING) {

					t3d_anim_set_time(&actor->actor->animation.standing_locomotion.running, (actor->actor->animation.standing_locomotion.walking.time * actor->actor->animation.settings.standing_locomotion.walk_to_run_ratio));

					t3d_anim_set_time(&actor->actor->animation.standing_locomotion.running_turn_left, actor->actor->animation.standing_locomotion.running.time);
					t3d_anim_set_time(&actor->actor->animation.standing_locomotion.running_turn_right, actor->actor->animation.standing_locomotion.running.time);
				}

				else if (actor->actor->animation.data.previous == SPRINTING) {

					t3d_anim_set_time(&actor->actor->animation.standing_locomotion.walking, (actor->actor->animation.standing_locomotion.sprinting.time * actor->actor->animation.settings.standing_locomotion.sprint_to_walk_ratio));

					t3d_anim_set_time(&actor->actor->animation.standing_locomotion.walking_turn_left, actor->actor->animation.standing_locomotion.walking.time);
					t3d_anim_set_time(&actor->actor->animation.standing_locomotion.walking_turn_right, actor->actor->animation.standing_locomotion.walking.time);
				}
			}

			actorAnimation_setFootingBlendingRatio(actor, actor->actor->animation.standing_locomotion.walking.time, actor->actor->animation.settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setRunningSpeed(actor);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_WALK], actor->actor->animation.data.locomotion_blending_ratio);

			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){

				if (actor->actor->animation.data.turning_blending_ratio < 0.0f){
				
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_right, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_left, time_get()->delta);
					
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
				}
				else {
				
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_left, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_right, time_get()->delta);
					
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
				}

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->actor->animation.data.turning_blending_ratio * (1 - actor->actor->animation.data.locomotion_blending_ratio)));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_WALK], fabsf(actor->actor->animation.data.turning_blending_ratio * actor->actor->animation.data.locomotion_blending_ratio));
			}				
			else {
				
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_left, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_right, time_get()->delta);
					
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
			}

			if (actor->actor->animation.jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->actor->animation.data.jump_blending_ratio > 0){
					if ((actor->actor->animation.jump.jump_left.time + time_get()->delta) < actor->actor->animation.settings.jump.jump_anim_length){
						t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case SPRINTING:
		{
			if (actor->actor->animation.data.current != SPRINTING){
				actor->actor->animation.data.previous = actor->actor->animation.data.current;
				actor->actor->animation.data.current = SPRINTING;
				t3d_anim_set_time(&actor->actor->animation.standing_locomotion.sprinting, (actor->actor->animation.standing_locomotion.running.time * actor->actor->animation.settings.standing_locomotion.run_to_sprint_ratio));
			}

			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);
			actorAnimation_setFootingBlendingRatio(actor, actor->actor->animation.standing_locomotion.walking.time, actor->actor->animation.settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setSprintingSpeed(actor);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.sprinting, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.data.locomotion_blending_ratio);


			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){

				if (actor->actor->animation.data.turning_blending_ratio < 0.0f){
					
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
				}
				else {
					
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
				}

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->actor->animation.data.turning_blending_ratio));
			}				
			else {
					
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
			}

			if (actor->actor->animation.jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->actor->animation.data.jump_blending_ratio > 0){
					if ((actor->actor->animation.jump.jump_left.time + time_get()->delta) < actor->actor->animation.settings.jump.jump_anim_length){
						t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}
	}
}


void actorAnimation_setRolling(Entity *actor)
{
	ActorAnimationBuffer buffer;
	buffer.count = 0;

	if (actor->actor->animation.data.current != ROLLING){

		actor->actor->animation.data.previous = actor->actor->animation.data.current;
		actor->actor->animation.data.current = ROLLING;

		t3d_anim_set_playing(&actor->actor->animation.roll.running_to_roll_left, true);
		t3d_anim_set_time(&actor->actor->animation.roll.running_to_roll_left, 0.0f);

		t3d_anim_set_playing(&actor->actor->animation.roll.running_to_roll_right, true);
		t3d_anim_set_time(&actor->actor->animation.roll.running_to_roll_right, 0.0f);
	}

	uint8_t speed_state = actorAnimation_getLocomotionSpeedState(actor);
	actorAnimation_setTurningBlendingRatio(actor);

	switch (speed_state)
	{
		case STANDING_IDLE:
		case WALKING:
		case RUNNING:
		{
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setRunningSpeed(actor);
			actorAnimation_setRollingBlendingRatio(actor);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_WALK], actor->actor->animation.data.locomotion_blending_ratio);

			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){
				if (actor->actor->animation.data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->actor->animation.data.turning_blending_ratio));
			}

			if (footing_phase >= 0.5f){
				t3d_anim_update(&actor->actor->animation.roll.running_to_roll_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_ROLL_R], actor->actor->animation.data.roll_blending_ratio);
			}
			else {
				t3d_anim_update(&actor->actor->animation.roll.running_to_roll_left, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_ROLL_L], actor->actor->animation.data.roll_blending_ratio);
			}

			if (actor->actor->animation.jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->actor->animation.data.jump_blending_ratio > 0){

					if ((actor->actor->animation.jump.jump_left.time + time_get()->delta) < actor->actor->animation.settings.jump.jump_anim_length){
						t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case SPRINTING:
		{
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setSprintingSpeed(actor);
			actorAnimation_setRollingBlendingRatio(actor);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.sprinting, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.data.locomotion_blending_ratio);

			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){
				if (actor->actor->animation.data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->actor->animation.data.turning_blending_ratio));
			}

			if (footing_phase >= 0.5f){
				t3d_anim_update(&actor->actor->animation.roll.running_to_roll_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_ROLL_R], actor->actor->animation.data.roll_blending_ratio);
			}
			else {
				t3d_anim_update(&actor->actor->animation.roll.running_to_roll_left, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_ROLL_L], actor->actor->animation.data.roll_blending_ratio);
			}

			if (actor->actor->animation.jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->actor->animation.data.jump_blending_ratio > 0){
					if ((actor->actor->animation.jump.jump_left.time + time_get()->delta) < actor->actor->animation.settings.jump.jump_anim_length){
						t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}
	}
}


void actorAnimation_setJumping(Entity *actor)
{
	ActorAnimationBuffer buffer;
	buffer.count = 0;

	if (actor->actor->animation.data.current != JUMPING){

		actor->actor->animation.data.previous = actor->actor->animation.data.current;
		actor->actor->animation.data.current = JUMPING;
		
		t3d_anim_set_playing(&actor->actor->animation.jump.jump_left, true);
		t3d_anim_set_playing(&actor->actor->animation.jump.jump_right, true);

		if (actor->actor->animation.jump.land_left.isPlaying == false){

			actorAnimation_syncLandToJump(actor);
		}
		
		else {

			t3d_anim_set_time(&actor->actor->animation.jump.jump_left, 0.0f);
			t3d_anim_set_time(&actor->actor->animation.jump.jump_right, 0.0f);
			t3d_anim_set_time(&actor->actor->animation.jump.falling_left, 0.0f);
			t3d_anim_set_time(&actor->actor->animation.jump.falling_right, 0.0f);
			actor->actor->animation.data.jump_blending_ratio = 0.0f;
		}
	}

	uint8_t speed_state = actorAnimation_getLocomotionSpeedState(actor);
	actorAnimation_setTurningBlendingRatio(actor);

	bool is_crouching = (actor->actor->animation.jump.jump_left.time < actor->actor->animation.settings.jump.jump_anim_crouch);
	bool is_jumping = ((actor->actor->animation.jump.jump_left.time + time_get()->delta) < actor->actor->animation.settings.jump.jump_anim_length);

	actorAnimation_setJumpBlendingRatio(actor);

	switch (speed_state)
	{
		case STANDING_IDLE:
		{
			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_right, time_get()->delta);

			if (is_jumping){
				t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
			}
			else {
				t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
			}

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);

			if (actor->actor->animation.jump.land_left.isPlaying){

				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case WALKING:
		{
			actorAnimation_setFootingBlendingRatio(actor, actor->actor->animation.standing_locomotion.walking.time, actor->actor->animation.settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setWalkingSpeed(actor);
			actor->actor->animation.standing_locomotion.walking.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_right, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_WALK], actor->actor->animation.data.locomotion_blending_ratio);
			
			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){
				if (actor->actor->animation.data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_WALK], fabsf(actor->actor->animation.data.turning_blending_ratio));
			}
			
			if (is_jumping){
				t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
			}
			else {
				t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
			}

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);

			if (is_crouching && actor->actor->animation.jump.land_left.isPlaying){
				
				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case RUNNING:
		{
			actorAnimation_setFootingBlendingRatio(actor, actor->actor->animation.standing_locomotion.walking.time, actor->actor->animation.settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setRunningSpeed(actor);
			actor->actor->animation.standing_locomotion.walking.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);
			actor->actor->animation.standing_locomotion.running.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_WALK], actor->actor->animation.data.locomotion_blending_ratio);

			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){
				if (actor->actor->animation.data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->actor->animation.data.turning_blending_ratio));
			}

			if (is_jumping){
				t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
			}
			else {
				t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
			}

			if (is_crouching && actor->actor->animation.jump.land_left.isPlaying){
				
				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case SPRINTING:
		{
			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);
			actorAnimation_setFootingBlendingRatio(actor, actor->actor->animation.standing_locomotion.walking.time, actor->actor->animation.settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setSprintingSpeed(actor);
			actor->actor->animation.standing_locomotion.walking.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);
			actor->actor->animation.standing_locomotion.running.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);
			actor->actor->animation.standing_locomotion.sprinting.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.sprinting, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.data.locomotion_blending_ratio);

			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){
				if (actor->actor->animation.data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->actor->animation.data.turning_blending_ratio));
			}

			if (is_jumping){
				t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);
			}
			else {
				t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);
			}

			if (is_crouching && actor->actor->animation.jump.land_left.isPlaying){
				
				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}
	}
}


void actorAnimation_setFalling(Entity *actor)
{
	ActorAnimationBuffer buffer;
	buffer.count = 0;

	if (actor->actor->animation.data.current != FALLING){

		actor->actor->animation.data.previous = actor->actor->animation.data.current;
		actor->actor->animation.data.current = FALLING;

		t3d_anim_set_playing(&actor->actor->animation.jump.land_left, true);
		t3d_anim_set_playing(&actor->actor->animation.jump.land_right, true);
		t3d_anim_set_time(&actor->actor->animation.jump.land_left, 0.0f);
		t3d_anim_set_time(&actor->actor->animation.jump.land_right, 0.0f);
		actor->actor->animation.data.land_blending_ratio = 0.0f;
	}

	uint8_t speed_state = actorAnimation_getLocomotionSpeedState(actor);
	actorAnimation_setTurningBlendingRatio(actor);

	bool is_landing = (actor->transform.position.z < LAND_ANIM_STARTING_HEIGHT);
	bool is_jumping = ((actor->actor->animation.jump.jump_left.time + time_get()->delta) < actor->actor->animation.settings.jump.jump_anim_length);

	if (is_jumping) actorAnimation_setJumpBlendingRatio(actor);
	if (is_landing) actorAnimation_setLandingBlendingRatio(actor);

	switch (speed_state)
	{
		case STANDING_IDLE:
		{
			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_right, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);

			if (is_jumping){
				t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
			}
			else {
				t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
			}

			if (is_landing){
				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case WALKING:
		{
			actorAnimation_setFootingBlendingRatio(actor, actor->actor->animation.standing_locomotion.walking.time, actor->actor->animation.settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setWalkingSpeed(actor);
			actor->actor->animation.standing_locomotion.walking.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.standing_idle_right, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);
			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_WALK], actor->actor->animation.data.locomotion_blending_ratio);
			
			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){
				if (actor->actor->animation.data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->actor->animation.standing_locomotion.walking_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_WALK], fabsf(actor->actor->animation.data.turning_blending_ratio));
			}
			
			if (is_jumping){
				t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
			}
			else {
				t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
			}

			if (is_landing){
				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case RUNNING:
		{
			actorAnimation_setFootingBlendingRatio(actor, actor->actor->animation.standing_locomotion.walking.time, actor->actor->animation.settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setRunningSpeed(actor);
			actor->actor->animation.standing_locomotion.walking.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);
			actor->actor->animation.standing_locomotion.running.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_WALK], actor->actor->animation.data.locomotion_blending_ratio);

			if (is_jumping){
				t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
			}
			else {
				t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
			}

			if (is_landing){
				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}

		case SPRINTING:
		{
			t3d_anim_update(&actor->actor->animation.standing_locomotion.walking, time_get()->delta);
			actorAnimation_setFootingBlendingRatio(actor, actor->actor->animation.standing_locomotion.walking.time, actor->actor->animation.settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setSprintingSpeed(actor);
			actor->actor->animation.standing_locomotion.walking.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);
			actor->actor->animation.standing_locomotion.running.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);
			actor->actor->animation.standing_locomotion.sprinting.speed *= actor->actor->animation.settings.jump.jump_footing_speed * (1 - actor->actor->animation.data.jump_blending_ratio);

			float footing_phase = actor->actor->animation.data.footing_phase;

			t3d_anim_update(&actor->actor->animation.standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->actor->animation.standing_locomotion.sprinting, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_IDLE_R], actor->actor->animation.data.locomotion_blending_ratio);

			if (actor->actor->animation.data.turning_blending_ratio != 0.0f){
				if (actor->actor->animation.data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->actor->animation.standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->actor->animation.data.turning_blending_ratio));
			}

			if (is_jumping){
				t3d_anim_update(&actor->actor->animation.jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.jump_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
			}
			else {
				t3d_anim_update(&actor->actor->animation.jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.falling_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_L], actor->actor->animation.data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_JUMP_R], actor->actor->animation.data.jump_blending_ratio * footing_phase);
			}

			if (is_landing){
				t3d_anim_update(&actor->actor->animation.jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->actor->animation.jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_L], actor->actor->animation.data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->actor->armature.buffer[ANIM_SLOT_LAND_R], actor->actor->animation.data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->actor->armature.main, &buffer);
			break;
		}
	}
}

static void (*actorAnimation_handler[])(Entity *) = {
	
    [STANDING_IDLE] = actorAnimation_setStandingLocomotion,
    [WALKING]       = actorAnimation_setStandingLocomotion,
    [RUNNING]       = actorAnimation_setStandingLocomotion,
    [SPRINTING]     = actorAnimation_setStandingLocomotion,
    [ROLLING]       = actorAnimation_setRolling,
    [JUMPING]       = actorAnimation_setJumping,
    [FALLING]       = actorAnimation_setFalling,
};

void actorAnimation_set(Entity *actor)
{
    actorAnimation_handler[actor->actor->state.current](actor);
    t3d_skeleton_update(&actor->actor->armature.main);
}
