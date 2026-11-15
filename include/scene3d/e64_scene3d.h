#ifndef ENGINE64_SCENE3D_H
#define ENGINE64_SCENE3D_H

#include "entity/e64_entity.h"
#include "scene3d/e64_light.h"
#include "scene3d/e64_fog.h"
#include "camera/e64_camera3d.h"
#include "physics3d/e64_physics_world.h"

namespace e64 {

class Scene3D {
public:

	static constexpr int MAX_CHARACTERS = 16;
	static constexpr int MAX_ENTITIES = 255;

	/* One occurrence of a prefab in the scene: which prefab and where. A
	   scene's content is an array of these; the load walks it in order and
	   builds the live entity of the same index. A zero scale means identity. */
	struct Instance {

		const Prefab *prefab;
		Vector3 position;
		Vector3 rotation;
		Vector3 scale;

	};

	struct Def {

		const light::Def *light;
		const fog::Def *fog;
		Vector3 wind;

		const Instance *instance;
		uint8_t instance_count;

	};


	/* The view the scene is projected through. The load leaves it at its
	   defaults; the game places it and sets its lens, every frame if it
	   wants, and the step hands it to the viewport. */
	Camera3D camera;

	/* The world the scene loads its bodies and cloths into. */
	physics::World physics;

	Entity *entity[MAX_ENTITIES];
	uint8_t entity_count;

	Character3D *character[MAX_CHARACTERS];
	uint8_t character3d_count;


	/* The entities come out in declaration order: the scene's entity list
	   reads by the same index as the def's instance list. Nobody is seated:
	   the game seats its players once the scene is up, from its onEnter. */
	void load(const Def *def);
	void clear(void);
	void unload(void);

	/* Collides every character and carries the result to what draws it. Call
	   after physics::update, with the frame's buffer index. */
	void updateCharacters(uint8_t fb_index);

	/* The same for everything else the world holds. Call after physics::update,
	   with the frame's buffer index. */
	void updateEntities(uint8_t fb_index);

	void addEntity(Entity *entity);
	Character3D *getCharacter3D(uint8_t index);

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
