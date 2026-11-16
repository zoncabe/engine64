#include <assert.h>
#include <stdlib.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "entity/e64_entity3d.h"
#include "viewport/e64_viewport.h"
#include "math/e64_math.h"
#include "physics3d/e64_physics_world.h"

namespace e64 {

void Entity3D::init(const Prefab3D *prefab)
{
	*this = (Entity3D){};
	cull = prefab->cull;
	create = prefab->create;
	update = prefab->update;
	destroy = prefab->destroy;
}

Vector3 Entity3D::soundPosition(void) const
{
	if (body) return body->tx.position;
	if (mesh_count) return mesh[0]->transform.position;
	return (Vector3){ 0.0f, 0.0f, 0.0f };
}

void Entity3D::startLoopSounds(void)
{
	Vector3 position = soundPosition();

	for (uint8_t i = 0; i < sound_count; i++) {
		if (sound[i].def->loop)
			sound::play(&sound[i], &position, 1.0f, 0.0f);
	}
}

void Entity3D::setPartVisible(const char *name, bool visible)
{
	for (uint8_t i = 0; i < mesh_count; i++) {
		uint8_t part = mesh::part::find(mesh[i], name);
		if (!part) continue;

		mesh::part::setVisible(mesh[i], part, visible);
	}
}

void Entity3D::setPartOffset(const char *name, const Transform3D *offset)
{
	for (uint8_t i = 0; i < mesh_count; i++) {
		uint8_t part = mesh::part::find(mesh[i], name);
		if (!part) continue;

		mesh::part::setOffset(mesh[i], part, offset);
	}
}

void Entity3D::playSound(uint8_t trigger, const Vector3 *position, float volume_scale) const
{
	/* The candidates are the entity's sounds tagged with this trigger; the
	   loops are already playing and never fire. */
	uint8_t candidate[Sound::MAX_EMITTERS];
	uint8_t count = 0;

	for (uint8_t i = 0; i < sound_count && count < Sound::MAX_EMITTERS; i++) {
		const Sound::Def *def = sound[i].def;
		if (def->loop || def->trigger != trigger) continue;
		candidate[count++] = i;
	}
	if (count == 0) return;

	Vector3 here;
	if (position == NULL) {
		here = soundPosition();
		position = &here;
	}

	sound::play(&sound[candidate[rand() % count]], position, volume_scale, 0.0f);
}

void Entity3D::setMatrixFromBody(uint8_t fb_index)
{
	if (body == NULL || !(body->flags & RigidBody::BODY_FLAG_DYNAMIC)) return;

	/* The body gives the place and each model keeps the scale it has. */
	for (uint8_t i = 0; i < mesh_count; i++) {
		Transform3D placed = transform3d::fromBody(body, &mesh[i]->transform.scale);
		mesh::setMatrix(mesh[i], &placed, fb_index);
	}
}

/* The body is the def's, as declared: where it stands, how it turns and the
   shapes it carries all come from there, and nothing from the entity. */
void Entity3D::attachPhysics(physics::World *world, const RigidBody::Def *def)
{
	if (def == NULL) return;

	RigidBody::Def body_def = *def;

	/* What a def left at zero means: every layer bit off would collide with
	   nothing, and a zero axis is no rotation, for which any axis stands. */
	if (body_def.layers == 0) body_def.layers = 1;
	if (vector3::squaredMagnitude(&body_def.axis) == 0.0f)
		body_def.axis = (Vector3){ 0.0f, 0.0f, 1.0f };

	body = physics::world::createBody(world, &body_def);
	body->owner = this;

	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	for (uint8_t s = 0; s < def->shape_count; s++)
		rigidBody::addShape(body, &def->shape[s], scale);
}


namespace entity3d {

Entity3D *create(const Prefab3D *prefab)
{
	Entity3D *entity = (Entity3D *)malloc(sizeof(Entity3D));
	assert(entity);
	entity->init(prefab);

	if (prefab->mesh_count) {
		entity->mesh = (Mesh **)malloc(prefab->mesh_count * sizeof(Mesh *));
		assert(entity->mesh);
		for (uint8_t i = 0; i < prefab->mesh_count; i++)
			entity->mesh[i] = mesh::create(&prefab->mesh[i]);
		entity->mesh_count = prefab->mesh_count;
	}

	if (prefab->light_count) {
		entity->light = (Light **)malloc(prefab->light_count * sizeof(Light *));
		assert(entity->light);
		for (uint8_t i = 0; i < prefab->light_count; i++)
			entity->light[i] = light::create(&prefab->light[i]);
		entity->light_count = prefab->light_count;
	}

	/* The sounds open with the entity and close with it. The loops wait for
	   startLoopSounds, once the scene has attached the body. */
	if (prefab->sound_count) {
		entity->sound = (Sound *)malloc(prefab->sound_count * sizeof(Sound));
		assert(entity->sound);
		for (uint8_t i = 0; i < prefab->sound_count; i++)
			entity->sound[i] = sound::load(prefab->sound[i]);
		entity->sound_count = prefab->sound_count;
	}

	return entity;
}

void destroy(Entity3D *entity)
{
	for (uint8_t i = 0; i < entity->mesh_count; i++)
		mesh::destroy(entity->mesh[i]);
	free(entity->mesh);

	for (uint8_t i = 0; i < entity->light_count; i++)
		light::destroy(entity->light[i]);
	free(entity->light);

	for (uint8_t i = 0; i < entity->sound_count; i++)
		sound::unload(&entity->sound[i]);
	free(entity->sound);

	/* The body belongs to the physics world, which drops it with it. */
	free(entity);
}

}

}
