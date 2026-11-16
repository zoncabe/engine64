#include <assert.h>
#include <stdint.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>

#include "engine/e64_common.h" /* RENDER_SCALE */
#include "scene3d/e64_light.h"

namespace e64 {
namespace light {

/* The seven slots t3d has, and the colour that lights what none of them
   reaches. A light lives in its slot from create to destroy and never
   moves, so the pointer handed out stays good. */
static Light pool[Light::MAX_LIGHTS];
static color_t ambient;


void init(color_t ambient_color)
{
	ambient = ambient_color;

	for (int i = 0; i < Light::MAX_LIGHTS; i++)
		pool[i] = (Light){};
}

void setAmbient(color_t ambient_color)
{
	ambient = ambient_color;
}

Light *create(const Light::Def *def)
{
	assert(def->type != Light::LIGHT_NONE);

	for (int i = 0; i < Light::MAX_LIGHTS; i++) {
		Light *light = &pool[i];
		if (light->type != Light::LIGHT_NONE) continue;

		*light = (Light){
			.type = def->type,
			.color = def->color,
			.direction = def->direction,
			.position = def->position,
			.size = def->size,
		};

		if (light->type == Light::LIGHT_DIRECTIONAL)
			light->direction = vector3::normalized(&def->direction);

		return light;
	}

	assert(!"every light slot is taken");
	return NULL;
}

void destroy(Light *light)
{
	*light = (Light){};
}

/* A light is kept in metres, where whoever moves it writes it, and the
   hardware measures against the geometry it lights: the conversion to
   render units happens here, on the way out. The free slots are skipped,
   so t3d gets the open lights one after the other. */
void set(void)
{
	t3d_light_set_ambient((uint8_t *)&ambient.r);

	int count = 0;
	for (int i = 0; i < Light::MAX_LIGHTS; i++) {
		const Light *light = &pool[i];

		switch (light->type) {
			case Light::LIGHT_DIRECTIONAL: {
				T3DVec3 direction = {{ light->direction.x, light->direction.y, light->direction.z }};
				t3d_light_set_directional(count++, (uint8_t *)&light->color.r, &direction);
				break;
			}
			case Light::LIGHT_POINT: {
				T3DVec3 position = {{
					light->position.x * RENDER_SCALE,
					light->position.y * RENDER_SCALE,
					light->position.z * RENDER_SCALE,
				}};
				t3d_light_set_point(count++, (uint8_t *)&light->color.r, &position,
				                    light->size * RENDER_SCALE, false);
				break;
			}
			case Light::LIGHT_NONE:
				break;
		}
	}

	t3d_light_set_count(count);
}

}
}
