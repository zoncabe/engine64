#include <assert.h>

#include "../../include/entity/entity.h"


static void actorMotion_setHorizontalAcceleration(Entity *entity, const MotionCommand *cmd, float target_speed, float acceleration_rate)
{
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    ActorMotionData *d = &a->motion.data;

    d->target_velocity.x = target_speed *  sinf(rad(cmd->target_yaw));
    d->target_velocity.y = target_speed * -cosf(rad(cmd->target_yaw));

    b->acceleration.x = acceleration_rate * (d->target_velocity.x - b->velocity.x);
    b->acceleration.y = acceleration_rate * (d->target_velocity.y - b->velocity.y);
}

static void actorMotion_setHorizontalInertiaAcceleration(Entity *entity, float target_speed, float acceleration_rate)
{
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    ActorMotionData *d = &a->motion.data;
    const float yaw = entity->transform.rotation.z;

    d->target_velocity.x = target_speed * -sinf(rad(yaw));
    d->target_velocity.y = target_speed * -cosf(rad(yaw));

    b->acceleration.x = acceleration_rate * (d->target_velocity.x - b->velocity.x);
    b->acceleration.y = acceleration_rate * (d->target_velocity.y - b->velocity.y);
}

static void actorMotion_setStopingAcceleration(Entity *entity)
{
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    const float k = a->motion.settings.idle_acceleration_rate;

    b->acceleration.x = k * (0 - b->velocity.x);
    b->acceleration.y = k * (0 - b->velocity.y);
}

static void actorMotion_setRotation(Entity *entity)
{
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    ActorMotionData *d = &a->motion.data;
    const ActorMotionSettings *s = &a->motion.settings;

    if (b->velocity.x == 0 && b->velocity.y == 0) return;

    Vector2 horizontal_velocity = {b->velocity.x, b->velocity.y};
    d->horizontal_speed = vector2_magnitude(&horizontal_velocity);

    const float velocity_yaw = deg(atan2(-b->velocity.x, -b->velocity.y));

    if (d->rotation_mode == ACTOR_ROTATION_MODE_SNAP) {
        entity->transform.rotation.z = angle_wrap(velocity_yaw);
        return;
    }

    const float target_yaw = angle_wrap_relative(velocity_yaw, entity->transform.rotation.z);

    if (fabsf(target_yaw - entity->transform.rotation.z) > ACTOR_ROTATION_SNAP_THRESHOLD)
        entity->transform.rotation.z = angle_wrap(lerpf(entity->transform.rotation.z, target_yaw, ACTOR_ROTATION_LERP_FACTOR * d->horizontal_speed / s->sprint_target_speed));
    else
        entity->transform.rotation.z = target_yaw;
}

static void actorMotion_integrate(Entity *entity, float dt)
{
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    ActorMotionData *d = &a->motion.data;

    d->previous_yaw = entity->transform.rotation.z;

    if (b->acceleration.x || b->acceleration.y || b->acceleration.z)
        vector3_addScaledVector(&b->velocity, &b->acceleration, dt);

    if (fabsf(b->velocity.x) < LOCOMOTION_MIN_SPEED && fabsf(b->velocity.y) < LOCOMOTION_MIN_SPEED && fabsf(b->velocity.z) == 0) {
        b->velocity.x = 0;
        b->velocity.y = 0;
        d->horizontal_speed = 0;
    }

    if (b->velocity.x != 0 || b->velocity.y != 0 || b->velocity.z != 0)
        vector3_addScaledVector(&entity->transform.position, &b->velocity, dt);

    actorMotion_setRotation(entity);
}

static void actorMotion_setIdle(Entity *entity, MotionCommand *cmd, float dt)
{
    (void)cmd; (void)dt;
    actorMotion_setStopingAcceleration(entity);
}

static void actorMotion_setWalking(Entity *entity, MotionCommand *cmd, float dt)
{
    (void)dt;
    const ActorMotionSettings *s = &entity->actor->motion.settings;
    actorMotion_setHorizontalAcceleration(entity, cmd, s->walk_target_speed, s->walk_acceleration_rate);
}

static void actorMotion_setRunning(Entity *entity, MotionCommand *cmd, float dt)
{
    (void)dt;
    const ActorMotionSettings *s = &entity->actor->motion.settings;
    actorMotion_setHorizontalAcceleration(entity, cmd, s->run_target_speed, s->run_acceleration_rate);
}

static void actorMotion_setSprinting(Entity *entity, MotionCommand *cmd, float dt)
{
    (void)dt;
    const ActorMotionSettings *s = &entity->actor->motion.settings;
    actorMotion_setHorizontalAcceleration(entity, cmd, s->sprint_target_speed, s->sprint_acceleration_rate);
}

