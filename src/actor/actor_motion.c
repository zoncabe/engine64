#include "../../include/time/time.h"
#include "../../include/actor/actor_states.h"
#include "../../include/actor/actor_motion.h"


static void actorMotion_setHorizontalAcceleration(Entity *entity, float target_speed, float acceleration_rate)
{
    entity->actor->motion.data.target_velocity.x = target_speed * sinf(rad(entity->actor->motion.input.target_yaw));
    entity->actor->motion.data.target_velocity.y = target_speed * -cosf(rad(entity->actor->motion.input.target_yaw));

    entity->actor->body.acceleration.x = acceleration_rate * (entity->actor->motion.data.target_velocity.x - entity->actor->body.velocity.x);
    entity->actor->body.acceleration.y = acceleration_rate * (entity->actor->motion.data.target_velocity.y - entity->actor->body.velocity.y);
}

static void actorMotion_setHorizontalInertiaAcceleration(Entity *entity, float target_speed, float acceleration_rate)
{
    entity->actor->motion.data.target_velocity.x = target_speed * -sinf(rad(entity->transform.rotation.z));
    entity->actor->motion.data.target_velocity.y = target_speed * -cosf(rad(entity->transform.rotation.z));

    entity->actor->body.acceleration.x = acceleration_rate * (entity->actor->motion.data.target_velocity.x - entity->actor->body.velocity.x);
    entity->actor->body.acceleration.y = acceleration_rate * (entity->actor->motion.data.target_velocity.y - entity->actor->body.velocity.y);
}

static void actorMotion_setStopingAcceleration(Entity *entity)
{
    entity->actor->body.acceleration.x = entity->actor->motion.settings.idle_acceleration_rate * (0 - entity->actor->body.velocity.x);
    entity->actor->body.acceleration.y = entity->actor->motion.settings.idle_acceleration_rate * (0 - entity->actor->body.velocity.y);
}

static void actorMotion_integrate(Entity *entity)
{
    entity->actor->motion.data.previous_yaw = entity->transform.rotation.z;

    if (entity->actor->body.acceleration.x || entity->actor->body.acceleration.y || entity->actor->body.acceleration.z)
        vector3_addScaledVector(&entity->actor->body.velocity, &entity->actor->body.acceleration, time_get()->delta);

    if (fabs(entity->actor->body.velocity.x) < LOCOMOTION_MIN_SPEED && fabs(entity->actor->body.velocity.y) < LOCOMOTION_MIN_SPEED && fabs(entity->actor->body.velocity.z) == 0) {
        entity->actor->body.velocity.x = 0;
        entity->actor->body.velocity.y = 0;
        entity->actor->motion.data.horizontal_speed = 0;
    }

    if (entity->actor->body.velocity.x != 0 || entity->actor->body.velocity.y != 0 || entity->actor->body.velocity.z != 0)
        vector3_addScaledVector(&entity->transform.position, &entity->actor->body.velocity, time_get()->delta);

    if (entity->actor->body.velocity.x != 0 || entity->actor->body.velocity.y != 0) {
        Vector2 horizontal_velocity = {entity->actor->body.velocity.x, entity->actor->body.velocity.y};
        entity->actor->motion.data.horizontal_speed = vector2_magnitude(&horizontal_velocity);

        float target_yaw = angle_wrap_relative(deg(atan2(-entity->actor->body.velocity.x, -entity->actor->body.velocity.y)), entity->transform.rotation.z);

        if (entity->actor->state.current == ROLLING)
            entity->transform.rotation.z = angle_wrap(deg(atan2(-entity->actor->body.velocity.x, -entity->actor->body.velocity.y)));
        else if (fabsf(target_yaw - entity->transform.rotation.z) > ACTOR_ROTATION_SNAP_THRESHOLD)
            entity->transform.rotation.z = angle_wrap(lerpf(entity->transform.rotation.z, target_yaw, ACTOR_ROTATION_LERP_FACTOR * entity->actor->motion.data.horizontal_speed / entity->actor->motion.settings.sprint_target_speed));
        else
            entity->transform.rotation.z = target_yaw;
    }
}

static void actorMotion_setIdle(Entity *entity)
{
    actorMotion_setStopingAcceleration(entity);
}

static void actorMotion_setWalking(Entity *entity)
{
    actorMotion_setHorizontalAcceleration(entity, entity->actor->motion.settings.walk_target_speed, entity->actor->motion.settings.walk_acceleration_rate);
}

static void actorMotion_setRunning(Entity *entity)
{
    actorMotion_setHorizontalAcceleration(entity, entity->actor->motion.settings.run_target_speed, entity->actor->motion.settings.run_acceleration_rate);
}

