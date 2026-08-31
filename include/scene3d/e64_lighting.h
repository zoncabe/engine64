#ifndef ENGINE64_LIGHTING_H
#define ENGINE64_LIGHTING_H

#include <libdragon.h>
#include <t3d/t3dmath.h>

/* t3d hands out seven slots and a light takes one whatever its kind, so the
   split between directional and point is the scene's to make. */
#define LIGHT_COUNT 7


typedef enum {

	/* An empty slot. Zero on purpose: the set walks the table in order and
	   stops at the first one, so a scene pays only for what it declared. */
	LIGHT_NONE,

	LIGHT_DIRECTIONAL,
	LIGHT_POINT,

} LightType;


typedef struct {

	LightType type;
	color_t   color;

	union {
		/* Where the light comes from; normalised by the init. */
		struct { T3DVec3 direction; } directional;

		/* Where it stands and how far it carries. */
		struct { T3DVec3 position; float size; } point;
	};

} LightSource;


typedef struct {

	color_t ambient_color;
	LightSource source[LIGHT_COUNT];

} LightDef;

typedef LightDef Light;


Light *light_get(void);

/* Copies the scene's declaration into the live lights. */
void light_init(const LightDef *def);

/* Hands the lights to t3d, stopping at the first empty slot. */
void light_set(const Light *light);

#endif
