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
	actor->armature->main = t3d_skeleton_create_buffered(actor->mesh->model, FB_COUNT);

	for (int i = 0; i < ANIM_SLOT_COUNT; i++)
		actor->armature->buffer[i] = t3d_skeleton_clone(&actor->armature->main, false);
}

void actorAnimation_initStandingLocomotionSet(Entity *actor)
{
	actor->animation->standing_locomotion.standing_idle_left = t3d_anim_create(actor->mesh->model, "standing-idle-left");
	actor->animation->standing_locomotion.standing_idle_right = t3d_anim_create(actor->mesh->model, "standing-idle-right");

	actor->animation->standing_locomotion.walking = t3d_anim_create(actor->mesh->model, "walking");
	actor->animation->standing_locomotion.running = t3d_anim_create(actor->mesh->model, "running");
	actor->animation->standing_locomotion.sprinting = t3d_anim_create(actor->mesh->model, "sprinting");

	actor->animation->standing_locomotion.walking_turn_left = t3d_anim_create(actor->mesh->model, "walking-turn-left");
	actor->animation->standing_locomotion.walking_turn_right = t3d_anim_create(actor->mesh->model, "walking-turn-right");

	actor->animation->standing_locomotion.running_turn_left = t3d_anim_create(actor->mesh->model, "running-turn-left");
	actor->animation->standing_locomotion.running_turn_right = t3d_anim_create(actor->mesh->model, "running-turn-right");

	t3d_anim_attach(&actor->animation->standing_locomotion.standing_idle_left, &actor->armature->main);
	t3d_anim_attach(&actor->animation->standing_locomotion.standing_idle_right, &actor->armature->buffer[ANIM_SLOT_IDLE_R]);

	t3d_anim_attach(&actor->animation->standing_locomotion.walking, &actor->armature->buffer[ANIM_SLOT_WALK]);
	t3d_anim_attach(&actor->animation->standing_locomotion.running, &actor->armature->main);
	t3d_anim_attach(&actor->animation->standing_locomotion.sprinting, &actor->armature->buffer[ANIM_SLOT_IDLE_R]);
	
	t3d_anim_attach(&actor->animation->standing_locomotion.walking_turn_left, &actor->armature->buffer[ANIM_SLOT_TURN_WALK]);
	t3d_anim_attach(&actor->animation->standing_locomotion.walking_turn_right, &actor->armature->buffer[ANIM_SLOT_TURN_WALK]);
	t3d_anim_attach(&actor->animation->standing_locomotion.running_turn_left, &actor->armature->buffer[ANIM_SLOT_TURN_RUN]);
	t3d_anim_attach(&actor->animation->standing_locomotion.running_turn_right, &actor->armature->buffer[ANIM_SLOT_TURN_RUN]);
}

void actorAnimation_initJumpSet(Entity *actor)
{
	actor->animation->jump.jump_left = t3d_anim_create(actor->mesh->model, "jump-left");
	actor->animation->jump.jump_right = t3d_anim_create(actor->mesh->model, "jump-right");

	actor->animation->jump.falling_left = t3d_anim_create(actor->mesh->model, "falling-idle-left");
	actor->animation->jump.falling_right = t3d_anim_create(actor->mesh->model, "falling-idle-right");

	actor->animation->jump.land_left = t3d_anim_create(actor->mesh->model, "land-left");
	actor->animation->jump.land_right = t3d_anim_create(actor->mesh->model, "land-right");

	t3d_anim_set_looping(&actor->animation->jump.jump_left, false);
	t3d_anim_set_looping(&actor->animation->jump.jump_right, false);

	t3d_anim_set_looping(&actor->animation->jump.land_left, false);
	t3d_anim_set_looping(&actor->animation->jump.land_right, false);

	t3d_anim_attach(&actor->animation->jump.jump_left, &actor->armature->buffer[ANIM_SLOT_JUMP_L]);
	t3d_anim_attach(&actor->animation->jump.jump_right, &actor->armature->buffer[ANIM_SLOT_JUMP_R]);

	t3d_anim_attach(&actor->animation->jump.falling_left, &actor->armature->buffer[ANIM_SLOT_JUMP_L]);
	t3d_anim_attach(&actor->animation->jump.falling_right, &actor->armature->buffer[ANIM_SLOT_JUMP_R]);

	t3d_anim_attach(&actor->animation->jump.land_left, &actor->armature->buffer[ANIM_SLOT_LAND_L]);
	t3d_anim_attach(&actor->animation->jump.land_right, &actor->armature->buffer[ANIM_SLOT_LAND_R]);
	
	t3d_anim_set_playing(&actor->animation->jump.jump_left, false);
	t3d_anim_set_playing(&actor->animation->jump.jump_right, false);

	t3d_anim_set_playing(&actor->animation->jump.land_left, false);
	t3d_anim_set_playing(&actor->animation->jump.land_right, false);
}