static void actorMotion_setSprinting(Entity *entity)
{
    actorMotion_setHorizontalAcceleration(entity, entity->actor->motion.settings.sprint_target_speed, entity->actor->motion.settings.sprint_acceleration_rate);
}

static void actorMotion_setRolling(Entity *entity)
{
    if (entity->actor->motion.data.roll_timer < entity->actor->motion.settings.roll_change_grip_time) {
        actorMotion_setHorizontalInertiaAcceleration(entity, entity->actor->motion.data.horizontal_speed, entity->actor->motion.settings.run_acceleration_rate);
        entity->actor->motion.data.roll_timer += time_get()->delta;
    } else if (entity->actor->motion.data.roll_timer < entity->actor->motion.settings.roll_timer_max) {
        actorMotion_setHorizontalAcceleration(entity, entity->actor->motion.data.horizontal_speed, entity->actor->motion.settings.roll_acceleration_grip_rate);
        entity->actor->motion.data.roll_timer += time_get()->delta;
    } else {
        actor_setState(&entity->actor->state, entity->actor->state.locomotion);
        entity->actor->motion.data.roll_timer = 0;
    }
}

static void actorMotion_setJump(Entity *entity)
{
    if (entity->actor->motion.input.jump_triggered) {
        entity->actor->motion.data.jump_initial_velocity = entity->actor->body.velocity;
        entity->actor->motion.input.jump_triggered = false;
    }

    actorMotion_setHorizontalAcceleration(entity, entity->actor->motion.data.horizontal_speed, entity->actor->motion.settings.aerial_control_rate);

    if (entity->actor->motion.data.jump_timer < entity->actor->motion.settings.jump_timer_max) {
        entity->actor->motion.data.jump_timer += time_get()->delta;
        if (entity->actor->motion.input.jump_hold) {
            entity->actor->motion.data.jump_force += time_get()->delta;
            vector3_scale(&entity->actor->body.velocity, ACTOR_JUMP_HOLD_VELOCITY_SCALE);
        }
    } else if (entity->actor->motion.data.jump_force > 0) {
        entity->actor->motion.data.jump_timer += time_get()->delta;
        entity->actor->body.velocity = entity->actor->motion.data.jump_initial_velocity;
        vector3_scale(&entity->actor->body.velocity, ACTOR_JUMP_LAUNCH_VELOCITY_SCALE);
        entity->actor->body.velocity.z = entity->actor->motion.data.jump_force * entity->actor->motion.settings.jump_force_multiplier;
        if (entity->actor->body.velocity.z < entity->actor->motion.settings.jump_minimum_speed)
            entity->actor->body.velocity.z = entity->actor->motion.settings.jump_minimum_speed;
        entity->actor->motion.data.jump_force = 0;
    } else if (entity->actor->body.velocity.z > 0) {
        entity->actor->motion.data.jump_timer += time_get()->delta;
        entity->actor->body.acceleration.z = entity->actor->motion.settings.gravity;
    } else {
        entity->actor->body.acceleration.z = entity->actor->motion.settings.gravity;
        entity->actor->motion.data.jump_timer = 0;
        actor_setState(&entity->actor->state, FALLING);
        return;
    }
}

static void actorMotion_setFalling(Entity *entity)
{
    entity->actor->motion.data.grounded = 0;
    actorMotion_setHorizontalAcceleration(entity, entity->actor->motion.data.horizontal_speed, entity->actor->motion.settings.aerial_control_rate);
    entity->actor->body.acceleration.z = entity->actor->motion.settings.gravity;
    if (entity->actor->body.velocity.z > entity->actor->motion.settings.fall_max_speed)
        entity->actor->body.velocity.z = entity->actor->motion.settings.fall_max_speed;

    if (entity->transform.position.z <= entity->actor->motion.data.grounding_height + ACTOR_GROUNDING_SNAP_ZONE) {
        entity->actor->motion.data.grounded = 1;
        entity->actor->body.acceleration.z = 0;
        entity->actor->body.velocity.z = 0;
        entity->transform.position.z = entity->actor->motion.data.grounding_height;
        actor_setState(&entity->actor->state, entity->actor->state.locomotion);
        return;
    }
}

static void (*actorMotion_handler[])(Entity *) = {
    [STANDING_IDLE] = actorMotion_setIdle,
    [WALKING]       = actorMotion_setWalking,
    [RUNNING]       = actorMotion_setRunning,
    [SPRINTING]     = actorMotion_setSprinting,
    [ROLLING]       = actorMotion_setRolling,
    [JUMPING]       = actorMotion_setJump,
    [FALLING]       = actorMotion_setFalling,
};

void actorMotion_update(Entity *entity)
{
    actorMotion_handler[entity->actor->state.current](entity);
    actorMotion_integrate(entity);
}
