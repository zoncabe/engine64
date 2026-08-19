#include <stdint.h>
#include <libdragon.h>
#include <rspq_profile.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dmath.h>

#include "physics/physics.h"
#include "scene/lighting.h"


static Light light;

Light* light_get(void) { return &light; }


void light_initAmbient(color_t color)
{
	light.ambient_color = color;
}

void light_initDirectional(DirectionalLight* light, T3DVec3 direction, color_t color) {

	light->direction = direction;
	light->color = color;
	t3d_vec3_norm(&light->direction);
}

void light_initPoint(PointLight* light, T3DVec3 position, color_t color, float strength) {

	light->position = position;
	light->color = color;
	light->size = strength;
}

void light_updatePoint(PointLight* light, T3DVec3 position, color_t color, float strength) {

	light->position = position;
	light->color = color;
	light->size = strength;
}

void light_setDirectional(DirectionalLight* light)
{
	for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; i++){

		t3d_light_set_directional(i, &light[i].color.r, &light[i].direction);
	}
}

void light_setPoint(PointLight* light)
{
	for (int i = 0; i < POINT_LIGHT_COUNT; i++) {

		/* The slots are shared with the directionals, which took the first ones. */
		t3d_light_set_point(DIRECTIONAL_LIGHT_COUNT + i, &light[i].color.r,
		                    &light[i].position, light[i].size, false);
	}
}

void light_set(Light* light)
{
	t3d_light_set_ambient(&light->ambient_color.r);
	light_setDirectional(light->directional);
	light_setPoint(light->point);
	t3d_light_set_count(DIRECTIONAL_LIGHT_COUNT + POINT_LIGHT_COUNT);
}
