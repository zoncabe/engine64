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
        entity->actor = malloc(sizeof(Actor));
        *entity->actor = (Actor){
            .body      = (RigidBody){0},
            .motion    = (ActorMotion){ .settings = *motion_settings, .data.grounded = true },
            .animation = (ActorAnimation){ .settings = *animation_settings },
            .state     = (ActorStateData){ STANDING_IDLE, 0, 0 },
        };
    }
}

Entity *entity_create(EntityType type, const char *model_path, const ActorMotionSettings *motion_settings, const ActorAnimationSettings *animation_settings, const AnimDef *anim_def)
{
    Entity *entity = malloc(sizeof(Entity));
    entity_init(entity, type, motion_settings, animation_settings);

    entity->mesh               = malloc(sizeof(Mesh));
    entity->mesh->model        = t3d_model_load(model_path);
    entity->mesh->matrix_buffer = malloc_uncached(sizeof(T3DMat4FP) * FB_COUNT);
    t3d_mat4fp_identity(entity->mesh->matrix_buffer);

    if (type == ENTITY_ACTOR) {
        actorAnimation_initGraph(entity, anim_def);

        rspq_block_begin();
        t3d_model_draw_skinned(entity->mesh->model, &entity->actor->animation.main);
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
        ActorAnimation *anim = &entity->actor->animation;
        for (uint8_t i = 0; i < entity->actor->anim_def->clip_count; i++)
            t3d_anim_destroy(&anim->animation[i]);
        for (uint8_t i = 0; i < entity->actor->anim_def->buffer_count; i++)
            t3d_skeleton_destroy(&anim->buffer[i]);
        t3d_skeleton_destroy(&anim->main);
        free(anim->animation);
        free(anim->buffer);
        free(anim->node_state);
        free(entity->actor);
    }

    free(entity);
}
