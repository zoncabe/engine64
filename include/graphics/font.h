#ifndef FONT_H
#define FONT_H

#include <libdragon.h>
#include "physics/math/vector2.h"

#define DROID_SANS    1
#define XOLONIUM_10   2
#define XOLONIUM_14   3
#define XOLONIUM_20   4
#define XOLONIUM_40   5
#define XOLONIUM_60   6

#define MENU_STYLE_NORMAL   1
#define MENU_STYLE_SELECTED 2
#define TEXT_STYLE_RED      3


typedef struct {
	uint8_t                  font;
	uint8_t                  style;
	const char              *text;
	const rdpq_textparms_t  *parms;
} Text;


void font_loadAsset(uint8_t id);
void font_unloadAsset(uint8_t id);
void text_draw(const Text *element, Vector2 position);

#endif