#include "ui/ui.h"
#include "scene2d/scene2d.h"
#include "time/time.h"


static UIAnimationPlayer ui_player;


void ui_play(const UIAnimation *animation, bool reversed)
{
	uiAnimationPlayer_start(&ui_player, scene2d_get(), animation, UI_ANIMATION_PLAY_ONCE, reversed);
}

bool ui_isTransitioning(void)
{
	return ui_player.is_active;
}

void ui_update(const UIAnimation *idle)
{
	uiAnimationPlayer_update(&ui_player, scene2d_get(), time_get()->delta);

	if (idle) uiAnimation_apply(scene2d_get(), idle, 0.0f);
}
