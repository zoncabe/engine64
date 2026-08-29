#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <libdragon.h>

typedef enum {

	BTN_A, BTN_B, BTN_Z, BTN_START,
	BTN_D_UP, BTN_D_DOWN, BTN_D_LEFT, BTN_D_RIGHT,
	BTN_C_UP, BTN_C_DOWN, BTN_C_LEFT, BTN_C_RIGHT,
	BTN_L, BTN_R,
	BTN_COUNT,

	/* Unbound: reads as never pressed. Parks an action that keeps its
	   algorithm but currently has no button. */
	BTN_NONE = BTN_COUNT,

} ButtonID;

/* The pad as the hardware hands it over. What any of it means is the game's
   to decide: it holds its own bindings and its own set of actions, and reads
   them off this with button_getPressed. */
typedef struct Controller {

	joypad_buttons_t pressed;
	joypad_buttons_t held;
	joypad_buttons_t released;
	joypad_inputs_t  input;

} Controller;

#define CONTROLLER_COUNT 1

Controller *controller_get(void);
void controller_start(void);
void controller_poll(void);

float button_getPressed(const Controller *pad, const joypad_buttons_t *button, ButtonID id);

#endif
