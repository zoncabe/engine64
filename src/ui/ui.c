#include <libdragon.h>
#include <t3d/t3danim.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/actor/actor.h"
#include "../../include/scene/scenery.h"
#include "../../include/graphics/font.h"
#include "../../include/graphics/sprites.h"
#include "../../include/graphics/shapes.h"
#include "../../include/render/render.h"
#include "../../include/game/game.h"
#include "../../include/ui/menu.h"
#include "../../include/ui/ui.h"
#include "../../include/player/player.h"


static const rdpq_textparms_t h14_parms = { .char_spacing = 0.95f };
static const rdpq_textparms_t h20_parms = { .char_spacing = 0.7f  };
static const rdpq_textparms_t h40_parms = { .char_spacing = 0.0f  };

static RenderContext main_menu_screen = {
    .element = {
        [0]  = { DRAW_RECTANGLE, .rectangle = { COLOR_GRADIENT, { 0.0f, 0.0f }, { 320.0f, 240.0f }, .gradient = { RGBA32(201, 121, 25, 255), RGBA32(223, 175, 117, 255), RGBA32(223, 175, 117, 255), RGBA32(201, 121, 25, 255) } } },
        [1]  = { DRAW_TEXT,      .text      = { HEADLINER_40, MENU_STYLE_NORMAL, {  43.0f,  55.0f }, "not a",   &h40_parms } },
        [2]  = { DRAW_TEXT,      .text      = { HEADLINER_40, MENU_STYLE_NORMAL, {  43.0f,  98.0f }, "GAME",    &h40_parms } },
        [3]  = { DRAW_TEXT,      .text      = { HEADLINER_20, MENU_STYLE_NORMAL, {  45.0f, 137.0f }, "Play",    &h20_parms } },
        [4]  = { DRAW_TEXT,      .text      = { HEADLINER_20, MENU_STYLE_NORMAL, {  45.0f, 162.0f }, "Options", &h20_parms } },
        [5]  = { DRAW_TEXT,      .text      = { HEADLINER_20, MENU_STYLE_NORMAL, {  45.0f, 187.0f }, "Credits", &h20_parms } },
        [6]  = { DRAW_TEXT,      .text      = { HEADLINER_14, 0,                 {  65.0f, 225.0f }, "Move",    &h14_parms } },
        [7]  = { DRAW_TEXT,      .text      = { HEADLINER_14, 0,                 { 115.0f, 225.0f }, "Select",  &h14_parms } },
        [8]  = { DRAW_SPRITE,    .sprite    = { SPR_BTN_A,   { 102.0f, 216.0f }, { 0.60f, 0.60f } } },
        [9]  = { DRAW_SPRITE,    .sprite    = { SPR_D_UP,    {  43.0f, 217.0f }, { 0.48f, 0.48f } } },
        [10] = { DRAW_SPRITE,    .sprite    = { SPR_D_DOWN,  {  53.0f, 217.0f }, { 0.48f, 0.48f } } },
        [11] = { DRAW_SPRITE,    .sprite    = { SPR_GORILLA, { 170.0f,   0.0f }, { 1.0f,  1.0f  } } },
    },
    .element_count = 12,
};