void actorAnimation_initRollSet(Entity *actor)
{
	actor->animation->roll.running_to_roll_right = t3d_anim_create(actor->mesh->model, "running-to-roll-right");
	actor->animation->roll.running_to_roll_left = t3d_anim_create(actor->mesh->model, "running-to-roll-left");

	t3d_anim_set_looping(&actor->animation->roll.running_to_roll_left, false);
	t3d_anim_set_looping(&actor->animation->roll.running_to_roll_right, false);

	t3d_anim_attach(&actor->animation->roll.running_to_roll_left, &actor->armature->buffer[ANIM_SLOT_ROLL_L]);
	t3d_anim_attach(&actor->animation->roll.running_to_roll_right, &actor->armature->buffer[ANIM_SLOT_ROLL_R]);
}

uint8_t actorAnimation_getLocomotionSpeedState(Entity *actor)
{
	if (actor->motion->data.horizontal_speed == 0) return STANDING_IDLE;

	else if (actor->motion->data.horizontal_speed > 0 && actor->motion->data.horizontal_speed <= actor->motion->settings.walk_target_speed) return WALKING;

	else if (actor->motion->data.horizontal_speed > actor->motion->settings.walk_target_speed && actor->motion->data.horizontal_speed <= actor->motion->settings.run_target_speed) return RUNNING;

	else return SPRINTING;
}

void actorAnimation_setLocomotionBlendingRatio(Entity *actor)
{
	uint8_t speed_state = actorAnimation_getLocomotionSpeedState(actor);
 
	switch (speed_state)
	{
		case STANDING_IDLE:
			actor->animation->data.locomotion_blending_ratio = 0;
			break;
 
		case WALKING:
			actor->animation->data.locomotion_blending_ratio = (actor->motion->data.horizontal_speed / actor->motion->settings.walk_target_speed);
			break;
 
		case RUNNING:
			actor->animation->data.locomotion_blending_ratio = 1.0f - ((actor->motion->data.horizontal_speed - actor->motion->settings.walk_target_speed) / (actor->motion->settings.run_target_speed - actor->motion->settings.walk_target_speed));
			break;
 
		case SPRINTING:
			actor->animation->data.locomotion_blending_ratio = ((actor->motion->data.horizontal_speed - actor->motion->settings.run_target_speed) / (actor->motion->settings.sprint_target_speed - actor->motion->settings.run_target_speed));
			break;
	}
 
	if (actor->animation->data.locomotion_blending_ratio > 1.0f) actor->animation->data.locomotion_blending_ratio = 1.0f;
	if (actor->animation->data.locomotion_blending_ratio < 0.0f) actor->animation->data.locomotion_blending_ratio = 0.0f;
}

void actorAnimation_setTurningBlendingRatio(Entity *actor)
{
    float delta_yaw = actor->transform.rotation.z - actor->motion->data.previous_yaw;

    if (delta_yaw > 180.0f) delta_yaw -= 360.0f;
    if (delta_yaw <= -180.0f) delta_yaw += 360.0f;

    actor->animation->data.turning_blending_ratio = delta_yaw / 5.0f;

    if (actor->animation->data.turning_blending_ratio > 1.0f) actor->animation->data.turning_blending_ratio = 1.0f;
    if (actor->animation->data.turning_blending_ratio < -1.0f) actor->animation->data.turning_blending_ratio = -1.0f;
	if (fabsf(actor->animation->data.turning_blending_ratio) < 0.001f) actor->animation->data.turning_blending_ratio = 0.0f;
	
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

	actor->animation->data.footing_phase = ratio;
}

