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
#include "../../include/ui/menu.h"
#include "../../include/graphics/sprites.h"
#include "../../include/graphics/shapes.h"
#include "../../include/cutscene/intro.h"

#define INTRO_SPRITE_COUNT 4

static SpriteID intro_ids[INTRO_SPRITE_COUNT] = {

	SPR_N64LOGO,
	SPR_LIBDRAGON,
	SPR_TINY3D,
	SPR_ZONCABE
};

float intro_transitions[INTRO_SPRITE_COUNT + 1] = { 3.00f, 5.00f, 7.00f, 10.0f };

static RenderContext intro_screen = {0};

const RenderContext* intro_getRenderContext(void) { return &intro_screen; }

void intro_init()
{
}

void intro_close()
{
}

void intro_animate(float t)
{
    intro_screen = (RenderContext){0};
    float start = 0.75f;

    for (int i = 0; i < INTRO_SPRITE_COUNT; i++) {
        float end = intro_transitions[i];

        if (t >= start && t < end) {
            intro_screen.element[0] = (DrawElement){
                .type = DRAW_SPRITE, .sprite = { intro_ids[i], { 0.0f, 0.0f }, { 1.0f, 1.0f } }
            };
            intro_screen.element_count = 1;

            float fade_progress = 0.0f;
            float local_t = t - start;
            float duration = end - start;

            if (local_t < 0.25f) {
                fade_progress = 1.0f - (local_t * 4.0f);
            } else if (local_t > (duration - (i == 3 ? 0.5f : 0.25f))) {
                float fade_speed = (i == 3) ? 2.0f : 4.0f;
                fade_progress = (t - (end - (1.0f / fade_speed))) * fade_speed;
            }

            if (fade_progress > 0.0f) {
                intro_screen.element[1] = (DrawElement){
                    .type = DRAW_RECTANGLE,
                    .rectangle = { .fill = COLOR_SOLID, .position = { 0.0f, 0.0f }, .size = { 320.0f, 240.0f }, .color = RGBA32(0, 0, 0, (uint8_t)(fade_progress * 255)) }
                };
                intro_screen.element_count = 2;
            }
        }

        start = end;
    }
}
