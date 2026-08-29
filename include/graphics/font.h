#ifndef FONT_H
#define FONT_H

#include <libdragon.h>
#include "physics/math/vector2.h"


typedef struct {

	uint8_t          id;      /* rdpq style id */
	rdpq_fontstyle_t style;   /* color, outline color, custom callback */

} FontStyle;

typedef struct {

	const char      *path;   /* NULL = unused slot (0 is: rdpq font ids start at 1) */
	const FontStyle *style;
	uint8_t          style_count;

} FontDef;

typedef struct {
	uint8_t                  font;
	uint8_t                  style;
	const char              *text;
	const rdpq_textparms_t  *parms;
} Text;


/* Hands the engine the game's font table, indexed by font id. Runs once,
   before any state loads resources. */
void font_init(const FontDef *fonts, uint8_t count);

void font_loadAsset(uint8_t id);
void font_unloadAsset(uint8_t id);
void text_draw(const Text *element, Vector2 position);

#endif