void actorAnimation_setRollingBlendingRatio(Entity *actor)
{
	float roll_anim_time = actor->animation->data.footing_phase < 0.5f ? actor->animation->roll.running_to_roll_left.time : actor->animation->roll.running_to_roll_right.time;

	if (roll_anim_time < actor->animation->settings.roll.run_to_rolling_anim_ground && actor->animation->data.roll_blending_ratio <= 1.0f) 
		actor->animation->data.roll_blending_ratio += (time_get()->delta / actor->animation->settings.roll.run_to_rolling_anim_ground);
		
	if (roll_anim_time > actor->animation->settings.roll.run_to_rolling_anim_stand && actor->animation->data.roll_blending_ratio > 0.0f) 
		actor->animation->data.roll_blending_ratio -= (time_get()->delta / (actor->animation->settings.roll.run_to_rolling_anim_length - actor->animation->settings.roll.run_to_rolling_anim_stand));
	
	if (actor->animation->data.roll_blending_ratio > 1.0f) {
		
		actor->animation->data.roll_blending_ratio = 1.0f;

		if (actor->animation->data.footing_phase < 0.5f) 
			t3d_anim_set_time(&actor->animation->standing_locomotion.running, actor->animation->settings.standing_locomotion.running_anim_length);
		if (actor->animation->data.footing_phase >= 0.5f) 
			t3d_anim_set_time(&actor->animation->standing_locomotion.running, actor->animation->settings.standing_locomotion.running_anim_length_half);

		t3d_anim_set_time(&actor->animation->standing_locomotion.walking, (actor->animation->standing_locomotion.running.time * actor->animation->settings.standing_locomotion.run_to_walk_ratio));
		t3d_anim_set_time(&actor->animation->standing_locomotion.sprinting, (actor->animation->standing_locomotion.running.time * actor->animation->settings.standing_locomotion.run_to_sprint_ratio));			
	}

	if (actor->animation->data.roll_blending_ratio < 0.0f) actor->animation->data.roll_blending_ratio = 0.0f;
}

void actorAnimation_setLandingBlendingRatio(Entity *actor)
{
    float rate = actor->animation->settings.jump.jump_max_blending_ratio * time_get()->delta / actor->animation->settings.jump.land_anim_crouch;

    if (actor->animation->jump.land_left.time < actor->animation->settings.jump.land_anim_crouch){

        if (actor->animation->data.land_blending_ratio < actor->animation->settings.jump.jump_max_blending_ratio) actor->animation->data.land_blending_ratio += rate;
        if (actor->animation->data.land_blending_ratio > actor->animation->settings.jump.jump_max_blending_ratio) actor->animation->data.land_blending_ratio = actor->animation->settings.jump.jump_max_blending_ratio;
    }

    else if (actor->animation->data.land_blending_ratio > 0){

        float stand_rate = actor->animation->settings.jump.jump_max_blending_ratio * time_get()->delta / (actor->animation->settings.jump.land_anim_length - actor->animation->settings.jump.land_anim_crouch);

        actor->animation->data.land_blending_ratio -= stand_rate;

        if (actor->animation->data.land_blending_ratio < 0) actor->animation->data.land_blending_ratio = 0;
    }

    if (actor->animation->data.jump_blending_ratio > 0) actor->animation->data.jump_blending_ratio -= rate;
    if (actor->animation->data.jump_blending_ratio < 0)  actor->animation->data.jump_blending_ratio = 0;
}

void actorAnimation_syncLandToJump(Entity *actor)
{
    float land_time = actor->animation->jump.land_left.time;
    float land_crouch = actor->animation->settings.jump.land_anim_crouch;
    float land_stand = actor->animation->settings.jump.land_anim_stand;
    float jump_crouch = actor->animation->settings.jump.jump_anim_crouch;

    float jump_time;

    if (land_time < land_crouch){
        float progress = land_time / land_crouch;
        jump_time = progress * jump_crouch;
    }
    else {
        float progress = (land_time - land_crouch) / (land_stand - land_crouch);
        jump_time = (1.0f - progress) * jump_crouch;
    }

    t3d_anim_set_time(&actor->animation->jump.jump_left, jump_time);
    t3d_anim_set_time(&actor->animation->jump.jump_right, jump_time);
}

