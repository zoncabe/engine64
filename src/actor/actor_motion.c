#include "../../include/time/time.h"
#include "../../include/actor/actor_states.h"
#include "../../include/actor/actor_motion.h"


static void actorMotion_setHorizontalAcceleration(Entity *entity, float target_speed, float acceleration_rate)
{
    entity->motion->data.target_velocity.x = target_speed * sinf(rad(entity->motion->input.target_yaw));
    entity->motion->data.target_velocity.y = target_speed * -cosf(rad(entity->motion->input.target_yaw));

    entity->physics->acceleration.x = acceleration_rate * (entity->motion->data.target_velocity.x - entity->physics->velocity.x);
    entity->physics->acceleration.y = acceleration_rate * (entity->motion->data.target_velocity.y - entity->physics->velocity.y);
}

static void actorMotion_setHorizontalInertiaAcceleration(Entity *entity, float target_speed, float acceleration_rate)
{
    entity->motion->data.target_velocity.x = target_speed * -sinf(rad(entity->transform.rotation.z));
    entity->motion->data.target_velocity.y = target_speed * -cosf(rad(entity->transform.rotation.z));

    entity->physics->acceleration.x = acceleration_rate * (entity->motion->data.target_velocity.x - entity->physics->velocity.x);
    entity->physics->acceleration.y = acceleration_rate * (entity->motion->data.target_velocity.y - entity->physics->velocity.y);
}

static void actorMotion_setStopingAcceleration(Entity *entity)
{
    entity->physics->acceleration.x = entity->motion->settings.idle_acceleration_rate * (0 - entity->physics->velocity.x);
    entity->physics->acceleration.y = entity->motion->settings.idle_acceleration_rate * (0 - entity->physics->velocity.y);
}

