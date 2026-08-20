#include "resources/resources.h"

#define RESOURCES(spr, fnt) { spr, sizeof(spr)/sizeof(*spr), fnt, sizeof(fnt)/sizeof(*fnt) }
#define NO_RESOURCES        { NULL, 0, NULL, 0 }


static const SpriteID intro_sprite[] = {
	SPRITE_LIBDRAGON, SPRITE_TINY3D,
};

static const uint8_t intro_font[] = { DROID_SANS, XOLONIUM_60 };

static const SpriteID mainmenu_sprite[] = {
	SPRITE_GORILLA, SPRITE_BTN_A, SPRITE_BTN_B, SPRITE_D_UP, SPRITE_D_DOWN, SPRITE_D_LEFT, SPRITE_D_RIGHT,
};

static const uint8_t mainmenu_font[] = {
	DROID_SANS, XOLONIUM_14, XOLONIUM_20, XOLONIUM_40,
};

static const SpriteID credits_sprite[] = {
	SPRITE_BTN_B, SPRITE_D_UP, SPRITE_D_DOWN,
};

static const uint8_t credits_font[] = {
	DROID_SANS, XOLONIUM_14, XOLONIUM_40,
};

/* The pause overlays gameplay and can open on any frame: its assets ride
   the gameplay set, loaded and freed with it, so no heap traffic happens
   mid-session. */
static const SpriteID gameplay_sprite[] = {
	SPRITE_CIRCLE_MASK, SPRITE_CIRCLE_PROGRESS,
	SPRITE_BTN_A, SPRITE_BTN_B, SPRITE_D_UP, SPRITE_D_DOWN, SPRITE_D_LEFT, SPRITE_D_RIGHT,
};

static const uint8_t gameplay_font[] = { DROID_SANS, XOLONIUM_14, XOLONIUM_20 };

static const ResourceSet state_resource[GAME_STATE_COUNT] = {

	[GAME_STATE_INTRO]     = RESOURCES(intro_sprite,    intro_font),
	[GAME_STATE_MAIN_MENU] = RESOURCES(mainmenu_sprite, mainmenu_font),
	[GAME_STATE_CREDITS]   = RESOURCES(credits_sprite,  credits_font),
	[GAME_STATE_GAMEPLAY]  = RESOURCES(gameplay_sprite, gameplay_font),
	[GAME_STATE_PAUSE]     = NO_RESOURCES,
	[GAME_STATE_GAME_OVER] = NO_RESOURCES,
};


const ResourceSet *resources_forState(GameState state)
{
	return &state_resource[state];
}

void resources_load(const ResourceSet *set)
{
	if (!set) return;
	for (int i = 0; i < set->sprite_count; i++)
		sprite_loadAsset(set->sprite[i]);
	for (int i = 0; i < set->font_count; i++)
		font_loadAsset(set->font[i]);
}

void resources_unload(const ResourceSet *set)
{
	if (!set) return;
	for (int i = 0; i < set->sprite_count; i++)
		sprite_unloadAsset(set->sprite[i]);
	for (int i = 0; i < set->font_count; i++)
		font_unloadAsset(set->font[i]);
}
