
#ifndef SPRITES_H
#define SPRITES_H

#include "../physics/math/vector2.h"

typedef enum {
    SPR_GORILLA,
    SPR_BTN_A,
    SPR_BTN_B,
    SPR_D_UP,
    SPR_D_DOWN,
    SPR_MOVE,
    SPR_SELECT,
    SPR_BACK,
    SPR_PLAY,
    SPR_OPTIONS,
    SPR_CREDITS,
    SPR_CONTINUE,
    SPR_QUIT,
    SPR_NOT_A,
    SPR_GAME,
    SPR_N64LOGO,
    SPR_LIBDRAGON,
    SPR_TINY3D,
    SPR_ZONCABE,
    SPRITE_ID_COUNT
} SpriteID;

typedef struct {
    SpriteID id;
    Vector2 position;
    Vector2 scale;
} Sprite;

void sprite_loadAsset(SpriteID id);
void sprite_unloadAsset(SpriteID id);
void sprite_setMode(void);
void sprite_draw(Sprite *list, int count);

#endif
