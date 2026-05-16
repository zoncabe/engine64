#include "player/player.h"
#include "control/menu_control.h"
#include "game/game.h"
#include "menu/menu.h"
#include "ui/settings_ui.h"
#include "ui/main_menu_ui.h"
#include "ui/pause_ui.h"


static void menuControl_handleIntro(Player *player, const ControllerActions *actions, Game *game) { (void)player; (void)actions; (void)game; }

static void mainMenu_onExitToSettings(void *ctx)
{
	(void)ctx;
	settings_ui_open(SETTINGS_CTX_FROM_MAIN_MENU, main_menu_startEnter);
}

static void pause_onExitToSettings(void *ctx)
{
	(void)ctx;
	settings_ui_open(SETTINGS_CTX_FROM_PAUSE, pause_startEnter);
}

static Game *pause_exit_game = NULL;
static Game *mainMenu_exit_game = NULL;

static void pause_onExitToGameplay(void *ctx)
{
	(void)ctx;
	if (pause_exit_game) game_setState(pause_exit_game, GAME_STATE_GAMEPLAY);
}

static void mainMenu_onExitToGameplay(void *ctx)
{
	(void)ctx;
	if (mainMenu_exit_game) game_setState(mainMenu_exit_game, GAME_STATE_GAMEPLAY);
}

static void settings_onCloseToGameplay(void)
{
	if (pause_exit_game) game_setState(pause_exit_game, GAME_STATE_GAMEPLAY);
}

static void menuControl_handleMainMenu(Player *player, const ControllerActions *actions, Game *game)
{
	(void)player;

	if (menuStack_current() != NULL) {
		settings_ui_handleInput(actions);
		return;
	}

	if (main_menu_isTransitioning()) return;

	if (actions->confirm) {
		int8_t idx = menuStack_getIndex();
		if (idx == 0) {
			mainMenu_exit_game = game;
			main_menu_startExit(mainMenu_onExitToGameplay, NULL);
		}
		if (idx == 1) main_menu_startExit(mainMenu_onExitToSettings, NULL);
	}
	if (actions->menu_up)   menuStack_moveIndex(-1, 2);
	if (actions->menu_down) menuStack_moveIndex(1,  2);
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
		if (menuStack_current() != NULL) {
			settings_ui_setOnClose(settings_onCloseToGameplay);
			if (!settings_ui_isTransitioning()) settings_ui_startExit();
		} else if (!pause_isTransitioning()) {
			pause_startExit(pause_onExitToGameplay, NULL);
		}
		return;
	}

	if (menuStack_current() != NULL) {
		settings_ui_handleInput(actions);
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
		pause_startExit(pause_onExitToSettings, NULL);
		return;
	}
	if (actions->confirm && menuStack_getIndex() == 2) {
		game_setState(game, GAME_STATE_MAIN_MENU);
		menuStack_setIndex(0);
	}
	if (actions->menu_up)   menuStack_moveIndex(-1, 2);
	if (actions->menu_down) menuStack_moveIndex(1,  2);
}

static void menuControl_handleGameOver(Player *player, const ControllerActions *actions, Game *game) { (void)player; (void)actions; (void)game; }

typedef void (*MenuControlHandler)(Player *, const ControllerActions *, Game *);

static const MenuControlHandler menuControl_handler[GAME_STATE_COUNT] = {
	[GAME_STATE_INTRO]      = menuControl_handleIntro,
	[GAME_STATE_MAIN_MENU]  = menuControl_handleMainMenu,
	[GAME_STATE_GAMEPLAY]   = menuControl_handleGameplay,
	[GAME_STATE_PAUSE]      = menuControl_handlePause,
	[GAME_STATE_GAME_OVER]  = menuControl_handleGameOver,
};


void menuControl_update(Player *player, const ControllerActions *actions, Game *game)
{
	menuControl_handler[game->state](player, actions, game);
}
