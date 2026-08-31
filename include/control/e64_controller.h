#ifndef ENGINE64_CONTROLLER_H
#define ENGINE64_CONTROLLER_H

#include <stdbool.h>
#include <libdragon.h>

typedef enum {

	/* Unbound: reads as never pressed. Zero on purpose, so an action left
	   out of a binding comes out with no button instead of on the A. */
	BTN_NONE,

	BTN_A, BTN_B, BTN_Z, BTN_START,
	BTN_D_UP, BTN_D_DOWN, BTN_D_LEFT, BTN_D_RIGHT,
	BTN_C_UP, BTN_C_DOWN, BTN_C_LEFT, BTN_C_RIGHT,
	BTN_L, BTN_R,
	BTN_COUNT,

} ButtonID;

/* Centred, the stick does not rest at zero: it wanders by a couple of units.
   Under this it reads as centred, so that wander reaches nothing. */
#define STICK_DEADZONE 6

/* Who a binding belongs to. The port and the player are the same index: the
   first controller drives the first player. */
typedef enum {

	PLAYER_1, PLAYER_2, PLAYER_3, PLAYER_4,
	PLAYER_COUNT,

} PlayerID;

/* The controller as the hardware hands it over. What any of it means is the game's
   to decide: it holds its own bindings and its own set of actions, and reads
   them off this with button_getPressed. */
typedef struct Controller {

	joypad_buttons_t pressed;
	joypad_buttons_t held;
	joypad_buttons_t released;
	joypad_inputs_t  input;

	/* The frame the stick crosses into a direction, -1, 0 or +1 per axis.
	   A stick has no edge of its own, so libdragon keeps this one the way it
	   keeps a button's: it is what tells pushing it now from having held it
	   pushed since before. */
	int8_t stick_pressed_x;
	int8_t stick_pressed_y;

	/* No controller in this port. An absent one reads as all zeroes, which is a
	   valid answer for an edge but not for a continuous value: the seats
	   behind the first would write their zero over whatever it set. */
	bool connected;

} Controller;

#define CONTROLLER_COUNT PLAYER_COUNT

Controller *controller_get(void);
void controller_start(void);
void controller_poll(void);

float button_getPressed(const Controller *controller, const joypad_buttons_t *button, ButtonID id);

#endif
