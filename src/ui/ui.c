#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/actor/actor.h"
#include "../../include/ui/ui.h"
#include "../../include/ui/menu.h"
#include "../../include/player/player.h"
#include "../../include/light/lighting.h"
#include "../../include/graphics/font.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"
#include "../../include/scene/scenery.h"
#include "../../include/game/game.h"
#include "../../include/graphics/shapes.h"
#include "../../include/graphics/sprites.h"


#define MAIN_MENU_SPRITE_COUNT  11
Sprite main_menu_sprites[MAIN_MENU_SPRITE_COUNT] = {
    { .path = "rom:/textures/play.ia8.sprite",    .position = {45, 125},  .rotation = 0.0f, .scale = {0.34f, 0.34f} },  // 2 play
    { .path = "rom:/textures/options.ia8.sprite", .position = {45, 150},  .rotation = 0.0f, .scale = {0.30f, 0.30f} },  // 3 options
    { .path = "rom:/textures/credits.ia8.sprite", .position = {45, 175},  .rotation = 0.0f, .scale = {0.30f, 0.30f} },  // 4 credits
    
    { .path = "rom:/textures/not_a.ia8.sprite",   .position = {41,  18},  .rotation = 0.0f, .scale = {0.65f, 0.65f} },  // 0 not_a
    { .path = "rom:/textures/game.ia8.sprite",    .position = {39,  55},  .rotation = 0.0f, .scale = {0.45f, 0.45f} },  // 1 game
    { .path = "rom:/textures/move.ia8.sprite",    .position = {65, 217},  .rotation = 0.0f, .scale = {0.32f, 0.32f} },  // 5 move
    { .path = "rom:/textures/select.ia8.sprite",  .position = {115, 215}, .rotation = 0.0f, .scale = {0.32f, 0.32f} },  // 6 select
    { .path = "rom:/textures/AButton.sprite",     .position = {102, 216}, .rotation = 0.0f, .scale = {0.60f, 0.60f} },  // 7 abutton
    { .path = "rom:/textures/DUp.sprite",         .position = {43, 217},  .rotation = 0.0f, .scale = {0.48f, 0.48f} },  // 8 dup
    { .path = "rom:/textures/DDown.sprite",       .position = {53, 217},  .rotation = 0.0f, .scale = {0.48f, 0.48f} },  // 9 ddown
    { .path = "rom:/textures/gorilla.rgba32.sprite", .position = {170, 0}, .rotation = 0.0f, .scale = {1.0f, 1.0f} },  // 10 gorilla
};

#define PAUSE_SPRITE_COUNT      10
Sprite pause_sprites[PAUSE_SPRITE_COUNT] = {
    { .path = "rom:/textures/continue.ia8.sprite", .position = {45,  40},  .rotation = 0.0f, .scale = {0.33f, 0.33f} },  // 0 kontinue
    { .path = "rom:/textures/options.ia8.sprite",  .position = {45,  70},  .rotation = 0.0f, .scale = {0.30f, 0.30f} },  // 1 options
    { .path = "rom:/textures/quit.ia8.sprite",     .position = {45, 100},  .rotation = 0.0f, .scale = {0.30f, 0.30f} },  // 2 quit
    
    { .path = "rom:/textures/move.ia8.sprite",     .position = {0,  187},  .rotation = 0.0f, .scale = {0.32f, 0.32f} },  // 3 move
    { .path = "rom:/textures/select.ia8.sprite",   .position = {0,  200},  .rotation = 0.0f, .scale = {0.32f, 0.32f} },  // 4 select
    { .path = "rom:/textures/back.ia8.sprite",     .position = {0,  215},  .rotation = 0.0f, .scale = {0.32f, 0.32f} },  // 5 back
    { .path = "rom:/textures/DUp.sprite",          .position = {0,  186},  .rotation = 0.0f, .scale = {0.48f, 0.48f} },  // 6 dup
    { .path = "rom:/textures/DDown.sprite",        .position = {0,  187},  .rotation = 0.0f, .scale = {0.48f, 0.48f} },  // 7 ddown
    { .path = "rom:/textures/AButton.sprite",      .position = {0,  201},  .rotation = 0.0f, .scale = {0.60f, 0.60f} },  // 8 abutton
    { .path = "rom:/textures/BButton.sprite",      .position = {0,  216},  .rotation = 0.0f, .scale = {0.60f, 0.60f} },  // 9 bbutton
};

