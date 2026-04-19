#include <math.h>
#include <fmath.h>

#include "player/player.h"
#include "actor/actor_states.h"
#include "control/player_control.h"
#include "control/controller.h"
#include "viewport/viewport.h"
#include "game/game.h"
#include "menu/menu.h"
#include "ui/settings_ui.h"
#include "ui/main_menu_ui.h"
#include "ui/pause_ui.h"


static void playerControl_setJump(Player *player, const ControllerActions *actions)
{
	ActorStateData *state = &player->entity->actor->state;

	if (actions->jump && actorStates_isLocomotion(state->current)) {
		player->cmd.jump_held      = true;
		player->cmd.jump_triggered = true;
		actor_setState(state, ACTOR_STATE_JUMPING);
	} else if (actions->jump_held) {
		return;
	} else {
		player->cmd.jump_held = false;
	}
}

static void playerControl_setRoll(Player *player, const ControllerActions *actions)
{
	ActorStateData *state = &player->entity->actor->state;

	if (actions->roll && actorStates_isLocomotion(state->current)) {
		player->cmd.roll_triggered = true;
		actor_setState(state, ACTOR_STATE_ROLLING);
	}
}

static void playerControl_setLocomotionWithStick(Player *player, const ControllerActions *actions, float camera_angle_around)
{
	ActorStateData *state = &player->entity->actor->state;
	float stick_magnitude = 0;

	if (fabsf(actions->stick_x) >= PLAYER_STICK_DEADZONE || fabsf(actions->stick_y) >= PLAYER_STICK_DEADZONE) {
		Vector2 stick   = {actions->stick_x, actions->stick_y};
		stick_magnitude = vector2_magnitude(&stick);
		player->cmd.target_yaw = rad_to_deg(fm_atan2f(actions->stick_x, -actions->stick_y) - deg_to_rad(camera_angle_around));
	}

	if (!actorStates_isLocomotion(state->current)) return;

	if (stick_magnitude == 0)
		actor_setState(state, ACTOR_STATE_IDLE);
	else if (stick_magnitude <= PLAYER_STICK_WALK_THRESHOLD)
		actor_setState(state, ACTOR_STATE_WALKING);
	else if (actions->sprint)
		actor_setState(state, ACTOR_STATE_SPRINTING);
	else
		actor_setState(state, ACTOR_STATE_RUNNING);
}

void player_setActorControl(Player *player, const ControllerActions *actions, Viewport *viewport)
{
	playerControl_setRoll(player, actions);
	playerControl_setJump(player, actions);
	playerControl_setLocomotionWithStick(player, actions, viewport->camera.spherical.data.angle_around_center);
}


static void playerControl_handleIntro(Player *player, const ControllerActions *actions, Game *game) { (void)player; (void)actions; (void)game; }

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

static void pause_onExitToGameplay(void *ctx)
{
	(void)ctx;
	if (pause_exit_game) game_setState(pause_exit_game, GAME_STATE_GAMEPLAY);
}

static void settings_onCloseToGameplay(void)
{
	if (pause_exit_game) game_setState(pause_exit_game, GAME_STATE_GAMEPLAY);
}

static void playerControl_handleMainMenu(Player *player, const ControllerActions *actions, Game *game)
{
	(void)player;

	if (menuStack_current() != NULL) {
		settings_ui_handleInput(actions);
		return;
	}

	if (main_menu_isTransitioning()) return;

	if (actions->confirm) {
		int8_t idx = menuStack_getIndex();
		if (idx == 0) game_setState(game, GAME_STATE_GAMEPLAY);
		if (idx == 1) main_menu_startExit(mainMenu_onExitToSettings, NULL);
	}
	if (actions->menu_up)   menuStack_moveIndex(-1, 2);
	if (actions->menu_down) menuStack_moveIndex(1,  2);
}

static void playerControl_handleGameplay(Player *player, const ControllerActions *actions, Game *game)
{
	(void)player;
	if (actions->pause) game_setState(game, GAME_STATE_PAUSE);
}

static void playerControl_handlePause(Player *player, const ControllerActions *actions, Game *game)
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

static void playerControl_handleGameOver(Player *player, const ControllerActions *actions, Game *game) { (void)player; (void)actions; (void)game; }

typedef void (*PlayerControlHandler)(Player *, const ControllerActions *, Game *);

static const PlayerControlHandler playerControl_handler[GAME_STATE_COUNT] = {
	[GAME_STATE_INTRO]      = playerControl_handleIntro,
	[GAME_STATE_MAIN_MENU]  = playerControl_handleMainMenu,
	[GAME_STATE_GAMEPLAY]   = playerControl_handleGameplay,
	[GAME_STATE_PAUSE]      = playerControl_handlePause,
	[GAME_STATE_GAME_OVER]  = playerControl_handleGameOver,
};


void player_setControllerData(Player *player, Game *game)
{
	controller_poll();
	Controller *control = controller_get();
	for (uint8_t i = 0; i < PLAYER_COUNT; i++)
		playerControl_handler[game->state](&player[i], &control[i].actions, game);
}