void actorAnimation_setJumpBlendingRatio(Entity *actor)
{
	if (actor->animation->jump.land_left.isPlaying){
		
		if (actor->animation->jump.land_left.time < actor->animation->settings.jump.land_anim_crouch){

			if (actor->animation->data.land_blending_ratio < actor->animation->settings.jump.jump_max_blending_ratio) actor->animation->data.land_blending_ratio += actor->animation->settings.jump.jump_max_blending_ratio * time_get()->delta / actor->animation->settings.jump.land_anim_crouch;
			if (actor->animation->data.land_blending_ratio > actor->animation->settings.jump.jump_max_blending_ratio) actor->animation->data.land_blending_ratio = actor->animation->settings.jump.jump_max_blending_ratio;
		}

		else if (actor->animation->data.land_blending_ratio > 0){

			float stand_rate = actor->animation->settings.jump.jump_max_blending_ratio * time_get()->delta / (actor->animation->settings.jump.land_anim_stand - actor->animation->settings.jump.land_anim_crouch);

			if (actor->animation->data.land_blending_ratio > 0) actor->animation->data.land_blending_ratio -= stand_rate;
			if (actor->animation->data.land_blending_ratio < 0) actor->animation->data.land_blending_ratio = 0;
		
		}
	
	}
		
	if (actor->animation->data.jump_blending_ratio >= actor->animation->settings.jump.jump_max_blending_ratio || actor->animation->data.current != JUMPING) return;

	actor->animation->data.jump_blending_ratio += (actor->animation->settings.jump.jump_max_blending_ratio * time_get()->delta / actor->animation->settings.jump.jump_anim_crouch);
	
	if (actor->animation->data.jump_blending_ratio > actor->animation->settings.jump.jump_max_blending_ratio) actor->animation->data.jump_blending_ratio = actor->animation->settings.jump.jump_max_blending_ratio;

}

void actorAnimation_setWalkingSpeed(Entity *actor)
{
	t3d_anim_set_speed(&actor->animation->standing_locomotion.walking, actor->animation->data.locomotion_blending_ratio);
	t3d_anim_set_speed(&actor->animation->standing_locomotion.walking_turn_left, actor->animation->data.locomotion_blending_ratio);
	t3d_anim_set_speed(&actor->animation->standing_locomotion.walking_turn_right, actor->animation->data.locomotion_blending_ratio);
}

void actorAnimation_setRunningSpeed(Entity *actor)
{
	actor->animation->data.speed = (1.0f - ((1.0f - actor->animation->settings.standing_locomotion.walk_to_run_ratio) * actor->animation->data.locomotion_blending_ratio));

	t3d_anim_set_speed(&actor->animation->standing_locomotion.running, actor->animation->data.speed);
	t3d_anim_set_speed(&actor->animation->standing_locomotion.running_turn_left, actor->animation->data.speed);
	t3d_anim_set_speed(&actor->animation->standing_locomotion.running_turn_right, actor->animation->data.speed);

	t3d_anim_set_speed(&actor->animation->standing_locomotion.walking, (actor->animation->data.speed * actor->animation->settings.standing_locomotion.run_to_walk_ratio));
	t3d_anim_set_speed(&actor->animation->standing_locomotion.walking_turn_left, actor->animation->standing_locomotion.walking.speed);
	t3d_anim_set_speed(&actor->animation->standing_locomotion.walking_turn_right, actor->animation->standing_locomotion.walking.speed);
}

