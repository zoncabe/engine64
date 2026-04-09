#include <t3d/t3danim.h>

#include "../../include/physics/physics.h"
#include "../../include/entity/entity.h"


static const bool updates_locomotion[] = {
    [STANDING_IDLE] = true,
    [WALKING]       = true,
    [RUNNING]       = true,
    [SPRINTING]     = true,
    [ROLLING]       = false,
    [JUMPING]       = false,
    [FALLING]       = false,
};

void actor_setState(ActorStateData *state, uint8_t new_state)
{
    if (state->current == new_state) return;
    state->previous = state->current;
    state->current  = new_state;
    if (updates_locomotion[new_state]) state->locomotion = new_state;
}

bool actorStates_isLocomotion(uint8_t state)
{
    return updates_locomotion[state];
}

void actorStates_evaluateTransitions(Entity *entity)
{
    ActorStateData *st = &entity->actor->state;
    if (st->next == ACTOR_STATE_NONE) return;
    actor_setState(st, st->next);
    st->next = ACTOR_STATE_NONE;
}
