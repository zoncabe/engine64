#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/actor/actor.h"
#include "../../include/actor/actor_states.h"
#include "../../include/actor/actor_motion.h"


void actorMotion_setHorizontalAcceleration(RigidBody* body, ActorMotionData* motion_data, float target_speed, float acceleration_rate)
{
    motion_data->target_velocity.x = target_speed * sinf(rad(motion_data->target_yaw));
    motion_data->target_velocity.y = target_speed * -cosf(rad(motion_data->target_yaw));

    body->acceleration.x = acceleration_rate * (motion_data->target_velocity.x - body->velocity.x);
    body->acceleration.y = acceleration_rate * (motion_data->target_velocity.y - body->velocity.y);
}

void actorMotion_setHorizontalInertiaAcceleration(RigidBody* body, ActorMotionData* motion_data, float target_speed, float acceleration_rate)
{
    motion_data->target_velocity.x = target_speed * -sinf(rad(body->rotation.z));
    motion_data->target_velocity.y = target_speed * -cosf(rad(body->rotation.z));

    body->acceleration.x = acceleration_rate * (motion_data->target_velocity.x - body->velocity.x);
    body->acceleration.y = acceleration_rate * (motion_data->target_velocity.y - body->velocity.y);
}

void actorMotion_setStopingAcceleration (RigidBody* body, ActorMotionSettings* motion_settings)
{
    body->acceleration.x = motion_settings->idle_acceleration_rate * (0 - body->velocity.x);
    body->acceleration.y = motion_settings->idle_acceleration_rate * (0 - body->velocity.y);
}

void actorMotion_setJumpAcceleration(RigidBody* body, float target_speed, float acceleration_rate)
{
    body->acceleration.z = acceleration_rate * (target_speed - body->velocity.z);
}

void actorMotion_integrate (RigidBody* body, ActorMotionData* motion_data, ActorMotionSettings* motion_settings, ActorStateData* state)
{
    if (body->acceleration.x != 0 || body->acceleration.y != 0 || body->acceleration.z != 0) vector3_addScaledVector(&body->velocity, &body->acceleration, timer.delta);

	if (fabs(body->velocity.x) < LOCOMOTION_MIN_SPEED && fabs(body->velocity.y) < LOCOMOTION_MIN_SPEED && fabs(body->velocity.z) == 0){
		body->velocity.x = 0;
		body->velocity.y = 0;
        motion_data->horizontal_speed = 0;
	}

    if (body->velocity.x != 0 || body->velocity.y != 0 || body->velocity.z != 0) vector3_addScaledVector(&body->position, &body->velocity, timer.delta);

    if (body->velocity.x != 0 || body->velocity.y != 0) {

        Vector2 horizontal_velocity = {body->velocity.x, body->velocity.y};
        motion_data->horizontal_speed = vector2_magnitude(&horizontal_velocity);

        float target_yaw = deg(atan2(-body->velocity.x, -body->velocity.y));

        if (target_yaw > body->rotation.z + 180) target_yaw -= 360;
        if (target_yaw < body->rotation.z - 180) target_yaw += 360;


        if(state->current == ROLLING) body->rotation.z = deg(atan2(-body->velocity.x, -body->velocity.y));
        
        else if (target_yaw < body->rotation.z - 1 || target_yaw > body->rotation.z + 1) body->rotation.z = lerpf(body->rotation.z, target_yaw, motion_data->horizontal_speed / motion_settings->sprint_target_speed);
        else body->rotation.z = target_yaw;

        if (body->rotation.z > 180) body->rotation.z -= 360;
        if (body->rotation.z < -180) body->rotation.z += 360;
	}
}

void actorMotion_setIdle(RigidBody* body, ActorMotionData* motion_data, ActorMotionSettings* motion_settings)
{
    actorMotion_setStopingAcceleration(body, motion_settings);
}

void actorMotion_setWalking(RigidBody* body, ActorMotionData* motion_data, ActorMotionSettings* motion_settings)
{
    actorMotion_setHorizontalAcceleration (body, motion_data, motion_settings->walk_target_speed, motion_settings->walk_acceleration_rate);
}

void actorMotion_setRunning(RigidBody* body, ActorMotionData* motion_data, ActorMotionSettings* motion_settings)
{    
    actorMotion_setHorizontalAcceleration (body, motion_data, motion_settings->run_target_speed, motion_settings->run_acceleration_rate);
}

