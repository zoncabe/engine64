#include <assert.h>
#include <stdlib.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>

#include "entity/e64_entity.h"
#include "viewport/e64_viewport.h"
#include "math/e64_math.h"
#include "physics3d/e64_physics_world.h"

namespace e64 {

void Entity::init(const Prefab *prefab)
{
	*this = (Entity){};
	render::transform::init(&transform);
	this->prefab = prefab;
	cull = prefab->cull;
}

void Entity::startLoopSounds(void)
{
	for (uint8_t i = 0; i < sound_count; i++) {
		if (sound[i].def->loop)
			sound::play(&sound[i], &transform.position, 1.0f, 0.0f);
	}
}

void Entity::setPartVisible(const char *name, bool visible)
{
	for (uint8_t i = 0; i < mesh_count; i++) {
		uint8_t part = mesh::part::find(mesh[i], name);
		if (!part) continue;

		mesh::part::setVisible(mesh[i], part, visible);
	}
}

void Entity::setPartOffset(const char *name, const Render::Transform *offset)
{
	for (uint8_t i = 0; i < mesh_count; i++) {
		uint8_t part = mesh::part::find(mesh[i], name);
		if (!part) continue;

		mesh::part::setOffset(mesh[i], part, offset);
	}
}

void Entity::playSound(uint8_t trigger, const Vector3 *position, float volume_scale) const
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
		if (body) here = body->tx.position;
		else here = transform.position;
		position = &here;
	}

	sound::play(&sound[candidate[rand() % count]], position, volume_scale, 0.0f);
}

void Entity::setMatrix(uint8_t fb_index)
{
	for (uint8_t i = 0; i < mesh_count; i++)
		mesh::setMatrix(mesh[i], &transform, fb_index);
}

void Entity::setMatrixFromBody(uint8_t fb_index)
{
	if (body == NULL || !(body->flags & RigidBody::BODY_FLAG_DYNAMIC)) return;

	for (uint8_t i = 0; i < mesh_count; i++)
		mesh::setMatrixFromBody(mesh[i], &body->tx.position, &body->q, &transform.scale, fb_index);
}

Transform Entity::colliderTransform(void) const
{
	T3DMat4 mat;
	t3d_mat4_from_srt_euler(&mat,
		(float[3]){1.0f, 1.0f, 1.0f},
		(float[3]){deg_to_rad(transform.rotation.x), deg_to_rad(transform.rotation.y), deg_to_rad(transform.rotation.z)},
		(float[3]){0.0f, 0.0f, 0.0f});

	return (Transform){
		.position = transform.position,
		.rotation = {
			.ex = { mat.m[0][0], mat.m[0][1], mat.m[0][2] },
			.ey = { mat.m[1][0], mat.m[1][1], mat.m[1][2] },
			.ez = { mat.m[2][0], mat.m[2][1], mat.m[2][2] },
		},
	};
}

/* Copies the body-def override bits on top of the freshly-initialised body
   (position, orientation) and then attaches the collider's shapes to it. */
void Entity::attachPhysics(physics::World *world)
{
	if (prefab->collider == NULL) return;

	RigidBody::Def body_def;
	rigidBody::def::init(&body_def);

	if (prefab->body) {
		/* The prefab's RigidBody::Def describes only the body properties,
		   not the world position. Position/rotation come from the entity. */
		const RigidBody::Def *src = prefab->body;
		body_def.body_type = src->body_type;
		body_def.gravity_scale = src->gravity_scale;
		body_def.layers = src->layers ? src->layers : 1;
		body_def.linear_damping = src->linear_damping;
		body_def.angular_damping = src->angular_damping;
		body_def.allow_sleep = src->allow_sleep;
		body_def.awake = src->awake;
		body_def.active = src->active;
		body_def.lock_axis_x = src->lock_axis_x;
		body_def.lock_axis_y = src->lock_axis_y;
		body_def.lock_axis_z = src->lock_axis_z;
	}

	/* Position in metres and rotation through the renderer's euler convention,
	   both from the collider transform: a rotated entity collides the way it
	   renders. */
	Transform collider = colliderTransform();
	body_def.position = collider.position;

	Quaternion rotation = quaternion::fromMatrix3(&collider.rotation);
	quaternion::toAxisAngle(&rotation, &body_def.axis, &body_def.angle);

	/* Identity degenerates to a zero axis; any axis stands for no rotation. */
	if (vector3::squaredMagnitude(&body_def.axis) == 0.0f)
		body_def.axis = (Vector3){ 0.0f, 0.0f, 1.0f };

	body = physics::world::createBody(world, &body_def);
	body->owner = this;

	for (uint8_t s = 0; s < prefab->collider->count; s++)
		rigidBody::addShape(body, &prefab->collider->shape[s], transform.scale);
}


namespace entity {

Entity *create(const Prefab *prefab)
{
	Entity *entity = (Entity *)malloc(sizeof(Entity));
	assert(entity);
	entity->init(prefab);

	if (prefab->mesh_count) {
		entity->mesh = (Mesh **)malloc(prefab->mesh_count * sizeof(Mesh *));
		assert(entity->mesh);
		for (uint8_t i = 0; i < prefab->mesh_count; i++)
			entity->mesh[i] = mesh::create(&prefab->mesh[i]);
		entity->mesh_count = prefab->mesh_count;
	}

	if (prefab->graphic_count) {
		entity->graphic = (Graphic **)malloc(prefab->graphic_count * sizeof(Graphic *));
		assert(entity->graphic);
		for (uint8_t i = 0; i < prefab->graphic_count; i++)
			entity->graphic[i] = graphic::create(&prefab->graphic[i]);
		entity->graphic_count = prefab->graphic_count;
	}

	/* The sounds open with the entity and close with it. The loops wait for
	   startLoopSounds, once the scene has placed the entity. */
	if (prefab->sound_count) {
		entity->sound = (Sound *)malloc(prefab->sound_count * sizeof(Sound));
		assert(entity->sound);
		for (uint8_t i = 0; i < prefab->sound_count; i++)
			entity->sound[i] = sound::load(prefab->sound[i]);
		entity->sound_count = prefab->sound_count;
	}

	return entity;
}

void destroy(Entity *entity)
{
	for (uint8_t i = 0; i < entity->mesh_count; i++)
		mesh::destroy(entity->mesh[i]);
	free(entity->mesh);

	for (uint8_t i = 0; i < entity->graphic_count; i++)
		graphic::destroy(entity->graphic[i]);
	free(entity->graphic);

	for (uint8_t i = 0; i < entity->sound_count; i++)
		sound::unload(&entity->sound[i]);
	free(entity->sound);

	/* The body belongs to the physics world, which drops it with it. */
	free(entity);
}

}

}
