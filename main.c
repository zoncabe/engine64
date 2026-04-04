#include <libdragon.h>

#include "include/game/game.h"
#include "include/scene/scene.h"


int main()
{
    debug_init_isviewer();
    debug_init_usblog();

    game_init();
    scene_init();

    for (;;)
        game_runFrame();

    game_close();

    return 0;
}
