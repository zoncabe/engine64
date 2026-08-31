#ifndef ENGINE64_CHARACTER_CONTROL_H
#define ENGINE64_CHARACTER_CONTROL_H

#include "e64_controller.h"
#include "character/e64_character.h"
#include "character/e64_character_movement.h"

#define PLAYER_STICK_WALK_THRESHOLD 65


/* What a body can be asked to do. What a character does not do is left out:
   an unwritten button is BTN_NONE and reads as never pressed. */
typedef struct CharacterControlBinding {

	/* Whose seat drives this body. */
	PlayerID player;

	ButtonID jump;
	ButtonID roll;
	ButtonID sprint;
	ButtonID aim;
	ButtonID shoot;
	ButtonID weapon_next;
	ButtonID weapon_prev;

} CharacterControlBinding;


typedef struct CharacterControls {

	bool  jump;
	bool  jump_held;
	bool  roll;
	bool  sprint;
	bool  aim;
	bool  shoot;            /* held: the bow draws while it stays down */
	bool  shoot_released;   /* the shot fires on this edge */
	bool  weapon_next;
	bool  weapon_prev;
	float stick_x;
	float stick_y;

} CharacterControls;


/* This frame's state of the buttons the binding names, off that player's
   controller.
   The binding is the mapping and is written once; this only reads what those
   buttons are doing now. */
void characterControls_read(CharacterControls *controls, const CharacterControlBinding *binding);
void characterControl_update(Character *character, MovementCommand *cmd, const CharacterControls *controls, float camera_angle_around);

#endif
