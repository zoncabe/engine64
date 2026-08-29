#include "player/player.h"
#include "control/player_control.h"
#include "control/character_control.h"
#include "control/menu_control.h"
#include "control/controller.h"
#include "viewport/viewport.h"
#include "game/game.h"


void player_setCharacterControl(Player *player, const CharacterControls *controls, Viewport *viewport)
{
	characterControl_update(
		player->character,
		&player->cmd,
		controls,
		camera_getAngleAround(&viewport->camera, &player->character->entity->transform.position)
	);
}

void player_setControllerData(Player *player, Game *game)
{
	for (int i = 0; i < PLAYER_COUNT; i++)
		menuControl_update(&player[i], game);
}
