#ifndef ENGINE64_UI_H
#define ENGINE64_UI_H

#include <stdbool.h>

#include "ui/e64_ui_animation.h"


/* The interface on screen: one animation plays at a time over the live 2D
   scene, so the engine keeps a single player for all of them. */

void ui_play(const UIAnimation *animation, bool reversed);

/* True while an animation plays: controls gate their input on this, and the
   state machinery waits on it before switching. */
bool ui_isTransitioning(void);

/* Advances what is playing and applies the animation that runs every frame,
   the one the live lookups live in. NULL for none. */
void ui_update(const UIAnimation *idle);

#endif
