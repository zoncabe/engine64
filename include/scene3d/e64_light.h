#ifndef ENGINE64_LIGHT_H
#define ENGINE64_LIGHT_H

#include <libdragon.h>

#include "math/e64_vector3.h"

namespace e64 {

/* One light of the scene, a primitive like a mesh or a sound: a prefab
   declares it, light::create opens it, and whoever holds it moves it by
   writing its fields. It carries its own place in the scene, in metres. */
class Light {
public:

	/* t3d hands out seven slots and a light takes one whatever its kind, so
	   the split between directional and point is the game's to make. */
	static constexpr int MAX_LIGHTS = 7;

	enum Type {

		/* A free slot. Zero on purpose: lights nobody opened are all free. */
		LIGHT_NONE,

		LIGHT_DIRECTIONAL,
		LIGHT_POINT,

	};


	struct Def {

		Type type;
		color_t color;

		/* Directional: where the light comes from. Normalised when the
		   light is created. */
		Vector3 direction;

		/* Point: where it stands in the scene and how far it carries. */
		Vector3 position;
		float size;

	};


	Type type;
	color_t color;

	Vector3 direction; /* directional; kept normalised by whoever writes it */
	Vector3 position; /* point, metres */
	float size; /* point, metres */

};


namespace light {

/* Frees every light and sets the ambient colour. */
void init(color_t ambient_color);

void setAmbient(color_t ambient_color);

/* Opens a light from its declaration. Asserts when the seven are taken. */
Light *create(const Light::Def *def);
void destroy(Light *light);

/* Hands the ambient colour and the open lights to t3d. */
void set(void);

}

}

#endif
