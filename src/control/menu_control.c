/*
	Dispatch only: what each state does with the pad lives in the game's
	state table, in the def's control callback.
*/
#include "control/menu_control.h"
#include "game/game.h"
#include "game/game_states.h"


void menuControls_map(MenuControls *controls, const Controller *pad, const MenuControlBinding *binding)
{
	*controls = (MenuControls){
		.confirm   = button_getPressed(pad, &pad->pressed, binding->confirm),
		.cancel    = button_getPressed(pad, &pad->pressed, binding->cancel),
		.pause     = button_getPressed(pad, &pad->pressed, binding->pause),
		.up        = button_getPressed(pad, &pad->pressed, binding->up),
		.down      = button_getPressed(pad, &pad->pressed, binding->down),
		.up_held   = button_getPressed(pad, &pad->held,    binding->up),
		.down_held = button_getPressed(pad, &pad->held,    binding->down),
		.left      = button_getPressed(pad, &pad->pressed, binding->left),
		.right     = button_getPressed(pad, &pad->pressed, binding->right),
		.tab_left  = button_getPressed(pad, &pad->pressed, binding->tab_left),
		.tab_right = button_getPressed(pad, &pad->pressed, binding->tab_right),
	};
}

void menuControl_update(Player *player, Game *game)
{
	const GameStateDef *def = gameState_get(game->state);
	if (def->control) def->control(player, game);
}
