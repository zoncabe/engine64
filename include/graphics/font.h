
#ifndef FONT_H
#define FONT_H

#include <libdragon.h> // rdpq_textparms_t
#include "../physics/math/vector2.h"

#define DROID_SANS    1
#define HEADLINER_10  2
#define HEADLINER_14  3
#define HEADLINER_20  4
#define HEADLINER_40  5

#define MENU_STYLE_NORMAL   1
#define MENU_STYLE_SELECTED 2

typedef struct {
    uint8_t                  font;
    uint8_t                  style;
    Vector2                  position;
    const char              *text;
    const rdpq_textparms_t  *parms;
} Text;

void fonts_init();
void text_draw(const Text *list, uint8_t count);

#endif