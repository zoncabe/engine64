#include "resources/resources.h"

#define RESOURCES(spr, fnt) { spr, sizeof(spr)/sizeof(*spr), fnt, sizeof(fnt)/sizeof(*fnt) }
#define NO_RESOURCES        { NULL, 0, NULL, 0 }


static const SpriteID intro_sprite[] = {
	SPRITE_N64LOGO, SPRITE_LIBDRAGON, SPRITE_TINY3D, SPRITE_ZONCABE,
};

static const uint8_t intro_font[] = { DROID_SANS };

static const SpriteID mainmenu_sprite[] = {
	SPRITE_GORILLA, SPRITE_BTN_A, SPRITE_BTN_B, SPRITE_D_UP, SPRITE_D_DOWN,
};

static const uint8_t mainmenu_font[] = {
	DROID_SANS, HEADLINER_14, HEADLINER_20, HEADLINER_40,
};

static const uint8_t gameplay_font[] = { DROID_SANS };

static const SpriteID pause_sprite[] = {
	SPRITE_BTN_A, SPRITE_BTN_B, SPRITE_D_UP, SPRITE_D_DOWN,
};

static const uint8_t pause_font[] = {
	HEADLINER_14, HEADLINER_20,
};

static const ResourceSet state_resource[GAME_STATE_COUNT] = {

	[GAME_STATE_INTRO]     = RESOURCES(intro_sprite,    intro_font),
	[GAME_STATE_MAIN_MENU] = RESOURCES(mainmenu_sprite, mainmenu_font),
	[GAME_STATE_GAMEPLAY]  = { NULL, 0, gameplay_font, sizeof(gameplay_font)/sizeof(*gameplay_font) },
	[GAME_STATE_PAUSE]     = RESOURCES(pause_sprite,    pause_font),
	[GAME_STATE_GAME_OVER] = NO_RESOURCES,
};


const ResourceSet *resources_forState(GameState state)
{
	return &state_resource[state];
}

void resources_load(const ResourceSet *set)
{
	if (!set) return;
	for (uint8_t i = 0; i < set->sprite_count; i++)
		sprite_loadAsset(set->sprite[i]);
	for (uint8_t i = 0; i < set->font_count; i++)
		font_loadAsset(set->font[i]);
}

void resources_unload(const ResourceSet *set)
{
	if (!set) return;
	for (uint8_t i = 0; i < set->sprite_count; i++)
		sprite_unloadAsset(set->sprite[i]);
	for (uint8_t i = 0; i < set->font_count; i++)
		font_unloadAsset(set->font[i]);
}
