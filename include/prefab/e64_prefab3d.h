/*
	Content declaration: the render, sound and physics primitives an entity
	of the 3D scene is made of. Count zero, or a NULL body, means the entity
	has none of that. Nothing here places the entity: each primitive
	declares where it stands itself.

	Anything built on top of the primitives (a character, a water surface, a
	cloth) is declared by the game, in a prefab of its own that carries one
	of these inside.
*/
#ifndef ENGINE64_PREFAB3D_H
#define ENGINE64_PREFAB3D_H

#include <stdint.h>

#include "model/e64_mesh.h"
#include "physics3d/e64_rigid_body.h"
#include "scene3d/e64_light.h"
#include "sound/e64_sound.h"

namespace e64 {

/* Its header includes this one. */
class Entity3D;


struct Prefab3D {

	/* What is drawn. */
	const Mesh::Def *mesh;
	uint8_t mesh_count;

	/* The lights the entity carries, each one a slot of the scene's seven
	   for as long as the entity lives. */
	const Light::Def *light;
	uint8_t light_count;

	/* The body and the shapes it carries, where it stands included. NULL
	   means no body. */
	const RigidBody::Def *body;

	/* Opened in the entity from create to delete. The looping ones play on
	   their own; the rest wait for whoever fires them. */
	const Sound::Def *const *sound;
	uint8_t sound_count;

	/* Test the model's bounding box against the view frustum before
	   drawing it. */
	bool cull;

	/* What the entity does, as game code: the engine calls these with the
	   entity and knows nothing of what they do. create runs once the scene
	   has created the entity, with its body; update every frame, after the
	   physics step; destroy when the scene unloads it, with the scene still
	   whole. NULL is not called. */
	void (*create)(Entity3D *entity);
	void (*update)(Entity3D *entity);
	void (*destroy)(Entity3D *entity);

};

}

#endif