static RollParams actorMotion_getRollParams(uint8_t locomotion, const ActorMotionSettings *s)
{
    RollParams p;
    bool stand    = (locomotion == STANDING_IDLE);
    p.ground_time = stand ? s->stand_roll_ground_time      : s->roll_ground_time;
    p.grip_time   = stand ? s->stand_roll_change_grip_time : s->roll_change_grip_time;
    p.timer_max   = stand ? s->stand_roll_timer_max        : s->roll_timer_max;

    switch (locomotion) {
        case SPRINTING: p.target_speed = s->sprint_roll_target_speed; p.launch_rate = s->sprint_roll_launch_rate; break;
        case RUNNING:   p.target_speed = s->run_roll_target_speed;    p.launch_rate = s->run_roll_launch_rate;    break;
        case WALKING:   p.target_speed = s->walk_roll_target_speed;   p.launch_rate = s->walk_roll_launch_rate;   break;
        default:        p.target_speed = s->stand_roll_target_speed;  p.launch_rate = s->stand_roll_launch_rate;  break;
    }
    return p;
}

static uint8_t actorMotion_rollPhase(const ActorMotionData *d, const MotionCommand *cmd, const RollParams *p)
{
    if (cmd->roll_triggered)        return ACTOR_ROLL_PHASE_LAUNCH;
    if (d->roll_timer < p->grip_time) return ACTOR_ROLL_PHASE_SPIN;
    if (d->roll_timer < p->timer_max) return ACTOR_ROLL_PHASE_GRIP;
    return ACTOR_ROLL_PHASE_DONE;
}

static void actorMotion_rollLaunch(Entity *entity, MotionCommand *cmd, const RollParams *p, float dt)
{
    ActorMotionData *d = &entity->actor->motion.data;
    actorMotion_setHorizontalAcceleration(entity, cmd, p->target_speed, p->launch_rate);
    d->roll_timer += dt;
    if (d->roll_timer >= p->ground_time) cmd->roll_triggered = false;
}

static void actorMotion_rollSpin(Entity *entity, float dt)
{
    Actor *a = entity->actor;
    ActorMotionData *d = &a->motion.data;
    actorMotion_setHorizontalInertiaAcceleration(entity, d->horizontal_speed, a->motion.settings.run_acceleration_rate);
    d->roll_timer += dt;
}

static void actorMotion_rollGrip(Entity *entity, MotionCommand *cmd, float dt)
{
    Actor *a = entity->actor;
    ActorMotionData *d = &a->motion.data;
    actorMotion_setHorizontalAcceleration(entity, cmd, d->horizontal_speed, a->motion.settings.roll_acceleration_grip_rate);
    d->roll_timer += dt;
}

static void actorMotion_rollDone(Entity *entity)
{
    Actor *a = entity->actor;
    a->state.next = a->state.locomotion;
    a->motion.data.roll_timer = 0;
}

static void actorMotion_setRolling(Entity *entity, MotionCommand *cmd, float dt)
{
    Actor *a = entity->actor;
    ActorMotionData *d = &a->motion.data;

    d->rotation_mode = ACTOR_ROTATION_MODE_SNAP;

    const RollParams p = actorMotion_getRollParams(a->state.locomotion, &a->motion.settings);

    switch (actorMotion_rollPhase(d, cmd, &p)) {
        case ACTOR_ROLL_PHASE_LAUNCH: actorMotion_rollLaunch(entity, cmd, &p, dt); break;
        case ACTOR_ROLL_PHASE_SPIN:    actorMotion_rollSpin(entity, dt);            break;
        case ACTOR_ROLL_PHASE_GRIP:   actorMotion_rollGrip(entity, cmd, dt);       break;
        case ACTOR_ROLL_PHASE_DONE:   actorMotion_rollDone(entity);                break;
    }
}

static uint8_t actorMotion_jumpPhase(const ActorMotionData *d, const RigidBody *b, const ActorMotionSettings *s)
{
    if (d->jump_timer < s->jump_timer_max) return ACTOR_JUMP_PHASE_CHARGING;
    if (d->jump_force > 0)                 return ACTOR_JUMP_PHASE_LAUNCH;
    if (b->velocity.z > 0)                 return ACTOR_JUMP_PHASE_RISING;
    return ACTOR_JUMP_PHASE_DONE;
}

static void actorMotion_jumpCharging(Entity *entity, MotionCommand *cmd, float dt)
{
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    ActorMotionData *d = &a->motion.data;

    d->jump_timer += dt;
    if (cmd->jump_hold) {
        d->jump_force += dt;
        vector3_scale(&b->velocity, ACTOR_JUMP_HOLD_VELOCITY_SCALE);
    }
}

