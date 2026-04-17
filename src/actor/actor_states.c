#include <t3d/t3danim.h>

#include "physics/physics.h"
#include "entity/entity.h"


static const bool updates_locomotion[] = {
	[ACTOR_STATE_IDLE]      = true,
	[ACTOR_STATE_WALKING]   = true,
	[ACTOR_STATE_RUNNING]   = true,
	[ACTOR_STATE_SPRINTING] = true,
	[ACTOR_STATE_ROLLING]   = false,
	[ACTOR_STATE_JUMPING]   = false,
	[ACTOR_STATE_FALLING]   = false,
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
	ActorStateData *state = &entity->actor->state;
	if (state->next == ACTOR_STATE_NONE) return;
	actor_setState(state, state->next);
	state->next = ACTOR_STATE_NONE;
}
