#ifndef SCREEN_H
#define SCREEN_H

#include "render/render.h"
#include "screen/screen_animation.h"

#define SCREEN_MAX_SECTION          8
#define SCREEN_SECTION_MAX_ELEMENT 64


typedef struct {

	DrawElement element[SCREEN_SECTION_MAX_ELEMENT];
	uint8_t       element_count;

	bool          has_scissor;
	float         scissor_x;
	float         scissor_y;
	float         scissor_w;
	float         scissor_h;

} ScreenSection;

typedef struct Screen {

	ScreenSection          section[SCREEN_MAX_SECTION];
	uint8_t                section_count;
	const ScreenAnimation *animation;

} Screen;

#endif
