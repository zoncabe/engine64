#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dskeleton.h>

#include "../../include/light/lighting.h"
#include "../../include/viewport/viewport.h"
#include "../../include/graphics/font.h"
#include "../../include/graphics/sprites.h"
#include "../../include/graphics/shapes.h"
#include "../../include/render/render.h"
#include "../../include/time/time.h"
#include "../../include/scene/scene.h"
#include "../../include/game/game.h"


void render_initContext(RenderContext *ctx)
{
    *ctx = (RenderContext){0};
}

void render_getScreenContext(RenderContext *ctx, const RenderContext *screen)
{
    memcpy(ctx->element + ctx->element_count,
           screen->element,
           screen->element_count * sizeof(DrawElement));
    ctx->element_count += screen->element_count;
}

static void render_setSceneContext(RenderContext *ctx, const Scene *s, uint8_t fb_index)
{
    for (uint8_t i = 0; i < s->entity_count; i++) {
        Entity    *e      = s->entity[i];
        RenderMesh *mesh  = e->mesh;
        T3DMat4FP  *matrix = mesh->matrix_buffer ? &mesh->matrix_buffer[fb_index] : NULL;
        T3DSkeleton *skel  = e->armature ? &e->armature->main : NULL;
        ctx->object[ctx->object_count++] = (T3DElement){ mesh->dl, matrix, skel };
    }
}

static void render_setTransitionContext(RenderContext *ctx, const GameTransition *t)
{
    if (!t->active || t->is_overlay) return;
    switch (t->type) {
        case TRANSITION_FADE:
            ctx->element[ctx->element_count++] = (DrawElement){
                .type = DRAW_RECTANGLE, .rectangle = { COLOR_SOLID, {0.0f, 0.0f}, {320.0f, 240.0f},
                        .color = RGBA32(0, 0, 0, (uint8_t)(t->progress * 255)) }
            };
            break;
    }
}

static void render_setDebugContext(RenderContext *ctx)
{
    static char fps_buf[8];
    snprintf(fps_buf, sizeof(fps_buf), "%d FPS", (int)time_get()->rate);
    ctx->element[ctx->element_count++] = (DrawElement){
        .type = DRAW_TEXT,
        .text = { DROID_SANS, 0, {272.0f, 20.0f}, fps_buf, NULL }
    };
}

void render_setContext(RenderContext *ctx, const Scene *scene, uint8_t fb_index, const GameTransition *transition, const RenderContext *screen)
{
    render_initContext(ctx);
    if (scene)      render_setSceneContext(ctx, scene, fb_index);
    if (screen)     render_getScreenContext(ctx, screen);
    if (transition) render_setTransitionContext(ctx, transition);
    render_setDebugContext(ctx);
}


static void render_start(int *fb_index)
{
    *fb_index = (*fb_index + 1) % FB_COUNT;
    viewport_clear();
}

static void render_end(void)
{
    rdpq_detach_show();
}


void render(RenderContext *ctx, int *fb_index)
{
    render_start(fb_index);

    if (ctx->object_count > 0) {
        light_set(light_get());
        for (uint8_t i = 0; i < ctx->object_count; i++) {
            T3DElement *obj = &ctx->object[i];
            if (obj->skeleton) t3d_skeleton_use(obj->skeleton);
            if (obj->matrix)   t3d_matrix_push(obj->matrix);
            rspq_block_run(obj->dl);
            if (obj->matrix)   t3d_matrix_pop(1);
        }
    }

    for (uint8_t i = 0; i < ctx->element_count; i++) {
        switch (ctx->element[i].type) {
            case DRAW_RECTANGLE: shapes_drawRectangle(&ctx->element[i].rectangle);          break;
            case DRAW_TEXT:      text_draw(&ctx->element[i].text, 1);                       break;
            case DRAW_SPRITE:    sprite_setMode(); sprite_draw(&ctx->element[i].sprite, 1); break;
        }
    }

    render_end();
}
