#include <math.h>
#include <fmath.h>

#include "entity/entity.h"
#include "actor/actor_states.h"
#include "control/actor_control.h"


static void actorControl_setJump(Actor *actor, MotionCommand *cmd, const ControllerActions *actions)
{
	ActorStateData *state = &actor->state;

	if (actions->jump && actorStates_isLocomotion(state->current)) {
		cmd->jump_held      = true;
		cmd->jump_triggered = true;
		actor_setState(state, ACTOR_STATE_JUMPING);
	} else if (actions->jump_held) {
		return;
	} else {
		cmd->jump_held = false;
	}
}

static void actorControl_setRoll(Actor *actor, MotionCommand *cmd, const ControllerActions *actions)
{
	ActorStateData *state = &actor->state;

	if (actions->roll && actorStates_isLocomotion(state->current)) {
		cmd->roll_triggered = true;
		actor_setState(state, ACTOR_STATE_ROLLING);
	}
}

static void actorControl_setLocomotionWithStick(Actor *actor, MotionCommand *cmd, const ControllerActions *actions, float camera_angle_around)
{
	ActorStateData *state = &actor->state;
	float stick_magnitude = 0;

	if (fabsf(actions->stick_x) >= PLAYER_STICK_DEADZONE || fabsf(actions->stick_y) >= PLAYER_STICK_DEADZONE) {
		Vector2 stick   = {actions->stick_x, actions->stick_y};
		stick_magnitude = vector2_magnitude(&stick);
		cmd->target_yaw = rad_to_deg(fm_atan2f(actions->stick_x, -actions->stick_y) - deg_to_rad(camera_angle_around));
	}

	if (!actorStates_isLocomotion(state->current)) return;

	if (stick_magnitude == 0)
		actor_setState(state, ACTOR_STATE_IDLE);
	else if (stick_magnitude <= PLAYER_STICK_WALK_THRESHOLD)
		actor_setState(state, ACTOR_STATE_WALKING);
	else if (actions->sprint)
		actor_setState(state, ACTOR_STATE_SPRINTING);
	else
		actor_setState(state, ACTOR_STATE_RUNNING);
}

void actorControl_update(Actor *actor, MotionCommand *cmd, const ControllerActions *actions, float camera_angle_around)
{
	actorControl_setRoll(actor, cmd, actions);
	actorControl_setJump(actor, cmd, actions);
	actorControl_setLocomotionWithStick(actor, cmd, actions, camera_angle_around);
}