void ui_init()
{
    fonts_init();
    sprite_init(main_menu_sprites, MAIN_MENU_SPRITE_COUNT);
    sprite_init(pause_sprites,     PAUSE_SPRITE_COUNT);

    game.timer.mainMenu_transition = 1.0f;
    game.timer.gameplay_transition = 1.0f;
    game.timer.pause_transition    = 0.0f;
}

void ui_drawDebugData()
{
    //rdpq_text_printf(NULL, DROID_SANS, 270, 20, "%d FPS", (int)(timer.rate));
}

void ui_drawSlidingTransition(float progress, float r, float g, float b, uint8_t direction)
{
    switch(direction)
    {
        case UP:    shapes_drawSolidBox(r, g, b, 1.0f, 0.0f, 240.0f - (240.0f * progress), 320.0f, 240.0f); break;
        case DOWN:  shapes_drawSolidBox(r, g, b, 1.0f, 0.0f, 0.0f, 320.0f, 240.0f * progress);              break;
        case LEFT:  shapes_drawSolidBox(r, g, b, 1.0f, 320.0f - (320.0f * progress), 0.0f, 320.0f, 240.0f); break;
        case RIGHT: shapes_drawSolidBox(r, g, b, 1.0f, 0.0f, 0.0f, 320.0f * progress, 240.0f);              break;
    }
}

void ui_drawFadingTransition(float progress, float r, float g, float b)
{
    shapes_drawSolidBox(r, g, b, progress, 0.0f, 0.0f, 320.0f, 240.0f);
}

void ui_drawMainMenu()
{
    shapes_drawGradientBox(
        0.788f, 0.475f, 0.098f, 1.0f,
        0.875f, 0.686f, 0.459f, 1.0f,
        0.875f, 0.686f, 0.459f, 1.0f,
        0.788f, 0.475f, 0.098f, 1.0f,
        0.0f, 0.0f, 320.0f, 240.0f
    );

    sprite_setMode();

    for (int i = 0; i <= 2; i++) {
        
        if (menu.index == i && main_menu_sprites[i].scale.x < 0.34f) main_menu_sprites[i].scale.x += 0.01f;
        
        else if (menu.index != i && main_menu_sprites[i].scale.x > 0.30f) main_menu_sprites[i].scale.x -= 0.015f;

        main_menu_sprites[i].scale.y = main_menu_sprites[i].scale.x;
    }

    sprite_draw(main_menu_sprites, MAIN_MENU_SPRITE_COUNT);
}

void ui_drawPauseMenu()
{
    float t = game.timer.pause_transition;

    shapes_drawGradientBox(
        0.0f, 0.0f, 0.0f, 0.3f * t,
        0.0f, 0.0f, 0.0f, t,
        0.0f, 0.0f, 0.0f, t,
        0.0f, 0.0f, 0.0f, 0.3f * t,
        0.0f, 0.0f, 320.0f, 240.0f
    );

    sprite_setMode();

    for (int i = 0; i <= 2; i++) {

        if (menu.index == i && pause_sprites[i].scale.x < 0.33f) pause_sprites[i].scale.x += 0.01f;

        else if (menu.index != i && pause_sprites[i].scale.x > 0.30f) pause_sprites[i].scale.x -= 0.015f;

        pause_sprites[i].scale.y = pause_sprites[i].scale.x;
    }

    float offsets_x[] = { 92.0f, 83.0f, 57.0f };
    for (int i = 0; i <= 2; i++)
        pause_sprites[i].position.x = 320.0f - (offsets_x[i] * t);

    pause_sprites[3].position.x = 347.0f - (85.0f * t);
    pause_sprites[4].position.x = 338.0f - (76.0f * t);
    pause_sprites[5].position.x = 338.0f - (76.0f * t);
    pause_sprites[6].position.x = 320.0f - (85.0f * t);
    pause_sprites[7].position.x = 330.0f - (85.0f * t);
    pause_sprites[8].position.x = 320.0f - (76.0f * t);
    pause_sprites[9].position.x = 320.0f - (76.0f * t);

    sprite_draw(pause_sprites, PAUSE_SPRITE_COUNT);
}