static void actorMotion_integrate(Entity *entity)
{
    entity->motion->data.previous_yaw = entity->transform.rotation.z;

    if (entity->physics->acceleration.x || entity->physics->acceleration.y || entity->physics->acceleration.z)
        vector3_addScaledVector(&entity->physics->velocity, &entity->physics->acceleration, time_get()->delta);

    if (fabs(entity->physics->velocity.x) < LOCOMOTION_MIN_SPEED && fabs(entity->physics->velocity.y) < LOCOMOTION_MIN_SPEED && fabs(entity->physics->velocity.z) == 0) {
        entity->physics->velocity.x = 0;
        entity->physics->velocity.y = 0;
        entity->motion->data.horizontal_speed = 0;
    }

    if (entity->physics->velocity.x != 0 || entity->physics->velocity.y != 0 || entity->physics->velocity.z != 0)
        vector3_addScaledVector(&entity->transform.position, &entity->physics->velocity, time_get()->delta);

    if (entity->physics->velocity.x != 0 || entity->physics->velocity.y != 0) {
        Vector2 horizontal_velocity = {entity->physics->velocity.x, entity->physics->velocity.y};
        entity->motion->data.horizontal_speed = vector2_magnitude(&horizontal_velocity);

        float target_yaw = angle_wrap_relative(deg(atan2(-entity->physics->velocity.x, -entity->physics->velocity.y)), entity->transform.rotation.z);

        if (entity->state.current == ROLLING)
            entity->transform.rotation.z = angle_wrap(deg(atan2(-entity->physics->velocity.x, -entity->physics->velocity.y)));
        else if (fabsf(target_yaw - entity->transform.rotation.z) > ACTOR_ROTATION_SNAP_THRESHOLD)
            entity->transform.rotation.z = angle_wrap(lerpf(entity->transform.rotation.z, target_yaw, ACTOR_ROTATION_LERP_FACTOR * entity->motion->data.horizontal_speed / entity->motion->settings.sprint_target_speed));
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
    actorMotion_setHorizontalAcceleration(entity, entity->motion->settings.walk_target_speed, entity->motion->settings.walk_acceleration_rate);
}

static void actorMotion_setRunning(Entity *entity)
{
    actorMotion_setHorizontalAcceleration(entity, entity->motion->settings.run_target_speed, entity->motion->settings.run_acceleration_rate);
}

static void actorMotion_setSprinting(Entity *entity)
{
    actorMotion_setHorizontalAcceleration(entity, entity->motion->settings.sprint_target_speed, entity->motion->settings.sprint_acceleration_rate);
}

static void actorMotion_setRolling(Entity *entity)
{
    if (entity->motion->data.roll_timer < entity->motion->settings.roll_change_grip_time) {
        actorMotion_setHorizontalInertiaAcceleration(entity, entity->motion->data.horizontal_speed, entity->motion->settings.run_acceleration_rate);
        entity->motion->data.roll_timer += time_get()->delta;
    } else if (entity->motion->data.roll_timer < entity->motion->settings.roll_timer_max) {
        actorMotion_setHorizontalAcceleration(entity, entity->motion->data.horizontal_speed, entity->motion->settings.roll_acceleration_grip_rate);
        entity->motion->data.roll_timer += time_get()->delta;
    } else {
        actor_setState(&entity->state, entity->state.locomotion);
        entity->motion->data.roll_timer = 0;
    }
}

static void actorMotion_setJump(Entity *entity)
{
    if (entity->motion->input.jump_triggered) {
        entity->motion->data.jump_initial_velocity = entity->physics->velocity;
        entity->motion->input.jump_triggered = false;
    }

    actorMotion_setHorizontalAcceleration(entity, entity->motion->data.horizontal_speed, entity->motion->settings.aerial_control_rate);

    if (entity->motion->data.jump_timer < entity->motion->settings.jump_timer_max) {
        entity->motion->data.jump_timer += time_get()->delta;
        if (entity->motion->input.jump_hold) {
            entity->motion->data.jump_force += time_get()->delta;
            vector3_scale(&entity->physics->velocity, ACTOR_JUMP_HOLD_VELOCITY_SCALE);
        }
    } else if (entity->motion->data.jump_force > 0) {
        entity->motion->data.jump_timer += time_get()->delta;
        entity->physics->velocity = entity->motion->data.jump_initial_velocity;
        vector3_scale(&entity->physics->velocity, ACTOR_JUMP_LAUNCH_VELOCITY_SCALE);
        entity->physics->velocity.z = entity->motion->data.jump_force * entity->motion->settings.jump_force_multiplier;
        if (entity->physics->velocity.z < entity->motion->settings.jump_minimum_speed)
            entity->physics->velocity.z = entity->motion->settings.jump_minimum_speed;
        entity->motion->data.jump_force = 0;
    } else if (entity->physics->velocity.z > 0) {
        entity->motion->data.jump_timer += time_get()->delta;
        entity->physics->acceleration.z = entity->motion->settings.gravity;
    } else {
        entity->physics->acceleration.z = entity->motion->settings.gravity;
        entity->motion->data.jump_timer = 0;
        actor_setState(&entity->state, FALLING);
        return;
    }
}

static void actorMotion_setFalling(Entity *entity)
{
    entity->motion->data.grounded = 0;
    actorMotion_setHorizontalAcceleration(entity, entity->motion->data.horizontal_speed, entity->motion->settings.aerial_control_rate);
    entity->physics->acceleration.z = entity->motion->settings.gravity;
    if (entity->physics->velocity.z > entity->motion->settings.fall_max_speed)
        entity->physics->velocity.z = entity->motion->settings.fall_max_speed;

    if (entity->transform.position.z <= entity->motion->data.grounding_height + ACTOR_GROUNDING_SNAP_ZONE) {
        entity->motion->data.grounded = 1;
        entity->physics->acceleration.z = 0;
        entity->physics->velocity.z = 0;
        entity->transform.position.z = entity->motion->data.grounding_height;
        actor_setState(&entity->state, entity->state.locomotion);
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
    actorMotion_handler[entity->state.current](entity);
    actorMotion_integrate(entity);
}
