#include "player/e64_player.h"
#include "control/e64_player_control.h"
#include "control/e64_character_control.h"
#include "control/e64_menu_control.h"
#include "control/e64_controller.h"
#include "viewport/e64_viewport.h"
#include "game/e64_game.h"


void player_setCharacterControl(PlayerID id, Viewport *viewport)
{
	Player *player = &player_get()[id];
	if (player->character == NULL || player->control == NULL) return;

	/* Read where it is used: what the controller is doing this frame is worth
	   nothing on the next one. */
	CharacterControls controls;
	characterControls_read(&controls, player->control);

	characterControl_update(
		player->character,
		&player->cmd,
		&controls,
		camera_getAngleAround(&viewport->camera, &player->character->entity->transform.position)
	);
}

/* Once a frame, not once per seat: there is one menu, and the state reads the
   controller its own binding names. Run per seat, every value a state wrote
   continuously came out as the last seat's, which is the one nobody is on. */
void player_setControllerData(Game *game)
{
	menuControl_update(game);
}