void actorAnimation_setSprintingSpeed(Entity *actor)
{
	actor->animation->data.speed = (actor->animation->settings.standing_locomotion.run_to_sprint_ratio + ((1.0f - actor->animation->settings.standing_locomotion.run_to_sprint_ratio) * actor->animation->data.locomotion_blending_ratio));
	
	t3d_anim_set_speed(&actor->animation->standing_locomotion.running, (actor->animation->data.speed * actor->animation->settings.standing_locomotion.sprint_to_run_ratio));
	t3d_anim_set_speed(&actor->animation->standing_locomotion.running_turn_left, actor->animation->standing_locomotion.running.speed);
	t3d_anim_set_speed(&actor->animation->standing_locomotion.running_turn_right, actor->animation->standing_locomotion.running.speed);

	t3d_anim_set_speed(&actor->animation->standing_locomotion.sprinting, actor->animation->data.speed);

	t3d_anim_set_speed(&actor->animation->standing_locomotion.walking, (actor->animation->data.speed * actor->animation->settings.standing_locomotion.sprint_to_walk_ratio));
	t3d_anim_set_speed(&actor->animation->standing_locomotion.walking_turn_left, actor->animation->standing_locomotion.walking.speed);
	t3d_anim_set_speed(&actor->animation->standing_locomotion.walking_turn_right, actor->animation->standing_locomotion.walking.speed);
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
			if (actor->animation->data.current != STANDING_IDLE){
				actor->animation->data.previous = actor->animation->data.current;
				actor->animation->data.current = STANDING_IDLE;
			}

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_right, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);

			if (actor->animation->jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->animation->data.jump_blending_ratio > 0){
					
					if ((actor->animation->jump.jump_left.time + time_get()->delta) < actor->animation->settings.jump.jump_anim_length){
						t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case WALKING:
		{
			if (actor->animation->data.current != WALKING){
				
				actor->animation->data.previous = actor->animation->data.current;
				actor->animation->data.current = WALKING;

				t3d_anim_set_time(&actor->animation->standing_locomotion.walking_turn_left, actor->animation->standing_locomotion.walking.time);
				t3d_anim_set_time(&actor->animation->standing_locomotion.walking_turn_right, actor->animation->standing_locomotion.walking.time);
			}

			actorAnimation_setFootingBlendingRatio(actor, actor->animation->standing_locomotion.walking.time, actor->animation->settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setWalkingSpeed(actor);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_right, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);
			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_WALK], actor->animation->data.locomotion_blending_ratio);
			
			if (actor->animation->data.turning_blending_ratio != 0.0f){

				if (actor->animation->data.turning_blending_ratio < 0.0f){
				
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_right, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_left, time_get()->delta);
				}
				else {
				
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_left, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_right, time_get()->delta);
				}

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_WALK], fabsf(actor->animation->data.turning_blending_ratio * actor->animation->data.locomotion_blending_ratio));
			}				
			else {
				
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_left, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_right, time_get()->delta);
			}

			if (actor->animation->jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->animation->data.jump_blending_ratio > 0){
					if ((actor->animation->jump.jump_left.time + time_get()->delta) < actor->animation->settings.jump.jump_anim_length){
						t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case RUNNING:
		{
			if (actor->animation->data.current != RUNNING){
				actor->animation->data.previous = actor->animation->data.current;
				actor->animation->data.current = RUNNING;

				if (actor->animation->data.previous == WALKING || actor->animation->data.previous == JUMPING || actor->animation->data.previous == ROLLING) {

					t3d_anim_set_time(&actor->animation->standing_locomotion.running, (actor->animation->standing_locomotion.walking.time * actor->animation->settings.standing_locomotion.walk_to_run_ratio));

					t3d_anim_set_time(&actor->animation->standing_locomotion.running_turn_left, actor->animation->standing_locomotion.running.time);
					t3d_anim_set_time(&actor->animation->standing_locomotion.running_turn_right, actor->animation->standing_locomotion.running.time);
				}

				else if (actor->animation->data.previous == SPRINTING) {

					t3d_anim_set_time(&actor->animation->standing_locomotion.walking, (actor->animation->standing_locomotion.sprinting.time * actor->animation->settings.standing_locomotion.sprint_to_walk_ratio));

					t3d_anim_set_time(&actor->animation->standing_locomotion.walking_turn_left, actor->animation->standing_locomotion.walking.time);
					t3d_anim_set_time(&actor->animation->standing_locomotion.walking_turn_right, actor->animation->standing_locomotion.walking.time);
				}
			}

			actorAnimation_setFootingBlendingRatio(actor, actor->animation->standing_locomotion.walking.time, actor->animation->settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setRunningSpeed(actor);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_WALK], actor->animation->data.locomotion_blending_ratio);

			if (actor->animation->data.turning_blending_ratio != 0.0f){

				if (actor->animation->data.turning_blending_ratio < 0.0f){
				
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_right, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_left, time_get()->delta);
					
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
				}
				else {
				
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_left, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_right, time_get()->delta);
					
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
				}

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->animation->data.turning_blending_ratio * (1 - actor->animation->data.locomotion_blending_ratio)));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_WALK], fabsf(actor->animation->data.turning_blending_ratio * actor->animation->data.locomotion_blending_ratio));
			}				
			else {
				
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_left, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_right, time_get()->delta);
					
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
			}

			if (actor->animation->jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->animation->data.jump_blending_ratio > 0){
					if ((actor->animation->jump.jump_left.time + time_get()->delta) < actor->animation->settings.jump.jump_anim_length){
						t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case SPRINTING:
		{
			if (actor->animation->data.current != SPRINTING){
				actor->animation->data.previous = actor->animation->data.current;
				actor->animation->data.current = SPRINTING;
				t3d_anim_set_time(&actor->animation->standing_locomotion.sprinting, (actor->animation->standing_locomotion.running.time * actor->animation->settings.standing_locomotion.run_to_sprint_ratio));
			}

			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);
			actorAnimation_setFootingBlendingRatio(actor, actor->animation->standing_locomotion.walking.time, actor->animation->settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setSprintingSpeed(actor);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.sprinting, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->data.locomotion_blending_ratio);


			if (actor->animation->data.turning_blending_ratio != 0.0f){

				if (actor->animation->data.turning_blending_ratio < 0.0f){
					
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
				}
				else {
					
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
				}

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->animation->data.turning_blending_ratio));
			}				
			else {
					
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
			}

			if (actor->animation->jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->animation->data.jump_blending_ratio > 0){
					if ((actor->animation->jump.jump_left.time + time_get()->delta) < actor->animation->settings.jump.jump_anim_length){
						t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}
	}
}