void actorMotion_setSprinting(RigidBody* body, ActorMotionData* motion_data, ActorMotionSettings* motion_settings)
{
    actorMotion_setHorizontalAcceleration (body, motion_data, motion_settings->sprint_target_speed, motion_settings->sprint_acceleration_rate);
}

void actorMotion_setRolling(RigidBody* body, ActorMotionData* motion_data, ActorMotionSettings* motion_settings, ActorStateData* state)
{
    if (motion_data->roll_timer < motion_settings->roll_change_grip_time){

        actorMotion_setHorizontalInertiaAcceleration (body, motion_data, motion_data->horizontal_speed, motion_settings->run_acceleration_rate);
        motion_data->roll_timer += timer.delta;
    }

    else if (motion_data->roll_timer < motion_settings->roll_timer_max){ 
        
        actorMotion_setHorizontalAcceleration (body, motion_data, motion_data->horizontal_speed, motion_settings->roll_acceleration_grip_rate);
        motion_data->roll_timer += timer.delta;
    }
    
    else {
        actor_setState(state, state->locomotion);
        motion_data->roll_timer = 0;
    }
}

void actorMotion_setJump(RigidBody* body, ActorMotionData* motion_data, ActorMotionInput* motion_input, ActorMotionSettings* motion_settings, ActorStateData* state)
{
    actorMotion_setHorizontalAcceleration (body, motion_data, motion_data->horizontal_speed, motion_settings->aerial_control_rate);
    
    if (motion_data->jump_timer < motion_settings->jump_timer_max){
        
        motion_data->jump_timer += timer.delta;
        
        if(motion_input->jump_hold){
            
            motion_data->jump_force += timer.delta;
            vector3_scale(&body->velocity, 0.96f);
        } 
    }

    else if (motion_data->jump_force > 0){
        
        motion_data->jump_timer += timer.delta;

        body->velocity = motion_data->jump_initial_velocity;
        vector3_scale(&body->velocity, 0.8f);

        body->velocity.z = motion_data->jump_force * motion_settings->jump_force_multiplier;
        if (body->velocity.z < motion_settings->jump_minimum_speed) body->velocity.z = motion_settings->jump_minimum_speed;

        motion_data->jump_force = 0;
    }

    else if (body->velocity.z > 0){

        motion_data->jump_timer += timer.delta;
                
        body->acceleration.z = motion_settings->gravity;
    }
    
    else {
        
        body->acceleration.z = motion_settings->gravity;
        motion_data->jump_timer = 0;
        
        actor_setState(state, FALLING);
        return;
    }
}

void actorMotion_setFalling(RigidBody* body, ActorMotionData* motion_data, ActorMotionSettings* motion_settings, ActorStateData* state)
{
    motion_data->grounded = 0;
    actorMotion_setHorizontalAcceleration (body, motion_data, motion_data->horizontal_speed, motion_settings->aerial_control_rate);
    body->acceleration.z = motion_settings->gravity;
    if (body->velocity.z > motion_settings->fall_max_speed) body->velocity.z = motion_settings->fall_max_speed;

    if (body->position.z <= motion_data->grounding_height + 10) {

        motion_data->grounded = 1;
        body->acceleration.z = 0;
        body->velocity.z = 0;
        body->position.z = motion_data->grounding_height;

        actor_setState(state, state->locomotion);

        return;
    }
}


void actor_setMotion(Actor* actor)
{
   switch (actor->state.current) {
    
        case STANDING_IDLE: {
            actorMotion_setIdle(&actor->body, &actor->motion.data, &actor->motion.settings);
            break;
        }
        case WALKING: {
            actorMotion_setWalking(&actor->body, &actor->motion.data, &actor->motion.settings);
            break;
        }
        case RUNNING: {
            actorMotion_setRunning(&actor->body, &actor->motion.data, &actor->motion.settings);
            break;
        }
        case SPRINTING: {
            actorMotion_setSprinting(&actor->body, &actor->motion.data, &actor->motion.settings);
            break;
        }
        case ROLLING: {
            actorMotion_setRolling(&actor->body, &actor->motion.data, &actor->motion.settings, &actor->state);
            break;
        }
        case JUMPING: {
            actorMotion_setJump(&actor->body, &actor->motion.data, &actor->motion.input, &actor->motion.settings, &actor->state);
            break;
        }
        case FALLING: {
            actorMotion_setFalling(&actor->body, &actor->motion.data, &actor->motion.settings, &actor->state);
            break;
        }
    }

	actorMotion_integrate(&actor->body, &actor->motion.data, &actor->motion.settings, &actor->state);
}
