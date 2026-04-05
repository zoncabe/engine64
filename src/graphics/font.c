#include "../../include/graphics/font.h"


static const char *font_paths[] = {
    [DROID_SANS]   = "rom:/fonts/DroidSans.font64",
    [HEADLINER_10] = "rom:/fonts/Headliner10.font64",
    [HEADLINER_14] = "rom:/fonts/Headliner14.font64",
    [HEADLINER_20] = "rom:/fonts/Headliner20.font64",
    [HEADLINER_40] = "rom:/fonts/Headliner40.font64",
};

static rdpq_font_t *fonts[6];

void font_loadAsset(uint8_t id)
{
    fonts[id] = rdpq_font_load(font_paths[id]);

    if (id == HEADLINER_20) {
        rdpq_font_style(fonts[id], MENU_STYLE_NORMAL,   &(rdpq_fontstyle_t){RGBA32(200, 200, 200, 255)});
        rdpq_font_style(fonts[id], MENU_STYLE_SELECTED, &(rdpq_fontstyle_t){RGBA32(255, 220, 30,  255)});
    }
    if (id == HEADLINER_40)
        rdpq_font_style(fonts[id], MENU_STYLE_NORMAL,   &(rdpq_fontstyle_t){RGBA32(255, 255, 255, 200)});

    rdpq_text_register_font(id, fonts[id]);
}

void font_unloadAsset(uint8_t id)
{
    rdpq_text_unregister_font(id);
    rdpq_font_free(fonts[id]);
    fonts[id] = NULL;
}

void text_draw(const Text *list, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
        rdpq_text_printf(list[i].parms, list[i].font, list[i].position.x, list[i].position.y, "^%02d%s", list[i].style, list[i].text);
}
