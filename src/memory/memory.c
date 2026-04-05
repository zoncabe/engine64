#include <stdint.h>
#include "../../include/graphics/sprites.h"
#include "../../include/graphics/font.h"
#include "../../include/game/game_states.h"
#include "../../include/memory/memory.h"


static const SpriteID intro_sprites[] = {
    SPR_N64LOGO, SPR_LIBDRAGON, SPR_TINY3D, SPR_ZONCABE,
};
static const uint8_t intro_fonts[] = { DROID_SANS };

static const SpriteID mainmenu_sprites[] = {
    SPR_GORILLA, SPR_BTN_A, SPR_BTN_B, SPR_D_UP, SPR_D_DOWN,
    SPR_NOT_A, SPR_GAME, SPR_PLAY, SPR_OPTIONS, SPR_CREDITS,
};
static const uint8_t mainmenu_fonts[] = {
    DROID_SANS, HEADLINER_14, HEADLINER_20, HEADLINER_40,
};

static const uint8_t gameplay_fonts[] = { DROID_SANS };

static const SpriteID pause_sprites[] = {
    SPR_BTN_A, SPR_BTN_B, SPR_D_UP, SPR_D_DOWN, SPR_CONTINUE, SPR_QUIT,
};
static const uint8_t pause_fonts[] = {
    HEADLINER_14, HEADLINER_20,
};


typedef struct {
    const SpriteID *sprites;
    uint8_t         sprite_count;
    const uint8_t  *fonts;
    uint8_t         font_count;
} StateResources;

#define RESOURCES(spr, fnt) { spr, sizeof(spr)/sizeof(*spr), fnt, sizeof(fnt)/sizeof(*fnt) }
#define NO_RESOURCES        { NULL, 0, NULL, 0 }

static const StateResources state_resources[GAME_STATE_COUNT] = {
    [INTRO]     = RESOURCES(intro_sprites,    intro_fonts),
    [MAIN_MENU] = RESOURCES(mainmenu_sprites, mainmenu_fonts),
    [GAMEPLAY]  = { NULL, 0, gameplay_fonts, sizeof(gameplay_fonts)/sizeof(*gameplay_fonts) },
    [PAUSE]     = RESOURCES(pause_sprites,    pause_fonts),
    [GAME_OVER] = NO_RESOURCES,
};


void memory_load(GameState state)
{
    const StateResources *res = &state_resources[state];
    for (uint8_t i = 0; i < res->sprite_count; i++)
        sprite_loadAsset(res->sprites[i]);
    for (uint8_t i = 0; i < res->font_count; i++)
        font_loadAsset(res->fonts[i]);
}

void memory_unload(GameState state)
{
    const StateResources *res = &state_resources[state];
    for (uint8_t i = 0; i < res->sprite_count; i++)
        sprite_unloadAsset(res->sprites[i]);
    for (uint8_t i = 0; i < res->font_count; i++)
        font_unloadAsset(res->fonts[i]);
}
