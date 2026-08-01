#include <math.h>
#include <fmath.h>

#include "entity/entity.h"
#include "control/character_control.h"


static void characterControl_setJump(Character *character, MovementCommand *cmd, const ControllerActions *actions)
{
	CharacterMovement *movement = &character->movement;

	if (actions->jump && characterMovement_isLocomotion(movement->current)) {
		cmd->jump_held      = true;
		cmd->jump_triggered = true;
		characterMovement_setMode(movement, MOVEMENT_STATE_JUMPING);
	} else if (actions->jump_held) {
		return;
	} else {
		cmd->jump_held = false;
	}
}

static void characterControl_setRoll(Character *character, MovementCommand *cmd, const ControllerActions *actions)
{
	CharacterMovement *movement = &character->movement;

	if (actions->roll && characterMovement_isLocomotion(movement->current) && movement->current != MOVEMENT_STATE_IDLE) {
		cmd->roll_triggered = true;
		characterMovement_setMode(movement, MOVEMENT_STATE_ROLLING);
	}
}

static void characterControl_setLocomotionWithStick(Character *character, MovementCommand *cmd, const ControllerActions *actions, float camera_angle_around)
{
	CharacterMovement *movement = &character->movement;
	float stick_magnitude = 0;

	if (fabsf(actions->stick_x) >= PLAYER_STICK_DEADZONE || fabsf(actions->stick_y) >= PLAYER_STICK_DEADZONE) {
		Vector2 stick   = {actions->stick_x, actions->stick_y};
		stick_magnitude = vector2_magnitude(&stick);
		cmd->target_yaw = rad_to_deg(fm_atan2f(actions->stick_x, -actions->stick_y) - deg_to_rad(camera_angle_around));
	}

	if (!characterMovement_isLocomotion(movement->current)) return;

	if (stick_magnitude == 0)
		characterMovement_setMode(movement, MOVEMENT_STATE_IDLE);
	else if (stick_magnitude <= PLAYER_STICK_WALK_THRESHOLD)
		characterMovement_setMode(movement, MOVEMENT_STATE_WALKING);
	else if (actions->sprint)
		characterMovement_setMode(movement, MOVEMENT_STATE_SPRINTING);
	else
		characterMovement_setMode(movement, MOVEMENT_STATE_RUNNING);
}

void characterControl_update(Character *character, MovementCommand *cmd, const ControllerActions *actions, float camera_angle_around)
{
	characterControl_setRoll(character, cmd, actions);
	characterControl_setJump(character, cmd, actions);
	characterControl_setLocomotionWithStick(character, cmd, actions, camera_angle_around);
}
