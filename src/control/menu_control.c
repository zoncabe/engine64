#include "player/player.h"
#include "control/menu_control.h"
#include "game/game.h"
#include "menu/menu.h"
#include "ui/main_menu_ui.h"
#include "ui/pause_ui.h"
#include "ui/credits_ui.h"


static void menuControl_handleIntro(Player *player, const ControllerActions *actions, Game *game) { (void)player; (void)actions; (void)game; }

static Game *pause_exit_game = NULL;
static Game *mainMenu_exit_game = NULL;

static void pause_onExitToGameplay(void *ctx)
{
	(void)ctx;
	if (pause_exit_game) game_setState(pause_exit_game, GAME_STATE_GAMEPLAY);
}

static void pause_onQuitToMainMenu(void *ctx)
{
	(void)ctx;
	if (pause_exit_game) game_setState(pause_exit_game, GAME_STATE_MAIN_MENU);
}

static void mainMenu_onExitToGameplay(void *ctx)
{
	(void)ctx;
	if (mainMenu_exit_game) game_setState(mainMenu_exit_game, GAME_STATE_GAMEPLAY);
}

static void mainMenu_onExitToCredits(void *ctx)
{
	(void)ctx;
	if (mainMenu_exit_game) game_setState(mainMenu_exit_game, GAME_STATE_CREDITS);
}

static void credits_onExitToMainMenu(void *ctx)
{
	(void)ctx;
	if (mainMenu_exit_game) game_setState(mainMenu_exit_game, GAME_STATE_MAIN_MENU);
}

static void menuControl_handleMainMenu(Player *player, const ControllerActions *actions, Game *game)
{
	(void)player;

	if (main_menu_isTransitioning()) return;

	if (actions->confirm) {
		int8_t idx = menuStack_getIndex();
		if (idx == 0) {
			mainMenu_exit_game = game;
			main_menu_startExit(mainMenu_onExitToGameplay, NULL);
		}
		if (idx == 1) {
			mainMenu_exit_game = game;
			main_menu_startExit(mainMenu_onExitToCredits, NULL);
		}
	}
	if (actions->menu_up)   menuStack_moveIndex(-1, 1);
	if (actions->menu_down) menuStack_moveIndex(1,  1);
}

#define CREDITS_DPAD_SPEED   60.0f
#define CREDITS_STICK_SPEED 140.0f
#define CREDITS_STICK_DEAD    0.15f

static void menuControl_handleCredits(Player *player, const ControllerActions *actions, Game *game)
{
	(void)player;

	if (credits_isTransitioning()) return;

	if (actions->cancel) {
		mainMenu_exit_game = game;
		credits_startExit(credits_onExitToMainMenu, NULL);
		return;
	}

	float velocity = 0.0f;

	if (actions->menu_up_held)   velocity -= CREDITS_DPAD_SPEED;
	if (actions->menu_down_held) velocity += CREDITS_DPAD_SPEED;

	/* Stick up scrolls back, down forward; its magnitude sets the speed.
	   Raw N64 stick range is about +-80. */
	float stick = actions->stick_y / 80.0f;
	if (stick >  1.0f) stick =  1.0f;
	if (stick < -1.0f) stick = -1.0f;
	if (stick > CREDITS_STICK_DEAD || stick < -CREDITS_STICK_DEAD)
		velocity -= stick * CREDITS_STICK_SPEED;

	credits_setScrollVelocity(velocity);
}

static void menuControl_handleGameplay(Player *player, const ControllerActions *actions, Game *game)
{
	(void)player;
	if (actions->pause) game_setState(game, GAME_STATE_PAUSE);
}

static void menuControl_handlePause(Player *player, const ControllerActions *actions, Game *game)
{
	(void)player;

	if (actions->pause) {
		pause_exit_game = game;
		if (!pause_isTransitioning())
			pause_startExit(pause_onExitToGameplay, NULL);
		return;
	}

	if (pause_isTransitioning()) return;

	if (actions->cancel || (actions->confirm && menuStack_getIndex() == 0)) {
		pause_exit_game = game;
		pause_startExit(pause_onExitToGameplay, NULL);
		menuStack_setIndex(0);
		return;
	}
	if (actions->confirm && menuStack_getIndex() == 1) {
		pause_exit_game = game;
		pause_startQuit(pause_onQuitToMainMenu, NULL);
		menuStack_setIndex(0);
	}
	if (actions->menu_up)   menuStack_moveIndex(-1, 1);
	if (actions->menu_down) menuStack_moveIndex(1,  1);
}

static void menuControl_handleGameOver(Player *player, const ControllerActions *actions, Game *game) { (void)player; (void)actions; (void)game; }

typedef void (*MenuControlHandler)(Player *, const ControllerActions *, Game *);

static const MenuControlHandler menuControl_handler[GAME_STATE_COUNT] = {
	[GAME_STATE_INTRO]      = menuControl_handleIntro,
	[GAME_STATE_MAIN_MENU]  = menuControl_handleMainMenu,
	[GAME_STATE_CREDITS]    = menuControl_handleCredits,
	[GAME_STATE_GAMEPLAY]   = menuControl_handleGameplay,
	[GAME_STATE_PAUSE]      = menuControl_handlePause,
	[GAME_STATE_GAME_OVER]  = menuControl_handleGameOver,
};


void menuControl_update(Player *player, const ControllerActions *actions, Game *game)
{
	menuControl_handler[game->state](player, actions, game);
}