void actorAnimation_setRolling(Entity *actor)
{
	ActorAnimationBuffer buffer;
	buffer.count = 0;

	if (actor->animation->data.current != ROLLING){

		actor->animation->data.previous = actor->animation->data.current;
		actor->animation->data.current = ROLLING;

		t3d_anim_set_playing(&actor->animation->roll.running_to_roll_left, true);
		t3d_anim_set_time(&actor->animation->roll.running_to_roll_left, 0.0f);

		t3d_anim_set_playing(&actor->animation->roll.running_to_roll_right, true);
		t3d_anim_set_time(&actor->animation->roll.running_to_roll_right, 0.0f);
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

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_WALK], actor->animation->data.locomotion_blending_ratio);

			if (actor->animation->data.turning_blending_ratio != 0.0f){
				if (actor->animation->data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->animation->data.turning_blending_ratio));
			}

			if (footing_phase >= 0.5f){
				t3d_anim_update(&actor->animation->roll.running_to_roll_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_ROLL_R], actor->animation->data.roll_blending_ratio);
			}
			else {
				t3d_anim_update(&actor->animation->roll.running_to_roll_left, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_ROLL_L], actor->animation->data.roll_blending_ratio);
			}

			if (actor->animation->jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->animation->data.jump_blending_ratio > 0){

					if ((actor->animation->jump.jump_left.time + time_get()->delta) < actor->animation->settings.jump.jump_anim_length){
						t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case SPRINTING:
		{
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setSprintingSpeed(actor);
			actorAnimation_setRollingBlendingRatio(actor);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.sprinting, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->data.locomotion_blending_ratio);

			if (actor->animation->data.turning_blending_ratio != 0.0f){
				if (actor->animation->data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->animation->data.turning_blending_ratio));
			}

			if (footing_phase >= 0.5f){
				t3d_anim_update(&actor->animation->roll.running_to_roll_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_ROLL_R], actor->animation->data.roll_blending_ratio);
			}
			else {
				t3d_anim_update(&actor->animation->roll.running_to_roll_left, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_ROLL_L], actor->animation->data.roll_blending_ratio);
			}

			if (actor->animation->jump.land_left.isPlaying){
				actorAnimation_setLandingBlendingRatio(actor);

				if (actor->animation->data.jump_blending_ratio > 0){
					if ((actor->animation->jump.jump_left.time + time_get()->delta) < actor->animation->settings.jump.jump_anim_length){
						t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
					}
					else {
						t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
						t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
					}

					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
					actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
				}

				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}
	}
}


