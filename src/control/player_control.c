#include "player/player.h"
#include "control/player_control.h"
#include "control/character_control.h"
#include "control/menu_control.h"
#include "control/controller.h"
#include "viewport/viewport.h"
#include "game/game.h"


void player_setCharacterControl(Player *player, const ControllerActions *actions, Viewport *viewport)
{
	if (actions->switch_character_next) player_switchCharacter(player,  1);
	if (actions->switch_character_prev) player_switchCharacter(player, -1);

	characterControl_update(
		player->character,
		&player->cmd,
		actions,
		camera_getAngleAround(&viewport->camera, &player->character->entity->transform.position)
	);
}

void player_setControllerData(Player *player, Game *game)
{
	controller_poll();
	Controller *control = controller_get();
	for (int i = 0; i < PLAYER_COUNT; i++)
		menuControl_update(&player[i], &control[i].actions, game);
}
