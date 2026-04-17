#ifndef ACTOR_STATES_H
#define ACTOR_STATES_H

#include <stdbool.h>
#include <stdint.h>


typedef enum {

	ACTOR_STATE_IDLE,
	ACTOR_STATE_WALKING,
	ACTOR_STATE_RUNNING,
	ACTOR_STATE_SPRINTING,
	ACTOR_STATE_ROLLING,
	ACTOR_STATE_JUMPING,
	ACTOR_STATE_FALLING,
	ACTOR_STATE_COUNT,
	ACTOR_STATE_NONE

} ActorState;


typedef struct ActorStateData {

	uint8_t current;
	uint8_t previous;
	uint8_t locomotion;
	uint8_t next;

} ActorStateData;

void actor_setState(ActorStateData *state, uint8_t new_state);
bool actorStates_isLocomotion(uint8_t state);

void actorStates_evaluateTransitions(Entity *entity);

#endif
