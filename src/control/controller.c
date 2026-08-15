#include <libdragon.h>

#include "control/controller.h"


static Controller controller[CONTROLLER_COUNT];

Controller *controller_get(void) { return controller; }


static void controllerData_getInputs(ControllerData *data, uint8_t port)
{
	data->pressed = joypad_get_buttons_pressed(port);
	data->held    = joypad_get_buttons_held(port);
	data->input   = joypad_get_inputs(port);
}

static bool button_getPressed(const joypad_buttons_t *button, ButtonID id)
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
		case BTN_L:       return button->l;
		case BTN_R:       return button->r;
		default:          return false;
	}
}

static void controller_mapActions(Controller *controller)
{
	const ControllerBinding *bind = &controller->binding;
	const ControllerData    *raw  = &controller->raw;

	controller->actions = (ControllerActions){
		.confirm    = button_getPressed(&raw->pressed, bind->confirm),
		.cancel     = button_getPressed(&raw->pressed, bind->cancel),
		.pause      = button_getPressed(&raw->pressed, bind->pause),
		.menu_up        = button_getPressed(&raw->pressed, bind->menu_up),
		.menu_down      = button_getPressed(&raw->pressed, bind->menu_down),
		.menu_up_held   = button_getPressed(&raw->held,    bind->menu_up),
		.menu_down_held = button_getPressed(&raw->held,    bind->menu_down),
		.menu_left      = button_getPressed(&raw->pressed, bind->menu_left),
		.menu_right     = button_getPressed(&raw->pressed, bind->menu_right),
		.menu_tab_left  = button_getPressed(&raw->pressed, bind->menu_tab_left),
		.menu_tab_right = button_getPressed(&raw->pressed, bind->menu_tab_right),
		.jump       = button_getPressed(&raw->pressed, bind->jump),
		.jump_held  = button_getPressed(&raw->held,    bind->jump),
		.roll       = button_getPressed(&raw->pressed, bind->roll),
		.sprint     = button_getPressed(&raw->held,    bind->sprint),
		.camera_aim = button_getPressed(&raw->held,    bind->camera_aim),
		.switch_character_next = button_getPressed(&raw->pressed, bind->switch_character_next),
		.switch_character_prev = button_getPressed(&raw->pressed, bind->switch_character_prev),
		.stick_x    = raw->input.stick_x,
		.stick_y    = raw->input.stick_y,
		.cstick_x   = raw->input.cstick_x,
		.cstick_y   = raw->input.cstick_y,
	};
}

static const ControllerBinding default_binding = {
	.confirm    = BTN_A,
	.cancel     = BTN_B,
	.pause      = BTN_START,
	.menu_up        = BTN_D_UP,
	.menu_down      = BTN_D_DOWN,
	.menu_left      = BTN_D_LEFT,
	.menu_right     = BTN_D_RIGHT,
	.menu_tab_left  = BTN_L,
	.menu_tab_right = BTN_R,
	.jump       = BTN_A,
	.roll       = BTN_B,
	.sprint     = BTN_R,
	.camera_aim = BTN_Z,
	.switch_character_next = BTN_D_RIGHT,
	.switch_character_prev = BTN_D_LEFT,
};

void controller_start(void)
{
	for (int i = 0; i < CONTROLLER_COUNT; i++)
		controller[i] = (Controller){ .binding = default_binding };
}

void controller_poll(void)
{
	joypad_poll();
	for (int i = 0; i < CONTROLLER_COUNT; i++) {
		controllerData_getInputs(&controller[i].raw, i);
		controller_mapActions(&controller[i]);
	}
}
