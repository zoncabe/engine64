#ifndef ENGINE64_ENTITY_H
#define ENGINE64_ENTITY_H

#include "render/e64_render.h"
#include "prefab/e64_prefab.h"

namespace e64 {

namespace physics { class World; }


/* A prefab open in the world: what it declared, one live object per entry
   in the same order, and where it stands. create opens the content; the
   scene writes the transform and then starts what depends on it. */
class Entity {
public:

	Render::Transform transform;

	const Prefab *prefab;

	Mesh **mesh;
	uint8_t mesh_count;

	Graphic **graphic;
	uint8_t graphic_count;

	RigidBody *body;

	/* The looping ones play from the entity for as long as it exists; the
	   rest wait for whoever fires them, the character its own, the game the
	   prop's. */
	Sound *sound;
	uint8_t sound_count;

	/* Test each model's bounding box against the view frustum before drawing
	   it. Off means the entity is drawn every frame, no questions asked. */
	bool cull;


	/* Zeroes the entity, keeps the prefab and takes its cull flag; opening
	   the content is entity::create's. */
	void init(const Prefab *prefab);

	/* Starts the looping sounds from where the entity stands. Called once the
	   transform is written. */
	void startLoopSounds(void);

	void setMatrix(uint8_t fb_index);

	/* For entities the solver moves: their place lives in the body, not in
	   the transform, and a tumbling body needs its quaternion rather than the
	   euler angles the transform carries. No-op without a simulated body. */
	void setMatrixFromBody(uint8_t fb_index);

	/* Shows or hides the objects the prefab declared as a part, by the same
	   name, in every mesh that has one. */
	void setPartVisible(const char *name, bool visible);

	/* Draws those objects away from where they were modelled, without
	   touching the rest of the model. The offset is read in the entity's own
	   space and the part keeps following the entity. */
	void setPartOffset(const char *name, const Render::Transform *offset);

	/* Fires a trigger on the entity: one of its sounds declared with that
	   trigger plays, picked at random. position NULL plays from where the
	   entity is; a given one is in render units. Nothing declared, nothing
	   plays. */
	void playSound(uint8_t trigger, const Vector3 *position, float volume_scale) const;

	/* Builds the body at the entity's transform with the prefab's collider
	   shapes, from its body def when it has one; without it the body comes
	   out static, which is what scenery wants. No collider, no body. */
	void attachPhysics(physics::World *world);

	/* World transform of a static collider: entity position in metres plus
	   the entity rotation built with the same euler function the renderer
	   uses, so collision and visuals always match. */
	Transform colliderTransform(void) const;

};


namespace entity {

/* One allocation, the prefab's meshes, graphics and sounds opened; destroy
   closes them and frees it. */
Entity *create(const Prefab *prefab);
void destroy(Entity *entity);

}

}

#endif
