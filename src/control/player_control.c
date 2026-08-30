#include "player/player.h"
#include "control/player_control.h"
#include "control/character_control.h"
#include "control/menu_control.h"
#include "control/controller.h"
#include "viewport/viewport.h"
#include "game/game.h"


void player_setCharacterControl(PlayerID id, Viewport *viewport)
{
	Player *player = &player_get()[id];
	if (player->character == NULL || player->control == NULL) return;

	/* Read where it is used: what the pad is doing this frame is worth
	   nothing on the next one. */
	CharacterControls controls;
	characterControls_read(&controls, player->control, id);

	characterControl_update(
		player->character,
		&player->cmd,
		&controls,
		camera_getAngleAround(&viewport->camera, &player->character->entity->transform.position)
	);
}

void player_setControllerData(Player *player, Game *game)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		menuControl_update(&player[i], game);
}
