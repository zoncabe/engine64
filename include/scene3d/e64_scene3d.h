#ifndef ENGINE64_SCENE3D_H
#define ENGINE64_SCENE3D_H

#include <libdragon.h>

#include "entity/e64_entity3d.h"
#include "scene3d/e64_light.h"
#include "scene3d/e64_fog.h"
#include "camera/e64_camera3d.h"
#include "physics3d/e64_physics_world.h"

namespace e64 {

class Scene3D {
public:

	/* What a scene is before anything stands in it: the colour that lights
	   what no light reaches, the fog and the wind. The entities are not
	   declared here: the game creates them with createEntity once the scene
	   is up. Left at zero the scene is pitch black, without fog or wind. */
	struct Def {

		color_t ambient_color;
		const fog::Def *fog;
		Vector3 wind;

	};


	/* The view the scene is projected through. The load leaves it at its
	   defaults; the game places it and sets its lens, every frame if it
	   wants, and the step hands it to the viewport. */
	Camera3D camera;

	/* The world the scene creates its bodies in. */
	physics::World physics;

	/* The scene's entities, in creation order: an array of pointers that
	   grows by one with each entity created and is freed at unload. NULL
	   while the scene has none. */
	Entity3D **entity;
	uint16_t entity_count;


	/* Brings the scene up empty: lights, fog and the physics world. Nobody
	   is seated and nothing stands in it: the game creates its entities and
	   seats its players from its onEnter. */
	void load(const Def *def);
	void clear(void);

	/* Calls each entity's destroy first, with the scene still whole, and
	   then takes the entities and the world down. */
	void unload(void);

	/* Opens the prefab as an entity of this scene: its models, lights and
	   sounds where each one declares, its body in the scene's world, its
	   looping sounds playing, and its create called last. The entity is the
	   scene's from here on, destroyed at unload. */
	Entity3D *createEntity(const Prefab3D *prefab);

	/* Calls the update of every entity that has one. Call after
	   physics::update: the game reads the world already advanced, and what
	   it writes enters the next frame's step. */
	void updateCallbacks(void);

	/* Carries what the solver moved to what draws it. Call after
	   physics::update, with the frame's buffer index. */
	void updateEntities(uint8_t fb_index);

	void addEntity(Entity3D *entity);

	/* Pushes one Render::Element3D per visible model object into the frame's
	   context. The culling runs here, against the frame's frustum, before each
	   mesh is read. */
	void setRenderContext(Render::Context *ctx, const Viewport *viewport) const;

};


namespace scene3d {

Scene3D *get(void);

}

}

#endif
