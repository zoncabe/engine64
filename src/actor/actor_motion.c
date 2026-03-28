#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/actor/actor.h"
#include "../../include/actor/actor_states.h"
#include "../../include/actor/actor_motion.h"


void actorMotion_setHorizontalAcceleration(Actor* actor, float target_speed, float acceleration_rate)
{
    actor->motion.data.target_velocity.x = target_speed * sinf(rad(actor->motion.data.target_yaw));
    actor->motion.data.target_velocity.y = target_speed * -cosf(rad(actor->motion.data.target_yaw));

    actor->body.acceleration.x = acceleration_rate * (actor->motion.data.target_velocity.x - actor->body.velocity.x);
    actor->body.acceleration.y = acceleration_rate * (actor->motion.data.target_velocity.y - actor->body.velocity.y);
}

void actorMotion_setHorizontalInertiaAcceleration(Actor* actor, float target_speed, float acceleration_rate)
{
    actor->motion.data.target_velocity.x = target_speed * -sinf(rad(actor->body.rotation.z));
    actor->motion.data.target_velocity.y = target_speed * -cosf(rad(actor->body.rotation.z));

    actor->body.acceleration.x = acceleration_rate * (actor->motion.data.target_velocity.x - actor->body.velocity.x);
    actor->body.acceleration.y = acceleration_rate * (actor->motion.data.target_velocity.y - actor->body.velocity.y);
}

void actorMotion_setStopingAcceleration(Actor* actor)
{
    actor->body.acceleration.x = actor->motion.settings.idle_acceleration_rate * (0 - actor->body.velocity.x);
    actor->body.acceleration.y = actor->motion.settings.idle_acceleration_rate * (0 - actor->body.velocity.y);
}

void actorMotion_setJumpAcceleration(Actor* actor, float target_speed, float acceleration_rate)
{
    actor->body.acceleration.z = acceleration_rate * (target_speed - actor->body.velocity.z);
}

void actorMotion_integrate(Actor* actor)
{
    if (actor->body.acceleration.x != 0 || actor->body.acceleration.y != 0 || actor->body.acceleration.z != 0) vector3_addScaledVector(&actor->body.velocity, &actor->body.acceleration, timer.delta);

	if (fabs(actor->body.velocity.x) < LOCOMOTION_MIN_SPEED && fabs(actor->body.velocity.y) < LOCOMOTION_MIN_SPEED && fabs(actor->body.velocity.z) == 0){
		actor->body.velocity.x = 0;
		actor->body.velocity.y = 0;
        actor->motion.data.horizontal_speed = 0;
	}

    if (actor->body.velocity.x != 0 || actor->body.velocity.y != 0 || actor->body.velocity.z != 0) vector3_addScaledVector(&actor->body.position, &actor->body.velocity, timer.delta);

    if (actor->body.velocity.x != 0 || actor->body.velocity.y != 0) {

        Vector2 horizontal_velocity = {actor->body.velocity.x, actor->body.velocity.y};
        actor->motion.data.horizontal_speed = vector2_magnitude(&horizontal_velocity);

        float target_yaw = deg(atan2(-actor->body.velocity.x, -actor->body.velocity.y));

        if (target_yaw > actor->body.rotation.z + 180) target_yaw -= 360;
        if (target_yaw < actor->body.rotation.z - 180) target_yaw += 360;

        if(actor->state.current == ROLLING) actor->body.rotation.z = deg(atan2(-actor->body.velocity.x, -actor->body.velocity.y));
        
        else if (target_yaw < actor->body.rotation.z - 1 || target_yaw > actor->body.rotation.z + 1) actor->body.rotation.z = lerpf(actor->body.rotation.z, target_yaw, actor->motion.data.horizontal_speed / actor->motion.settings.sprint_target_speed);
        else actor->body.rotation.z = target_yaw;

        if (actor->body.rotation.z > 180) actor->body.rotation.z -= 360;
        if (actor->body.rotation.z < -180) actor->body.rotation.z += 360;
	}
}

void actorMotion_setIdle(Actor* actor)
{
    actorMotion_setStopingAcceleration(actor);
}

void actorMotion_setWalking(Actor* actor)
{
    actorMotion_setHorizontalAcceleration(actor, actor->motion.settings.walk_target_speed, actor->motion.settings.walk_acceleration_rate);
}

