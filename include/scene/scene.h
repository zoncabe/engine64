#ifndef SCENE_H
#define SCENE_H

#include "../entity/entity.h"
#include "../light/lighting.h"

#define SCENE_MAX_ACTORS 6
#define SCENE_MAX_SCENERY 12

#define SCENE_MAX_ENTITIES (SCENE_MAX_ACTORS + SCENE_MAX_SCENERY)


typedef struct {

    const char *model_path;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    const ActorMotionSettings *motion_settings;
    const ActorAnimationSettings *animation_settings;
    const AnimDef *anim_def;

} ActorDef;

typedef struct {

    const char *model_path;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;

} SceneryDef;

typedef struct {

    color_t ambient_color;
    DirectionalLight directional[DIRECTIONAL_LIGHT_COUNT];
    PointLight point[POINT_LIGHT_COUNT];

} LightDef;

typedef struct {

    LightDef light;
    ActorDef actor[SCENE_MAX_ACTORS];
    uint8_t actor_count;
    SceneryDef scenery[SCENE_MAX_SCENERY];
    uint8_t scenery_count;

} SceneDef;

typedef struct Scene {

    Entity *entity[SCENE_MAX_ENTITIES];
    uint8_t entity_count;

} Scene;

typedef enum {

    SCENE_DEMO,
    SCENE_COUNT,

} SceneID;


Scene *scene_get(void);
void scene_load(const SceneDef *def);
void scene_clear(void);
void scene_unload(void);
void scene_addEntity(Entity *e);

const SceneDef *sceneDef_get(SceneID id);

#endif
