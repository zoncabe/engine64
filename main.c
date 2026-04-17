#include <libdragon.h>

#include "include/game/game.h"


int main()
{
    debug_init_isviewer();
    debug_init_usblog();

    game_init();

    for (;;)
        game_runFrame();

    game_close();

    return 0;
}
