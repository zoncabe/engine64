#include <libdragon.h>
#include <t3d/t3danim.h>

#include "../../include/physics/physics.h"
#include "../../include/actor/actor.h"
#include "../../include/graphics/font.h"
#include "../../include/graphics/sprites.h"
#include "../../include/graphics/shapes.h"
#include "../../include/render/render.h"
#include "../../include/ui/menu.h"
#include "../../include/game/game.h"

static Menu menu;

int8_t menu_getIndex(void) { return menu.index; }
void menu_setIndex(int8_t index) { menu.index = index; }

void menu_moveIndex(int8_t delta, int8_t max) {
    menu.index += delta;
    if (menu.index < 0) menu.index = max;
    if (menu.index > max) menu.index = 0;
}

void menu_init()
{
    menu.index = 0;
    menu.top   = 0;
}

void menu_open(RenderContext *screen)
{
    if (menu.top < MENU_STACK_MAX) {
        menu.stack[menu.top++] = screen;
        menu.index = 0;
    }
}

void menu_back()
{
    if (menu.top > 0) {
        menu.top--;
        menu.index = 0;
    }
}

RenderContext *menu_current()
{
    if (menu.top == 0) return NULL;
    return menu.stack[menu.top - 1];
}
