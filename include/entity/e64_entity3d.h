#ifndef ENGINE64_ENTITY3D_H
#define ENGINE64_ENTITY3D_H

#include "render/e64_render.h"
#include "render/e64_transform3d.h"
#include "prefab/e64_prefab3d.h"

namespace e64 {

namespace physics { class World; }


/* A prefab open in the 3D scene: one live object per entry it declared, in
   the same order. It groups them and nothing else: the entity stands
   nowhere, each mesh, each light and the body carry their own place. The
   prefab itself is not kept: what the entity needs from it is copied here. */
class Entity3D {
public:

	Mesh **mesh;
	uint8_t mesh_count;

	Light **light;
	uint8_t light_count;

	RigidBody *body;

	/* The looping ones play from the entity for as long as it exists; the
	   rest wait for whoever fires them, the character its own, the game the
	   prop's. */
	Sound *sound;
	uint8_t sound_count;

	/* Test each model's bounding box against the view frustum before drawing
	   it. Off means the entity is drawn every frame, no questions asked. */
	bool cull;

	/* The game's code for this entity, copied from the prefab. The scene
	   calls them with the entity; NULL is not called. */
	void (*create)(Entity3D *entity);
	void (*update)(Entity3D *entity);
	void (*destroy)(Entity3D *entity);


	/* Zeroes the entity and takes the prefab's cull flag and callbacks;
	   opening the content is entity3d::create's. */
	void init(const Prefab3D *prefab);

	/* Where the entity's sounds play from, in metres: its body when it has
	   one, its first model otherwise, the origin when it has neither. */
	Vector3 soundPosition(void) const;

	/* Starts the looping sounds. Called once the body is attached. */
	void startLoopSounds(void);

	/* For entities the solver moves: every model is drawn where the body
	   is, each at its own scale. No-op without a simulated body. */
	void setMatrixFromBody(uint8_t fb_index);

	/* Shows or hides the objects the prefab declared as a part, by the same
	   name, in every mesh that has one. */
	void setPartVisible(const char *name, bool visible);

	/* Draws those objects away from where they were modelled, without
	   touching the rest of the model. The offset is read in the model's own
	   space and the part keeps following the model. */
	void setPartOffset(const char *name, const Transform3D *offset);

	/* Fires a trigger on the entity: one of its sounds declared with that
	   trigger plays, picked at random. position NULL plays from
	   soundPosition; a given one is in metres. Nothing declared, nothing
	   plays. */
	void playSound(uint8_t trigger, const Vector3 *position, float volume_scale) const;

	/* Builds the body the def declares, where the def says and with its
	   shapes at their declared size. NULL def, no body. */
	void attachPhysics(physics::World *world, const RigidBody::Def *def);

};


namespace entity3d {

/* One allocation, the prefab's meshes, lights and sounds opened; destroy
   closes them and frees it. */
Entity3D *create(const Prefab3D *prefab);
void destroy(Entity3D *entity);

}

}

#endif
