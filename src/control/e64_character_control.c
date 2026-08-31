#include <math.h>
#include <fmath.h>

#include "entity/e64_entity.h"
#include "control/e64_character_control.h"


static void characterControl_setJump(Character *character, MovementCommand *cmd, const CharacterControls *actions)
{
	CharacterMovement *movement = &character->movement;

	/* The button never changes the state: it only asks for a jump. The crouch
	   runs on the ground and the movement switches to the air on the impulse.
	   The coyote window counts as ground for this: the ledge is gone but the
	   ask is still taken. */
	if (actions->jump
	    && (characterMovement_isLocomotion(movement->current)
	        || characterMovement_isCoyoteOpen(character))
	    && movement->data.jump_timer == 0.0f) {
		cmd->jump_held      = true;
		cmd->jump_triggered = true;
	} else if (actions->jump_held) {
		return;
	} else {
		cmd->jump_held = false;
	}
}

static void characterControl_setRoll(Character *character, MovementCommand *cmd, const CharacterControls *actions)
{
	CharacterMovement *movement = &character->movement;

	/* Not while a jump is being charged: the crouch owns the body until it
	   takes off. */
	if (actions->roll && characterMovement_isLocomotion(movement->current)
	    && movement->current != MOVEMENT_STATE_IDLE
	    && movement->data.jump_timer == 0.0f) {
		cmd->roll_triggered = true;
		characterMovement_setMode(movement, MOVEMENT_STATE_ROLLING);
	}
}

/* Only with both feet in ordinary locomotion: never mid-air, mid-roll, in
   the water or on a ladder, and not while a jump crouch owns the body. */
static void characterControl_setWeaponSwitch(Character *character, const CharacterControls *actions)
{
	CharacterMovement *movement = &character->movement;

	if (!characterMovement_isLocomotion(movement->current)
	    || movement->data.jump_timer != 0.0f) return;

	if (actions->weapon_next) character_cycleWeapon(character, +1);
	if (actions->weapon_prev) character_cycleWeapon(character, -1);
}

static void characterControl_setLocomotionWithStick(Character *character, MovementCommand *cmd, const CharacterControls *actions, float camera_angle_around)
{
	CharacterMovement *movement = &character->movement;
	float stick_magnitude = 0;

	if (fabsf(actions->stick_x) >= STICK_DEADZONE || fabsf(actions->stick_y) >= STICK_DEADZONE) {
		Vector2 stick   = {actions->stick_x, actions->stick_y};
		stick_magnitude = vector2_magnitude(&stick);
		cmd->target_yaw = rad_to_deg(fm_atan2f(actions->stick_x, -actions->stick_y) - deg_to_rad(camera_angle_around));
	}

	/* Swimming keeps its state; the stick only picks the swim gait. */
	if (movement->current == MOVEMENT_STATE_SWIMMING) {
		if (stick_magnitude == 0)  cmd->swim_gait = CHARACTER_SWIM_GAIT_IDLE;
		else if (actions->sprint)  cmd->swim_gait = CHARACTER_SWIM_GAIT_FAST;
		else                       cmd->swim_gait = CHARACTER_SWIM_GAIT_SLOW;
		return;
	}

	uint8_t mode = (stick_magnitude == 0) ? MOVEMENT_STATE_IDLE : MOVEMENT_STATE_WALKING;

	/* An action owns the current state and its gait until it ends: the stick
	   only picks the state it goes back to. */
	if (!characterMovement_isLocomotion(movement->current)) {
		movement->locomotion = mode;
		return;
	}

	characterMovement_setMode(movement, mode);
	if (mode == MOVEMENT_STATE_IDLE) return;

	const CharacterMovementSettings *settings = movement->settings;
	uint8_t last_gait = settings->gait_count - 1;

	if (stick_magnitude <= PLAYER_STICK_WALK_THRESHOLD)
		cmd->gait = 0;
	else if (actions->sprint && !actions->aim)
		cmd->gait = last_gait;
	else
		cmd->gait = (last_gait > 1) ? 1 : last_gait;
}

/* The aim button holds the drawn weapon at the ready; the shoot button on top
   charges the shot. The release edge already travels in the actions, left for
   the shot itself. */
