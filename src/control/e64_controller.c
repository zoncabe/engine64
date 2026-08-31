#include <libdragon.h>

#include "control/e64_controller.h"


static Controller controller[CONTROLLER_COUNT];

Controller *controller_get(void) { return controller; }


static void controller_getInputs(Controller *controller, uint8_t port)
{
	controller->connected = joypad_is_connected(port);

	controller->pressed  = joypad_get_buttons_pressed(port);
	controller->held     = joypad_get_buttons_held(port);
	controller->released = joypad_get_buttons_released(port);
	controller->input    = joypad_get_inputs(port);

	controller->stick_pressed_x = joypad_get_axis_pressed(port, JOYPAD_AXIS_STICK_X);
	controller->stick_pressed_y = joypad_get_axis_pressed(port, JOYPAD_AXIS_STICK_Y);
}

/* How hard a bound button is being pushed, from 0 to 1. The C stick lives
   under the C buttons: an N64 controller drives it to the end of its travel
   and a GameCube one reports how far it really went. */
float button_getPressed(const Controller *controller, const joypad_buttons_t *button, ButtonID id)
{
	switch (id) {
		case BTN_A:       return button->a;
		case BTN_B:       return button->b;
		case BTN_Z:       return button->z;
		case BTN_START:   return button->start;
		case BTN_D_UP:    return button->d_up;
		case BTN_D_DOWN:  return button->d_down;
		case BTN_D_LEFT:  return button->d_left;
		case BTN_D_RIGHT: return button->d_right;
		case BTN_C_UP:    return controller->input.cstick_y < 0 ? -controller->input.cstick_y / (float)JOYPAD_RANGE_GCN_CSTICK_MAX : 0.0f;
		case BTN_C_DOWN:  return controller->input.cstick_y > 0 ?  controller->input.cstick_y / (float)JOYPAD_RANGE_GCN_CSTICK_MAX : 0.0f;
		case BTN_C_LEFT:  return controller->input.cstick_x < 0 ? -controller->input.cstick_x / (float)JOYPAD_RANGE_GCN_CSTICK_MAX : 0.0f;
		case BTN_C_RIGHT: return controller->input.cstick_x > 0 ?  controller->input.cstick_x / (float)JOYPAD_RANGE_GCN_CSTICK_MAX : 0.0f;
		case BTN_L:       return button->l;
		case BTN_R:       return button->r;
		default:          return 0.0f;
	}
}

void controller_start(void)
{
	for (int i = 0; i < CONTROLLER_COUNT; i++)
		controller[i] = (Controller){0};
}

void controller_poll(void)
{
	joypad_poll();
	for (int i = 0; i < CONTROLLER_COUNT; i++)
		controller_getInputs(&controller[i], i);
}