void actorAnimation_setJumping(Entity *actor)
{
	ActorAnimationBuffer buffer;
	buffer.count = 0;

	if (actor->animation->data.current != JUMPING){

		actor->animation->data.previous = actor->animation->data.current;
		actor->animation->data.current = JUMPING;
		
		t3d_anim_set_playing(&actor->animation->jump.jump_left, true);
		t3d_anim_set_playing(&actor->animation->jump.jump_right, true);

		if (actor->animation->jump.land_left.isPlaying == false){

			actorAnimation_syncLandToJump(actor);
		}
		
		else {

			t3d_anim_set_time(&actor->animation->jump.jump_left, 0.0f);
			t3d_anim_set_time(&actor->animation->jump.jump_right, 0.0f);
			t3d_anim_set_time(&actor->animation->jump.falling_left, 0.0f);
			t3d_anim_set_time(&actor->animation->jump.falling_right, 0.0f);
			actor->animation->data.jump_blending_ratio = 0.0f;
		}
	}

	uint8_t speed_state = actorAnimation_getLocomotionSpeedState(actor);
	actorAnimation_setTurningBlendingRatio(actor);

	bool is_crouching = (actor->animation->jump.jump_left.time < actor->animation->settings.jump.jump_anim_crouch);
	bool is_jumping = ((actor->animation->jump.jump_left.time + time_get()->delta) < actor->animation->settings.jump.jump_anim_length);

	actorAnimation_setJumpBlendingRatio(actor);

	switch (speed_state)
	{
		case STANDING_IDLE:
		{
			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_right, time_get()->delta);

			if (is_jumping){
				t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
			}
			else {
				t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
			}

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);

			if (actor->animation->jump.land_left.isPlaying){
				
				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case WALKING:
		{
			actorAnimation_setFootingBlendingRatio(actor, actor->animation->standing_locomotion.walking.time, actor->animation->settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setWalkingSpeed(actor);
			actor->animation->standing_locomotion.walking.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_right, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_WALK], actor->animation->data.locomotion_blending_ratio);
			
			if (actor->animation->data.turning_blending_ratio != 0.0f){
				if (actor->animation->data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_WALK], fabsf(actor->animation->data.turning_blending_ratio));
			}
			
			if (is_jumping){
				t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
			}
			else {
				t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
			}

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);

			if (is_crouching && actor->animation->jump.land_left.isPlaying){
				
				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case RUNNING:
		{
			actorAnimation_setFootingBlendingRatio(actor, actor->animation->standing_locomotion.walking.time, actor->animation->settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setRunningSpeed(actor);
			actor->animation->standing_locomotion.walking.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);
			actor->animation->standing_locomotion.running.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_WALK], actor->animation->data.locomotion_blending_ratio);

			if (actor->animation->data.turning_blending_ratio != 0.0f){
				if (actor->animation->data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->animation->data.turning_blending_ratio));
			}

			if (is_jumping){
				t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
			}
			else {
				t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
			}

			if (is_crouching && actor->animation->jump.land_left.isPlaying){
				
				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case SPRINTING:
		{
			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);
			actorAnimation_setFootingBlendingRatio(actor, actor->animation->standing_locomotion.walking.time, actor->animation->settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setSprintingSpeed(actor);
			actor->animation->standing_locomotion.walking.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);
			actor->animation->standing_locomotion.running.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);
			actor->animation->standing_locomotion.sprinting.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.sprinting, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->data.locomotion_blending_ratio);

			if (actor->animation->data.turning_blending_ratio != 0.0f){
				if (actor->animation->data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->animation->data.turning_blending_ratio));
			}

			if (is_jumping){
				t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);
			}
			else {
				t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);
			}

			if (is_crouching && actor->animation->jump.land_left.isPlaying){
				
				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}
	}
}


