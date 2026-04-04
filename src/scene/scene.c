#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

#include "../../include/time/time.h"
#include "../../include/physics/physics.h"
#include "../../include/control/control.h"
#include "../../include/light/lighting.h"
#include "../../include/camera/camera.h"
#include "../../include/viewport/viewport.h"
#include "../../include/actor/actor.h"
#include "../../include/player/player.h"
#include "../../include/control/player_control.h"
#include "../../include/player/player_animation.h"
#include "../../include/graphics/font.h"
#include "../../include/graphics/sprites.h"
#include "../../include/graphics/shapes.h"
#include "../../include/render/render.h"
#include "../../include/ui/ui.h"
#include "../../include/ui/menu.h"
#include "../../include/game/game.h"
#include "../../include/game/game_states.h"
#include "../../include/assets/male_muscled.h"
#include "../../include/memory/memory.h"
#include "../../include/scene/scene.h"


static Scene scene;

Scene *scene_get(void) { return &scene; }


static void scene_load(const SceneDef *def)
{
    light_initAmbient(def->light.ambient_color);
    for (uint8_t i = 0; i < DIRECTIONAL_LIGHT_COUNT; i++)
        light_initDirectional(&light_get()->directional[i], def->light.directional[i].direction, def->light.directional[i].color);
    for (uint8_t i = 0; i < POINT_LIGHT_COUNT; i++)
        light_initPoint(&light_get()->point[i], def->light.point[i].position, def->light.point[i].color, def->light.point[i].size);

    scene = (Scene){0};

    Player **player = player_get();
    for (uint8_t i = 0; i < def->actor_count; i++) {
        const ActorDef *a = &def->actor[i];
        player[i] = player_create(a->model_path, a->motion_settings, a->animation_settings);
        player[i]->entity->transform.position = a->position;
        player[i]->entity->transform.rotation = a->rotation;
        player[i]->entity->transform.scale    = a->scale;
        scene.entity[scene.entity_count++] = player[i]->entity;
    }

    for (uint8_t i = 0; i < def->scenery_count; i++) {
        const SceneryDef *s = &def->scenery[i];
        Entity *e = entity_create(ENTITY_SCENERY, s->model_path, NULL, NULL);
        e->transform.position = s->position;
        e->transform.rotation = s->rotation;
        e->transform.scale    = s->scale;
        for (uint8_t fb = 0; fb < FB_COUNT; fb++)
            renderMesh_buildMatrix(e->mesh, &e->transform, fb);
        scene.entity[scene.entity_count++] = e;
    }
}


void scene_init(void)
{
    const SceneDef def = {
        .light = {
            .ambient_color = {60, 60, 60, 0xFF},
            .directional = {
                [0] = { .color = {210, 210, 210, 0xFF}, .direction = {{1.0f, -1.0f, 1.0f}} },
            },
        },
        .actor = {
            [0] = {
                .model_path         = male_muscled_model,
                .position           = {-210.0f, -210.0f, 0.0f},
                .rotation           = {0.0f, 0.0f, 200.0f},
                .scale              = {1.0f, 1.0f, 1.0f},
                .motion_settings    = male_muscled_getMotionSettings(),
                .animation_settings = male_muscled_getAnimationSettings(),
            },
        },
        .actor_count = 1,
        .scenery = {
            [0] = { "rom:/models/room.t3dm",  {0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f} },
            [1] = { "rom:/models/axis.t3dm",  {0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 0.0f}, {1.5f, 1.5f, 1.5f} },
        },
        .scenery_count = 2,
    };

    scene_load(&def);
}
