#include "player/player.h"
#include "control/player_control.h"
#include "control/actor_control.h"
#include "control/menu_control.h"
#include "control/controller.h"
#include "viewport/viewport.h"
#include "game/game.h"


void player_setActorControl(Player *player, const ControllerActions *actions, Viewport *viewport)
{
	actorControl_update(
		player->entity->actor,
		&player->cmd,
		actions,
		viewport->camera.spherical.data.angle_around_center
	);
}

void player_setControllerData(Player *player, Game *game)
{
	controller_poll();
	Controller *control = controller_get();
	for (int i = 0; i < PLAYER_COUNT; i++)
		menuControl_update(&player[i], &control[i].actions, game);
}
