#include <assert.h>
#include "graphics/font.h"


static const char *font_paths[] = {

	[DROID_SANS]   = "rom:/fonts/DroidSans.font64",
	[XOLONIUM_10]  = "rom:/fonts/Xolonium10.font64",
	[XOLONIUM_14]  = "rom:/fonts/Xolonium14.font64",
	[XOLONIUM_20]  = "rom:/fonts/Xolonium20.font64",
	[XOLONIUM_40]  = "rom:/fonts/Xolonium40.font64",
	[XOLONIUM_60]  = "rom:/fonts/Xolonium60.font64",

};

static rdpq_font_t *fonts[7];


void font_loadAsset(uint8_t id)
{
	fonts[id] = rdpq_font_load(font_paths[id]);
	assert(fonts[id]);

	if (id == XOLONIUM_14) {
		rdpq_font_style(fonts[id], MENU_STYLE_NORMAL,   &(rdpq_fontstyle_t){RGBA32(200, 200, 200, 255)});
		rdpq_font_style(fonts[id], MENU_STYLE_SELECTED, &(rdpq_fontstyle_t){RGBA32(255, 220, 30,  255)});
	}
	if (id == XOLONIUM_20) {
		rdpq_font_style(fonts[id], MENU_STYLE_NORMAL,   &(rdpq_fontstyle_t){RGBA32(200, 200, 200, 255)});
		rdpq_font_style(fonts[id], MENU_STYLE_SELECTED, &(rdpq_fontstyle_t){RGBA32(255, 220, 30,  255)});
	}
	if (id == XOLONIUM_40) {
		rdpq_font_style(fonts[id], MENU_STYLE_NORMAL,   &(rdpq_fontstyle_t){RGBA32(255, 255, 255, 200)});
		rdpq_font_style(fonts[id], TEXT_STYLE_RED,      &(rdpq_fontstyle_t){RGBA32(230, 30, 30, 255)});
	}
	if (id == XOLONIUM_60) {
		rdpq_font_style(fonts[id], MENU_STYLE_NORMAL,   &(rdpq_fontstyle_t){RGBA32(255, 255, 255, 200)});
		rdpq_font_style(fonts[id], TEXT_STYLE_RED,      &(rdpq_fontstyle_t){RGBA32(230, 30, 30, 255)});
	}

	rdpq_text_register_font(id, fonts[id]);
}

void font_unloadAsset(uint8_t id)
{
	rdpq_text_unregister_font(id);
	rdpq_font_free(fonts[id]);
	fonts[id] = NULL;
}

void text_draw(const Text *element, Vector2 position)
{
	rdpq_text_printf(element->parms, element->font, position.x, position.y, "^%02d%s", element->style, element->text);
}
