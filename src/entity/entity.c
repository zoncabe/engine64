#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "../../include/entity/entity.h"
#include "../../include/actor/actor_states.h"
#include "../../include/actor/actor_animation.h"
#include "../../include/viewport/viewport.h"


void entity_init(Entity *entity, EntityType type, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings)
{
    *entity = (Entity){ .type = type };
    transform_init(&entity->transform);

    if (type == ENTITY_ACTOR) {
        entity->physics   = malloc(sizeof(RigidBody));
        entity->motion    = malloc(sizeof(ActorMotion));
        entity->armature  = malloc(sizeof(ActorArmature));
        entity->animation = malloc(sizeof(ActorAnimation));

        *entity->physics  = (RigidBody){0};
        *entity->motion   = (ActorMotion){ .settings = *motion_settings, .data.grounded = true };
        *entity->armature = (ActorArmature){0};
        *entity->animation = (ActorAnimation){ .settings = *animation_settings, .data.footing_phase = 0.5f };
        entity->state     = (ActorStateData){ STANDING_IDLE, 0, 0 };
    }
}

Entity *entity_create(EntityType type, const char *model_path, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings)
{
    Entity *entity = malloc(sizeof(Entity));
    entity_init(entity, type, motion_settings, animation_settings);

    entity->mesh               = malloc(sizeof(RenderMesh));
    entity->mesh->model        = t3d_model_load(model_path);
    entity->mesh->matrix_buffer = malloc_uncached(sizeof(T3DMat4FP) * FB_COUNT);
    t3d_mat4fp_identity(entity->mesh->matrix_buffer);

    if (type == ENTITY_ACTOR) {
        actorAnimation_initArmature(entity);

        rspq_block_begin();
        t3d_model_draw_skinned(entity->mesh->model, &entity->armature->main);
        entity->mesh->dl = rspq_block_end();
    } else {
        rspq_block_begin();
        t3d_model_draw(entity->mesh->model);
        entity->mesh->dl = rspq_block_end();
    }

    return entity;
}

void entity_delete(Entity *entity)
{
    rspq_block_free(entity->mesh->dl);
    free_uncached(entity->mesh->matrix_buffer);
    t3d_model_free(entity->mesh->model);
    free(entity->mesh);

    if (entity->type == ENTITY_ACTOR) {
        free(entity->physics);
        free(entity->motion);
        free(entity->armature);
        free(entity->animation);
    }

    free(entity);
}
