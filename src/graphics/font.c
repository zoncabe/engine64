#include "../../include/graphics/font.h"


static rdpq_font_t *DroiSans;
static rdpq_font_t *Headliner10;
static rdpq_font_t *Headliner14;
static rdpq_font_t *Headliner20;
static rdpq_font_t *Headliner40;

void fonts_init()
{
    DroiSans    = rdpq_font_load("rom:/fonts/DroidSans.font64");
    Headliner10 = rdpq_font_load("rom:/fonts/Headliner10.font64");
    Headliner14 = rdpq_font_load("rom:/fonts/Headliner14.font64");
    Headliner20 = rdpq_font_load("rom:/fonts/Headliner20.font64");
    Headliner40 = rdpq_font_load("rom:/fonts/Headliner40.font64");

    rdpq_font_style(Headliner20, MENU_STYLE_NORMAL,   &(rdpq_fontstyle_t){RGBA32(200, 200, 200, 255)});
    rdpq_font_style(Headliner20, MENU_STYLE_SELECTED, &(rdpq_fontstyle_t){RGBA32(255, 220, 30,  255)});
    rdpq_font_style(Headliner40, MENU_STYLE_NORMAL,   &(rdpq_fontstyle_t){RGBA32(255, 255, 255, 200)});

    rdpq_text_register_font(DROID_SANS,   DroiSans);
    rdpq_text_register_font(HEADLINER_10, Headliner10);
    rdpq_text_register_font(HEADLINER_14, Headliner14);
    rdpq_text_register_font(HEADLINER_20, Headliner20);
    rdpq_text_register_font(HEADLINER_40, Headliner40);
}

void text_draw(const Text *list, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
        rdpq_text_printf(list[i].parms, list[i].font, list[i].position.x, list[i].position.y, "^%02d%s", list[i].style, list[i].text);
}