void actorAnimation_setFalling(Entity *actor)
{
	ActorAnimationBuffer buffer;
	buffer.count = 0;

	if (actor->animation->data.current != FALLING){

		actor->animation->data.previous = actor->animation->data.current;
		actor->animation->data.current = FALLING;

		t3d_anim_set_playing(&actor->animation->jump.land_left, true);
		t3d_anim_set_playing(&actor->animation->jump.land_right, true);
		t3d_anim_set_time(&actor->animation->jump.land_left, 0.0f);
		t3d_anim_set_time(&actor->animation->jump.land_right, 0.0f);
		actor->animation->data.land_blending_ratio = 0.0f;
	}

	uint8_t speed_state = actorAnimation_getLocomotionSpeedState(actor);
	actorAnimation_setTurningBlendingRatio(actor);

	bool is_landing = (actor->transform.position.z < LAND_ANIM_STARTING_HEIGHT);
	bool is_jumping = ((actor->animation->jump.jump_left.time + time_get()->delta) < actor->animation->settings.jump.jump_anim_length);

	if (is_jumping) actorAnimation_setJumpBlendingRatio(actor);
	if (is_landing) actorAnimation_setLandingBlendingRatio(actor);

	switch (speed_state)
	{
		case STANDING_IDLE:
		{
			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_right, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);

			if (is_jumping){
				t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
			}
			else {
				t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
			}

			if (is_landing){
				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case WALKING:
		{
			actorAnimation_setFootingBlendingRatio(actor, actor->animation->standing_locomotion.walking.time, actor->animation->settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setWalkingSpeed(actor);
			actor->animation->standing_locomotion.walking.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_left, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.standing_idle_right, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->settings.standing_locomotion.action_idle_max_blending_ratio * footing_phase);
			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_WALK], actor->animation->data.locomotion_blending_ratio);
			
			if (actor->animation->data.turning_blending_ratio != 0.0f){
				if (actor->animation->data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->animation->standing_locomotion.walking_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_WALK], fabsf(actor->animation->data.turning_blending_ratio));
			}
			
			if (is_jumping){
				t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
			}
			else {
				t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
			}

			if (is_landing){
				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case RUNNING:
		{
			actorAnimation_setFootingBlendingRatio(actor, actor->animation->standing_locomotion.walking.time, actor->animation->settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setRunningSpeed(actor);
			actor->animation->standing_locomotion.walking.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);
			actor->animation->standing_locomotion.running.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_WALK], actor->animation->data.locomotion_blending_ratio);

			if (is_jumping){
				t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
			}
			else {
				t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
			}

			if (is_landing){
				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
			break;
		}

		case SPRINTING:
		{
			t3d_anim_update(&actor->animation->standing_locomotion.walking, time_get()->delta);
			actorAnimation_setFootingBlendingRatio(actor, actor->animation->standing_locomotion.walking.time, actor->animation->settings.standing_locomotion.walking_anim_length);
			actorAnimation_setLocomotionBlendingRatio(actor);
			actorAnimation_setSprintingSpeed(actor);
			actor->animation->standing_locomotion.walking.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);
			actor->animation->standing_locomotion.running.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);
			actor->animation->standing_locomotion.sprinting.speed *= actor->animation->settings.jump.jump_footing_speed * (1 - actor->animation->data.jump_blending_ratio);

			float footing_phase = actor->animation->data.footing_phase;

			t3d_anim_update(&actor->animation->standing_locomotion.running, time_get()->delta);
			t3d_anim_update(&actor->animation->standing_locomotion.sprinting, time_get()->delta);

			actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_IDLE_R], actor->animation->data.locomotion_blending_ratio);

			if (actor->animation->data.turning_blending_ratio != 0.0f){
				if (actor->animation->data.turning_blending_ratio < 0.0f)
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_left, time_get()->delta);
				else
					t3d_anim_update(&actor->animation->standing_locomotion.running_turn_right, time_get()->delta);
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_TURN_RUN], fabsf(actor->animation->data.turning_blending_ratio));
			}

			if (is_jumping){
				t3d_anim_update(&actor->animation->jump.jump_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.jump_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
			}
			else {
				t3d_anim_update(&actor->animation->jump.falling_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.falling_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_L], actor->animation->data.jump_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_JUMP_R], actor->animation->data.jump_blending_ratio * footing_phase);
			}

			if (is_landing){
				t3d_anim_update(&actor->animation->jump.land_left, time_get()->delta);
				t3d_anim_update(&actor->animation->jump.land_right, time_get()->delta);

				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_L], actor->animation->data.land_blending_ratio * fabsf(1.0f - footing_phase));
				actorAnimation_addLayer(&buffer, &actor->armature->buffer[ANIM_SLOT_LAND_R], actor->animation->data.land_blending_ratio * footing_phase);
			}

			actorAnimation_blendLayers(&actor->armature->main, &buffer);
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
    actorAnimation_handler[actor->state.current](actor);
    t3d_skeleton_update(&actor->armature->main);
}