static void characterControl_setAiming(Character *character, MovementCommand *cmd, const CharacterControls *actions)
{
	const WeaponDef *drawn = character_drawnWeapon(character);
	bool charges = drawn && drawn->shoot_mode == SHOOT_CHARGE;

	cmd->aiming = drawn && actions->aim
		&& characterMovement_isLocomotion(character->movement.current);
	cmd->charging_shoot = cmd->aiming && charges && actions->shoot;
}

static void characterControl_setStrafe(Character *character, MovementCommand *cmd, const CharacterControls *actions, float camera_angle_around)
{
	cmd->strafe     = actions->aim && characterMovement_isLocomotion(character->movement.current);
	cmd->strafe_yaw = angle_wrap(camera_angle_around + 180.0f + CHARACTER_STRAFE_YAW_OFFSET);
}

/* A ladder is asked for with the stick, never a button: pushing at the rungs
   climbs and pulling away from them descends. Both are read against the
   ladder's own facing, so which one the stick means never depends on where
   the camera happens to be. Only the release is a button, and it is the one
   that jumps everywhere else. */
static void characterControl_setClimb(Character *character, MovementCommand *cmd, const CharacterControls *actions)
{
	CharacterMovement *movement = &character->movement;

	cmd->climb         = 0.0f;
	cmd->climb_release = false;

	bool climbing = movement->current == MOVEMENT_STATE_CLIMBING;
	if (!climbing && !movement->data.on_ladder) return;

	if (climbing && actions->jump) {
		cmd->climb_release = true;
		return;
	}

	/* Already on it: the stick is read raw, up climbs and down descends. A
	   ladder is the one place the camera must not get a vote — it swings
	   around the body as it rises, and a heading built off it would turn
	   the same push into a climb or a drop depending on where it ended up. */
	if (climbing) {
		if (actions->stick_y >=  STICK_DEADZONE) cmd->climb =  1.0f;
		if (actions->stick_y <= -STICK_DEADZONE) cmd->climb = -1.0f;
		return;
	}

	if (fabsf(actions->stick_x) < STICK_DEADZONE
	 && fabsf(actions->stick_y) < STICK_DEADZONE) return;

	/* Grabbing on is the opposite case: walking at a ladder is what asks
	   for it, so the entry is the camera-relative heading measured against
	   the ladder's facing. target_yaw is the stick already in world space
	   and a body's rotation is the negative of the heading it walks, which
	   is what brings the two into one frame to be compared. */
	float alignment = fm_cosf(deg_to_rad(cmd->target_yaw + movement->data.ladder_yaw));

	if (alignment >= fm_cosf(deg_to_rad(CHARACTER_LADDER_ENTER_ANGLE))) cmd->climb = 1.0f;
}

void characterControls_read(CharacterControls *controls, const CharacterControlBinding *binding)
{
	const Controller *controller = &controller_get()[binding->player];

	*controls = (CharacterControls){
		.jump       = button_getPressed(controller, &controller->pressed, binding->jump),
		.jump_held  = button_getPressed(controller, &controller->held,    binding->jump),
		.roll       = button_getPressed(controller, &controller->pressed, binding->roll),
		.sprint     = button_getPressed(controller, &controller->held,    binding->sprint),
		.aim        = button_getPressed(controller, &controller->held,    binding->aim),
		.shoot          = button_getPressed(controller, &controller->held,     binding->shoot),
		.shoot_released = button_getPressed(controller, &controller->released, binding->shoot),
		.weapon_next    = button_getPressed(controller, &controller->pressed,  binding->weapon_next),
		.weapon_prev    = button_getPressed(controller, &controller->pressed,  binding->weapon_prev),
		.stick_x = controller->input.stick_x,
		.stick_y = controller->input.stick_y,
	};
}

void characterControl_update(Character *character, MovementCommand *cmd, const CharacterControls *actions, float camera_angle_around)
{
	characterControl_setWeaponSwitch(character, actions);
	characterControl_setRoll(character, cmd, actions);
	characterControl_setJump(character, cmd, actions);
	characterControl_setStrafe(character, cmd, actions, camera_angle_around);
	characterControl_setAiming(character, cmd, actions);
	characterControl_setLocomotionWithStick(character, cmd, actions, camera_angle_around);

	/* After the stick: the climb is read off the heading it just wrote. */
	characterControl_setClimb(character, cmd, actions);
}
