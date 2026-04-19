#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <libdragon.h>

typedef enum {

	BTN_A, BTN_B, BTN_Z, BTN_START,
	BTN_D_UP, BTN_D_DOWN, BTN_D_LEFT, BTN_D_RIGHT,
	BTN_L, BTN_R,
	BTN_COUNT,

} ButtonID;

typedef struct {

	ButtonID confirm;
	ButtonID cancel;
	ButtonID pause;
	ButtonID menu_up;
	ButtonID menu_down;
	ButtonID menu_left;
	ButtonID menu_right;
	ButtonID menu_tab_left;
	ButtonID menu_tab_right;
	ButtonID jump;
	ButtonID roll;
	ButtonID sprint;
	ButtonID camera_aim;

} ControllerBinding;

typedef struct ControllerActions {

	bool  confirm;
	bool  cancel;
	bool  pause;
	bool  menu_up;
	bool  menu_down;
	bool  menu_left;
	bool  menu_right;
	bool  menu_tab_left;
	bool  menu_tab_right;
	bool  jump;
	bool  jump_held;
	bool  roll;
	bool  sprint;
	bool  camera_aim;
	float stick_x;
	float stick_y;
	float cstick_x;
	float cstick_y;
	
} ControllerActions;

typedef struct {

	joypad_buttons_t pressed;
	joypad_buttons_t held;
	joypad_inputs_t  input;

} ControllerData;

typedef struct {

	ControllerData    raw;
	ControllerActions actions;
	ControllerBinding binding;

} Controller;

#define CONTROLLER_COUNT 1

Controller *controller_get(void);
void controller_start(void);
void controller_poll(void);

#endif
