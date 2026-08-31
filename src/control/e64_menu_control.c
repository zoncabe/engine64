/*
	Dispatch only: what each state does with the controller lives in the game's
	state table, in the def's control callback.
*/
#include <math.h>

#include "control/e64_menu_control.h"
#include "game/e64_game.h"
#include "game/e64_game_states.h"
#include "time/e64_time.h"


/* A direction given once moves the index once. Kept held, it waits out
   MENU_REPEAT_DELAY and from there steps every MENU_REPEAT_PERIOD, so reaching
   the far end of a list does not mean tapping once per entry. */
#define MENU_REPEAT_DELAY  0.8f
#define MENU_REPEAT_PERIOD 0.125f

/* How far the stick has to stay pushed to count as still held. The edge
   libdragon reports arrives at half the stick's travel, so the repeat reads
   the same line: under it, the input that opened the step is over. */
#define STICK_REPEAT_THRESHOLD (JOYPAD_RANGE_N64_STICK_MAX / 2)


/* The stick drives the same four directions as the buttons. How far it is
   pushed is what the menu reads for speed; the frame it crosses into a
   direction is what it reads as a step, so one push moves the index once.

   Centred it does not rest at zero, it wanders by a couple of units: without
   the deadzone that wander reaches whatever rides on this, and the roll a
   button is driving loses a different sliver of its speed every frame. */
static float stick_getInput(int8_t axis)
{
	if (axis <= STICK_DEADZONE) return 0.0f;

	float input = (axis - STICK_DEADZONE)
	            / (float)(JOYPAD_RANGE_N64_STICK_MAX - STICK_DEADZONE);

	return (input > 1.0f) ? 1.0f : input;
}

typedef enum {

	MENU_DIR_NONE,
	MENU_DIR_UP, MENU_DIR_DOWN, MENU_DIR_LEFT, MENU_DIR_RIGHT,

} MenuDirection;

/* Only one direction repeats at a time. A menu moves by one step, so the last
   direction pressed is the one holding the clock: pressing another takes it
   over, and the wait starts again from there. */
static struct {

	MenuDirection direction;
	float elapsed;
	bool  repeating;

} menuRepeat;

/* Whether this direction hands the menu a step this frame: the frame it is
   pressed, and then once per period for as long as it is kept down. */
static bool direction_getStep(MenuDirection direction, bool pressed, bool held)
{
	if (pressed) {
		menuRepeat.direction = direction;
		menuRepeat.elapsed   = 0.0f;
		menuRepeat.repeating = false;
		return true;
	}

	/* Nobody owns the clock and this direction is still down: it takes it over
	   and waits the delay out again. This is the way back for a direction held
	   through another one: pressing a second direction takes the clock away,
	   and letting that one go leaves the first with no edge left to claim it. */
	if (menuRepeat.direction == MENU_DIR_NONE && held) {
		menuRepeat.direction = direction;
		menuRepeat.elapsed   = 0.0f;
		menuRepeat.repeating = false;
		return false;
	}

	if (menuRepeat.direction != direction) return false;

	if (!held) {
		menuRepeat.direction = MENU_DIR_NONE;
		return false;
	}

	menuRepeat.elapsed += time_get()->delta;

	float wait = menuRepeat.repeating ? MENU_REPEAT_PERIOD : MENU_REPEAT_DELAY;
	if (menuRepeat.elapsed < wait) return false;

	/* What is left over stays on the clock: dropping it would stretch every
	   period by whatever the frame overshot it by. */
	menuRepeat.elapsed  -= wait;
	menuRepeat.repeating = true;
	return true;
}

void menuControls_map(MenuControls *controls, const Controller *controller, const MenuControlBinding *binding)
{
	const bool up    = direction_getStep(MENU_DIR_UP,
		button_getPressed(controller, &controller->pressed, binding->up)
		|| controller->stick_pressed_y > 0,
		button_getPressed(controller, &controller->held, binding->up)
		|| controller->input.stick_y > STICK_REPEAT_THRESHOLD);

	const bool down  = direction_getStep(MENU_DIR_DOWN,
		button_getPressed(controller, &controller->pressed, binding->down)
		|| controller->stick_pressed_y < 0,
		button_getPressed(controller, &controller->held, binding->down)
		|| controller->input.stick_y < -STICK_REPEAT_THRESHOLD);

	const bool left  = direction_getStep(MENU_DIR_LEFT,
		button_getPressed(controller, &controller->pressed, binding->left)
		|| controller->stick_pressed_x < 0,
		button_getPressed(controller, &controller->held, binding->left)
		|| controller->input.stick_x < -STICK_REPEAT_THRESHOLD);

	const bool right = direction_getStep(MENU_DIR_RIGHT,
		button_getPressed(controller, &controller->pressed, binding->right)
		|| controller->stick_pressed_x > 0,
		button_getPressed(controller, &controller->held, binding->right)
		|| controller->input.stick_x > STICK_REPEAT_THRESHOLD);

	*controls = (MenuControls){
		.confirm   = button_getPressed(controller, &controller->pressed, binding->confirm),
		.cancel    = button_getPressed(controller, &controller->pressed, binding->cancel),
		.pause     = button_getPressed(controller, &controller->pressed, binding->pause),

		.up        = up,
		.down      = down,
		.left      = left,
		.right     = right,

		.up_held   = fmaxf(button_getPressed(controller, &controller->held, binding->up),
		                   stick_getInput( controller->input.stick_y)),
		.down_held = fmaxf(button_getPressed(controller, &controller->held, binding->down),
		                   stick_getInput(-controller->input.stick_y)),

		.tab_left  = button_getPressed(controller, &controller->pressed, binding->tab_left),
		.tab_right = button_getPressed(controller, &controller->pressed, binding->tab_right),
	};
}

void menuControl_update(Game *game)
{
	const GameStateDef *def = gameState_get(game->state);
	if (def->control) def->control(game);
}