static RenderContext pause_screen = {
    .element = {
        [0]  = { DRAW_RECTANGLE, .rectangle = { COLOR_GRADIENT, { 0.0f, 0.0f }, { 320.0f, 240.0f } } },
        [1]  = { DRAW_TEXT,      .text      = { HEADLINER_20, MENU_STYLE_NORMAL, { 0.0f,  50.0f }, "Continue", &h20_parms } },
        [2]  = { DRAW_TEXT,      .text      = { HEADLINER_20, MENU_STYLE_NORMAL, { 0.0f,  80.0f }, "Options",  &h20_parms } },
        [3]  = { DRAW_TEXT,      .text      = { HEADLINER_20, MENU_STYLE_NORMAL, { 0.0f, 110.0f }, "Quit",     &h20_parms } },
        [4]  = { DRAW_TEXT,      .text      = { HEADLINER_14, 0,                 { 0.0f, 196.0f }, "Move",     &h14_parms } },
        [5]  = { DRAW_TEXT,      .text      = { HEADLINER_14, 0,                 { 0.0f, 211.0f }, "Select",   &h14_parms } },
        [6]  = { DRAW_TEXT,      .text      = { HEADLINER_14, 0,                 { 0.0f, 226.0f }, "Back",     &h14_parms } },
        [7]  = { DRAW_SPRITE,    .sprite    = { SPR_D_UP,   { 0.0f, 186.0f }, { 0.48f, 0.48f } } },
        [8]  = { DRAW_SPRITE,    .sprite    = { SPR_D_DOWN, { 0.0f, 187.0f }, { 0.48f, 0.48f } } },
        [9]  = { DRAW_SPRITE,    .sprite    = { SPR_BTN_A,  { 0.0f, 201.0f }, { 0.60f, 0.60f } } },
        [10] = { DRAW_SPRITE,    .sprite    = { SPR_BTN_B,  { 0.0f, 216.0f }, { 0.60f, 0.60f } } },
    },
    .element_count = 11,
};

const RenderContext* mainMenu_getRenderContext(void) { return &main_menu_screen; }
const RenderContext* pause_getRenderContext(void) { return &pause_screen; }

void mainMenu_animate()
{
    for (int i = 0; i < 3; i++)
        main_menu_screen.element[3 + i].text.style = (menu_getIndex() == i) ? MENU_STYLE_SELECTED : MENU_STYLE_NORMAL;
}

static const float pause_offsets_x[] = { 82.0f, 76.0f, 57.0f };

void pause_animate(const Game *game)
{
    if (game->state != PAUSE && game->state != GAMEPLAY) return;

    float t = game->transition.progress;
    pause_screen.element[0].rectangle.gradient[0] = RGBA32(0, 0, 0, (uint8_t)(0.3f * t * 255));
    pause_screen.element[0].rectangle.gradient[1] = RGBA32(0, 0, 0, (uint8_t)(t * 255));
    pause_screen.element[0].rectangle.gradient[2] = RGBA32(0, 0, 0, (uint8_t)(t * 255));
    pause_screen.element[0].rectangle.gradient[3] = RGBA32(0, 0, 0, (uint8_t)(0.3f * t * 255));

    for (int i = 0; i < 3; i++) {
        if (game->previous_state != PAUSE)
            pause_screen.element[1 + i].text.style = (menu_getIndex() == i) ? MENU_STYLE_SELECTED : MENU_STYLE_NORMAL;
        pause_screen.element[1 + i].text.position.x = 320.0f - pause_offsets_x[i] * t;
    }

    pause_screen.element[4].text.position.x = 347.0f - 85.0f * t;
    pause_screen.element[5].text.position.x = 338.0f - 76.0f * t;
    pause_screen.element[6].text.position.x = 338.0f - 76.0f * t;

    pause_screen.element[7].sprite.position.x  = 320.0f - 85.0f * t;
    pause_screen.element[8].sprite.position.x  = 330.0f - 85.0f * t;
    pause_screen.element[9].sprite.position.x  = 320.0f - 76.0f * t;
    pause_screen.element[10].sprite.position.x = 320.0f - 76.0f * t;
}

void ui_drawDebugData()
{
    rdpq_text_printf(NULL, DROID_SANS, 272, 20, "%d FPS", (int)(time_get()->rate));

    Player **p = player_get();
    if (p[0]) {
        T3DAnim    *idle = &p[0]->entity->actor->animation.animation[0];
        T3DSkeleton *sk  = &p[0]->entity->actor->animation.main;
        rdpq_text_printf(NULL, DROID_SANS, 10, 40, "idle playing=%d time=%.2f", idle->isPlaying, idle->time);
        rdpq_text_printf(NULL, DROID_SANS, 10, 55, "bone0 rot x=%.2f y=%.2f", sk->bones[0].rotation.v[0], sk->bones[0].rotation.v[1]);
    }
}

