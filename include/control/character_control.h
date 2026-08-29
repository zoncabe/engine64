#ifndef CHARACTER_CONTROL_H
#define CHARACTER_CONTROL_H

#include "controller.h"
#include "character/character.h"
#include "character/character_movement.h"

#define PLAYER_STICK_DEADZONE       6
#define PLAYER_STICK_WALK_THRESHOLD 65


/* What a body can be asked to do. A character that does not do one of these
   leaves its button at BTN_NONE and the control reads as never pressed. */
typedef struct CharacterControlBinding {

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


void characterControls_map(CharacterControls *controls, const Controller *pad, const CharacterControlBinding *binding);
void characterControl_update(Character *character, MovementCommand *cmd, const CharacterControls *controls, float camera_angle_around);

#endif