void actorMotion_setRunning(Actor* actor)
{    
    actorMotion_setHorizontalAcceleration(actor, actor->motion.settings.run_target_speed, actor->motion.settings.run_acceleration_rate);
}

void actorMotion_setSprinting(Actor* actor)
{
    actorMotion_setHorizontalAcceleration(actor, actor->motion.settings.sprint_target_speed, actor->motion.settings.sprint_acceleration_rate);
}

void actorMotion_setRolling(Actor* actor)
{
    if (actor->motion.data.roll_timer < actor->motion.settings.roll_change_grip_time){

        actorMotion_setHorizontalInertiaAcceleration(actor, actor->motion.data.horizontal_speed, actor->motion.settings.run_acceleration_rate);
        actor->motion.data.roll_timer += timer.delta;
    }

    else if (actor->motion.data.roll_timer < actor->motion.settings.roll_timer_max){ 
        
        actorMotion_setHorizontalAcceleration(actor, actor->motion.data.horizontal_speed, actor->motion.settings.roll_acceleration_grip_rate);
        actor->motion.data.roll_timer += timer.delta;
    }
    
    else {
        actor_setState(&actor->state, actor->state.locomotion);
        actor->motion.data.roll_timer = 0;
    }
}

void actorMotion_setJump(Actor* actor)
{
    actorMotion_setHorizontalAcceleration(actor, actor->motion.data.horizontal_speed, actor->motion.settings.aerial_control_rate);
    
    if (actor->motion.data.jump_timer < actor->motion.settings.jump_timer_max){
        
        actor->motion.data.jump_timer += timer.delta;
        
        if(actor->motion.input.jump_hold){
            
            actor->motion.data.jump_force += timer.delta;
            vector3_scale(&actor->body.velocity, 0.96f);
        } 
    }

    else if (actor->motion.data.jump_force > 0){
        
        actor->motion.data.jump_timer += timer.delta;

        actor->body.velocity = actor->motion.data.jump_initial_velocity;
        vector3_scale(&actor->body.velocity, 0.8f);

        actor->body.velocity.z = actor->motion.data.jump_force * actor->motion.settings.jump_force_multiplier;
        if (actor->body.velocity.z < actor->motion.settings.jump_minimum_speed) actor->body.velocity.z = actor->motion.settings.jump_minimum_speed;

        actor->motion.data.jump_force = 0;
    }

    else if (actor->body.velocity.z > 0){

        actor->motion.data.jump_timer += timer.delta;
                
        actor->body.acceleration.z = actor->motion.settings.gravity;
    }
    
    else {
        
        actor->body.acceleration.z = actor->motion.settings.gravity;
        actor->motion.data.jump_timer = 0;
        
        actor_setState(&actor->state, FALLING);
        return;
    }
}

void actorMotion_setFalling(Actor* actor)
{
    actor->motion.data.grounded = 0;
    actorMotion_setHorizontalAcceleration(actor, actor->motion.data.horizontal_speed, actor->motion.settings.aerial_control_rate);
    actor->body.acceleration.z = actor->motion.settings.gravity;
    if (actor->body.velocity.z > actor->motion.settings.fall_max_speed) actor->body.velocity.z = actor->motion.settings.fall_max_speed;

    if (actor->body.position.z <= actor->motion.data.grounding_height + 10) {

        actor->motion.data.grounded = 1;
        actor->body.acceleration.z = 0;
        actor->body.velocity.z = 0;
        actor->body.position.z = actor->motion.data.grounding_height;

        actor_setState(&actor->state, actor->state.locomotion);

        return;
    }
}


void actor_setMotion(Actor* actor)
{
   switch (actor->state.current) {
    
        case STANDING_IDLE: {
            actorMotion_setIdle(actor);
            break;
        }
        case WALKING: {
            actorMotion_setWalking(actor);
            break;
        }
        case RUNNING: {
            actorMotion_setRunning(actor);
            break;
        }
        case SPRINTING: {
            actorMotion_setSprinting(actor);
            break;
        }
        case ROLLING: {
            actorMotion_setRolling(actor);
            break;
        }
        case JUMPING: {
            actorMotion_setJump(actor);
            break;
        }
        case FALLING: {
            actorMotion_setFalling(actor);
            break;
        }
    }

	actorMotion_integrate(actor);
}
