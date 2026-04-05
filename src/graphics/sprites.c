#include <libdragon.h>
#include "../../include/graphics/sprites.h"

static const char *sprite_paths[SPRITE_ID_COUNT] = {
    [SPR_GORILLA]   = "rom:/textures/gorilla.rgba32.sprite",
    [SPR_BTN_A]     = "rom:/textures/AButton.sprite",
    [SPR_BTN_B]     = "rom:/textures/BButton.sprite",
    [SPR_D_UP]      = "rom:/textures/DUp.sprite",
    [SPR_D_DOWN]    = "rom:/textures/DDown.sprite",
    [SPR_MOVE]      = "rom:/textures/move.ia8.sprite",
    [SPR_SELECT]    = "rom:/textures/select.ia8.sprite",
    [SPR_BACK]      = "rom:/textures/back.ia8.sprite",
    [SPR_PLAY]      = "rom:/textures/play.ia8.sprite",
    [SPR_OPTIONS]   = "rom:/textures/options.ia8.sprite",
    [SPR_CREDITS]   = "rom:/textures/credits.ia8.sprite",
    [SPR_CONTINUE]  = "rom:/textures/continue.ia8.sprite",
    [SPR_QUIT]      = "rom:/textures/quit.ia8.sprite",
    [SPR_NOT_A]     = "rom:/textures/not_a.ia8.sprite",
    [SPR_GAME]      = "rom:/textures/game.ia8.sprite",
    [SPR_N64LOGO]   = "rom:/textures/n64logo.sprite",
    [SPR_LIBDRAGON] = "rom:/textures/libdragon.sprite",
    [SPR_TINY3D]    = "rom:/textures/tiny3d.sprite",
    [SPR_ZONCABE]   = "rom:/textures/zoncabe.sprite",
};

static sprite_t *sprite[SPRITE_ID_COUNT];

void sprite_loadAsset(SpriteID id)
{
    sprite[id] = sprite_load(sprite_paths[id]);
}

void sprite_unloadAsset(SpriteID id)
{
    sprite_free(sprite[id]);
    sprite[id] = NULL;
}

void sprite_setMode()
{
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_alphacompare(1);
}

void sprite_draw(Sprite *list, int count)
{
    for (int i = 0; i < count; i++)
        rdpq_sprite_blit(sprite[list[i].id], list[i].position.x, list[i].position.y,
            &(rdpq_blitparms_t){ .scale_x = list[i].scale.x, .scale_y = list[i].scale.y });
}
