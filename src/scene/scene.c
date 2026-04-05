#include "../../include/viewport/viewport.h"
#include "../../include/light/lighting.h"
#include "../../include/player/player.h"
#include "../../include/entity/entity.h"
#include "../../include/scene/scene.h"
#include "../../include/scene/demo_scene.h"


static Scene scene;

Scene *scene_get(void) { return &scene; }


void scene_load(const SceneDef *def)
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


void scene_clear(void)
{
    scene = (Scene){0};
}

void scene_unload(void)
{
    for (uint8_t i = 0; i < scene.entity_count; i++)
        if (scene.entity[i]->type != ENTITY_ACTOR)
            entity_delete(scene.entity[i]);
    scene_clear();
}

const SceneDef *sceneDef_get(SceneID id)
{
    static const SceneDef * const table[SCENE_COUNT] = {
        [SCENE_DEMO] = &demo_scene,
    };
    return table[id];
}
