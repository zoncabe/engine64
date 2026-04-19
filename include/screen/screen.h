#ifndef SCREEN_H
#define SCREEN_H

#include "render/render.h"
#include "screen/screen_animation.h"

/*
 * ScreenElement is an alias for DrawElement.
 *
 * The screen module builds and animates elements; the render module
 * consumes them as its native draw primitive. Both modules share the
 * exact same struct, only the name changes to match the vocabulary
 * of each layer.
 *
 * If the two layers ever need to diverge, break the alias and give
 * each its own struct; no conversion code is needed today.
 */
typedef DrawElement ScreenElement;


#define SCREEN_MAX_SECTION          8
#define SCREEN_SECTION_MAX_ELEMENT 64


typedef struct {

	ScreenElement element[SCREEN_SECTION_MAX_ELEMENT];
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
