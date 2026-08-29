#include <assert.h>
#include <malloc.h>
#include "graphics/font.h"


/* The game's table, handed over at font_init. */
static const FontDef *font_def;
static uint8_t        font_count;

static rdpq_font_t **font;


void font_init(const FontDef *fonts, uint8_t count)
{
	font_def   = fonts;
	font_count = count;

	font = calloc(count, sizeof(rdpq_font_t *));
	assert(font);
}

void font_loadAsset(uint8_t id)
{
	assert(id < font_count && font_def[id].path);

	const FontDef *def = &font_def[id];

	font[id] = rdpq_font_load(def->path);
	assert(font[id]);

	for (int i = 0; i < def->style_count; i++)
		rdpq_font_style(font[id], def->style[i].id, &def->style[i].style);

	rdpq_text_register_font(id, font[id]);
}

void font_unloadAsset(uint8_t id)
{
	rdpq_text_unregister_font(id);
	rdpq_font_free(font[id]);
	font[id] = NULL;
}

void text_draw(const Text *element, Vector2 position)
{
	rdpq_text_printf(element->parms, element->font, position.x, position.y, "^%02d%s", element->style, element->text);
}
