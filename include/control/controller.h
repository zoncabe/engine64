#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <libdragon.h>

typedef enum {

	/* Unbound: reads as never pressed. Zero on purpose, so an action left
	   out of a binding comes out with no button instead of on the A. */
	BTN_NONE = 0,

	BTN_A, BTN_B, BTN_Z, BTN_START,
	BTN_D_UP, BTN_D_DOWN, BTN_D_LEFT, BTN_D_RIGHT,
	BTN_C_UP, BTN_C_DOWN, BTN_C_LEFT, BTN_C_RIGHT,
	BTN_L, BTN_R,
	BTN_COUNT,

} ButtonID;

/* Who a binding belongs to. The port and the player are the same index: the
   first pad drives the first player. */
typedef enum {

	PLAYER_1, PLAYER_2, PLAYER_3, PLAYER_4,
	PLAYER_COUNT,

} PlayerID;

/* The pad as the hardware hands it over. What any of it means is the game's
   to decide: it holds its own bindings and its own set of actions, and reads
   them off this with button_getPressed. */
typedef struct Controller {

	joypad_buttons_t pressed;
	joypad_buttons_t held;
	joypad_buttons_t released;
	joypad_inputs_t  input;

} Controller;

#define CONTROLLER_COUNT PLAYER_COUNT

Controller *controller_get(void);
void controller_start(void);
void controller_poll(void);

float button_getPressed(const Controller *pad, const joypad_buttons_t *button, ButtonID id);

#endif