static void actorMotion_jumpLaunch(Entity *entity, float dt)
{
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    ActorMotionData *d = &a->motion.data;
    const ActorMotionSettings *s = &a->motion.settings;

    d->jump_timer += dt;
    b->velocity = d->jump_initial_velocity;
    vector3_scale(&b->velocity, ACTOR_JUMP_LAUNCH_VELOCITY_SCALE);
    b->velocity.z = d->jump_force * s->jump_force_multiplier;
    if (b->velocity.z < s->jump_minimum_speed)
        b->velocity.z = s->jump_minimum_speed;
    d->jump_force = 0;
}

static void actorMotion_jumpRising(Entity *entity, float dt)
{
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    ActorMotionData *d = &a->motion.data;

    d->jump_timer += dt;
    b->acceleration.z = a->motion.settings.gravity;
}

static void actorMotion_jumpDone(Entity *entity)
{
    Actor *a = entity->actor;
    a->body.acceleration.z = a->motion.settings.gravity;
    a->motion.data.jump_timer = 0;
    a->state.next = FALLING;
}

static void actorMotion_setJump(Entity *entity, MotionCommand *cmd, float dt)
{
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    ActorMotionData *d = &a->motion.data;
    const ActorMotionSettings *s = &a->motion.settings;

    if (cmd->jump_triggered) {
        d->jump_initial_velocity = b->velocity;
        cmd->jump_triggered = false;
    }

    actorMotion_setHorizontalAcceleration(entity, cmd, d->horizontal_speed, s->aerial_control_rate);

    switch (actorMotion_jumpPhase(d, b, s)) {
        case ACTOR_JUMP_PHASE_CHARGING: actorMotion_jumpCharging(entity, cmd, dt); break;
        case ACTOR_JUMP_PHASE_LAUNCH:   actorMotion_jumpLaunch(entity, dt);        break;
        case ACTOR_JUMP_PHASE_RISING:   actorMotion_jumpRising(entity, dt);        break;
        case ACTOR_JUMP_PHASE_DONE:     actorMotion_jumpDone(entity);              break;
    }
}

static void actorMotion_setFalling(Entity *entity, MotionCommand *cmd, float dt)
{
    (void)dt;
    Actor *a = entity->actor;
    RigidBody *b = &a->body;
    ActorMotionData *d = &a->motion.data;
    const ActorMotionSettings *s = &a->motion.settings;

    d->grounded = 0;
    actorMotion_setHorizontalAcceleration(entity, cmd, d->horizontal_speed, s->aerial_control_rate);
    b->acceleration.z = s->gravity;
    if (b->velocity.z > s->fall_max_speed)
        b->velocity.z = s->fall_max_speed;

    if (entity->transform.position.z <= d->grounding_height + ACTOR_GROUNDING_SNAP_ZONE) {
        d->grounded = 1;
        b->acceleration.z = 0;
        b->velocity.z = 0;
        entity->transform.position.z = d->grounding_height;
        a->state.next = a->state.locomotion;
        return;
    }
}

static void (*actorMotion_handler[ACTOR_STATE_COUNT])(Entity *, MotionCommand *, float) = {
    [STANDING_IDLE] = actorMotion_setIdle,
    [WALKING]       = actorMotion_setWalking,
    [RUNNING]       = actorMotion_setRunning,
    [SPRINTING]     = actorMotion_setSprinting,
    [ROLLING]       = actorMotion_setRolling,
    [JUMPING]       = actorMotion_setJump,
    [FALLING]       = actorMotion_setFalling,
};

_Static_assert(sizeof(actorMotion_handler) / sizeof(actorMotion_handler[0]) == ACTOR_STATE_COUNT, "actorMotion_handler must have one entry per actor state");

void actor_updateMotion(Entity *entity, MotionCommand *cmd, float dt)
{
    assert(entity);
    assert(entity->actor);
    assert(cmd);

    Actor *a = entity->actor;

    assert(a->state.current < ACTOR_STATE_COUNT);
    assert(actorMotion_handler[a->state.current] != NULL);

    // Default rotation mode each frame; states that need a different mode override it.
    a->motion.data.rotation_mode = ACTOR_ROTATION_MODE_LERP;
    // No pending transition until a handler posts one.
    a->state.next = ACTOR_STATE_NONE;

    actorMotion_handler[a->state.current](entity, cmd, dt);
    actorMotion_integrate(entity, dt);
    actorStates_evaluateTransitions(entity);
}
