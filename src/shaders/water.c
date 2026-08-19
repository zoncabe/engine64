#include <malloc.h>
#include <string.h>

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>

#include "shaders/water.h"
#include "physics/collision/collision_mesh.h"
#include "physics/world/physics_world.h"


/* Fresh water, and a drag that settles a bobbing crate in a few swings. */
#define WATER_DEFAULT_DENSITY       1000.0f
#define WATER_DEFAULT_LINEAR_DRAG   2.5f
#define WATER_DEFAULT_ANGULAR_DRAG  1.5f


static Water  water_pool[WATER_MAX_SURFACES];
static uint8_t water_count;


/* The material setup already wrote the tile's own translate (its low bits),
   so the scroll adds on top instead of replacing it. */
static void water_tileCb(void *user, rdpq_texparms_t *tile, rdpq_tile_t id)
{
	const Water *water = user;

	if (id == TILE0) {
		tile->s.translate += water->offset_a[0];
		tile->t.translate += water->offset_a[1];
	} else {
		tile->s.translate += water->offset_b[0];
		tile->t.translate += water->offset_b[1];
	}
}

Water *water_create(const WaterDef *def)
{
	if (water_count >= WATER_MAX_SURFACES) return NULL;

	CollisionMesh *mesh = collisionMesh_load(def->mesh_path);
	if (mesh == NULL) return NULL;

	Water *water = &water_pool[water_count];
	*water = (Water){ .def = *def };

	water->count    = mesh->vertex_count;
	water->position = malloc(sizeof(Vector3) * water->count * 3);
	water->rgba     = malloc(4 * water->count);
	if (water->position == NULL || water->rgba == NULL) {
		free(water->position);
		free(water->rgba);
		collisionMesh_delete(mesh);
		*water = (Water){0};
		return NULL;
	}
	water->normal = water->position + water->count;
	water->rest   = water->normal   + water->count;

	/* The mesh is scaffolding, same as the cloth: it seeds the points and
	   nothing keeps a reference to it afterwards. */
	memcpy(water->rest, mesh->vertices, sizeof(Vector3) * water->count);
	collisionMesh_delete(mesh);

	/* An unset tint would paint the water black; white leaves the caustics. */
	if (water->def.color[0] == 0 && water->def.color[1] == 0 && water->def.color[2] == 0)
		water->def.color[0] = water->def.color[1] = water->def.color[2] = 255;

	if (water->def.density      == 0.0f) water->def.density      = WATER_DEFAULT_DENSITY;
	if (water->def.linear_drag  == 0.0f) water->def.linear_drag  = WATER_DEFAULT_LINEAR_DRAG;
	if (water->def.angular_drag == 0.0f) water->def.angular_drag = WATER_DEFAULT_ANGULAR_DRAG;

	/* The plane is authored flat; the average irons out export noise. */
	for (uint16_t i = 0; i < water->count; i++)
		water->base_z += water->rest[i].z;
	water->base_z /= (float)water->count;

	for (uint16_t i = 0; i < water->count; i++) {
		water->position[i] = water->rest[i];
		water->normal[i]   = (Vector3){ 0.0f, 0.0f, 1.0f };
		memcpy(&water->rgba[i * 4], (uint8_t[]){ water->def.color[0], water->def.color[1],
		                                         water->def.color[2], 0xFF }, 4);
	}

	for (uint8_t w = 0; w < water->def.wave_count; w++) {
		Vector3 dir = { water->def.wave[w].direction_x, water->def.wave[w].direction_y, 0.0f };
		vector3_normalize(&dir);
		water->def.wave[w].direction_x = dir.x;
		water->def.wave[w].direction_y = dir.y;
		water->amplitude_sum += water->def.wave[w].amplitude;
	}

	water->conf = (T3DModelDrawConf){
		.userData = water,
		.tileCb   = water_tileCb,
	};

	water_count++;
	return water;
}

static void water_scroll(float offset[2], const float speed[2], float wrap, float delta)
{
	offset[0] += speed[0] * delta;
	offset[1] += speed[1] * delta;

	/* Folded into [0, wrap): a negative translate overflows the fixed-point
	   tile coordinates, and fm_fmodf keeps the sign of its operand. */
	if (wrap > 0.0f) {
		for (int i = 0; i < 2; i++) {
			offset[i] = fm_fmodf(offset[i], wrap);
			if (offset[i] < 0.0f) offset[i] += wrap;
		}
	}
}

static void water_waves(Water *water)
{
	const WaterDef *def = &water->def;

	for (uint16_t i = 0; i < water->count; i++) {
		const Vector3 *rest = &water->rest[i];
		float height = 0.0f;
		float slope_x = 0.0f;
		float slope_y = 0.0f;

		for (uint8_t w = 0; w < def->wave_count; w++) {
			const WaterWave *wave = &def->wave[w];

			float phase = (wave->direction_x * rest->x + wave->direction_y * rest->y)
			            * wave->frequency + water->time * wave->speed;

			float s = fm_sinf(phase);
			float c = fm_cosf(phase);

			height  += wave->amplitude * s;
			slope_x += wave->amplitude * wave->frequency * wave->direction_x * c;
			slope_y += wave->amplitude * wave->frequency * wave->direction_y * c;
		}

		water->position[i].z = rest->z + height;

		/* Normal of z = h(x,y) is (-dh/dx, -dh/dy, 1). */
		Vector3 normal = { -slope_x, -slope_y, 1.0f };
		water->normal[i] = vector3_normalized(&normal);

		/* Crests lighter, troughs darker, the shading of the example's lava. */
		float bright = 0.75f;
		if (water->amplitude_sum > 0.0f)
			bright += 0.25f * (height / water->amplitude_sum);

		uint8_t *rgba = &water->rgba[i * 4];
		rgba[0] = (uint8_t)(water->def.color[0] * bright);
		rgba[1] = (uint8_t)(water->def.color[1] * bright);
		rgba[2] = (uint8_t)(water->def.color[2] * bright);
	}
}

void water_update(float delta)
{
	for (uint8_t i = 0; i < water_count; i++) {
		Water *water = &water_pool[i];

		water->time += delta;
		water_scroll(water->offset_a, water->def.scroll_a, water->def.wrap_a, delta);
		water_scroll(water->offset_b, water->def.scroll_b, water->def.wrap_b, delta);

		if (water->culled && *water->culled) continue;

		water_waves(water);
	}
}

/* Same sum as water_waves, at one arbitrary point instead of the mesh's:
   the buoyancy samples ask here, so a body floats on the exact surface the
   player sees. Waves ride on the rest height, which lives in mesh space —
   identical to world space, since water models bake their placement. */
float water_surfaceHeight(const Water *water, float x, float y)
{
	const WaterDef *def = &water->def;
	float height = water->base_z;

	for (uint8_t w = 0; w < def->wave_count; w++) {
		const WaterWave *wave = &def->wave[w];

		float phase = (wave->direction_x * x + wave->direction_y * y)
		            * wave->frequency + water->time * wave->speed;
		height += wave->amplitude * fm_sinf(phase);
	}

	return height;
}

static float water_volumeSurfaceHeight(const void *surface, float x, float y)
{
	return water_surfaceHeight(surface, x, y);
}

void water_bindPhysics(Water *water, struct RigidBody *body, struct PhysicsWorld *world)
{
	water->volume = (BuoyancyVolume){
		.body           = body,
		.density        = water->def.density,
		.linear_drag    = water->def.linear_drag,
		.angular_drag   = water->def.angular_drag,
		.surface_height = water_volumeSurfaceHeight,
		.surface        = water,
	};

	physicsWorld_addBuoyancy(world, &water->volume);
}

void water_clear(void)
{
	for (uint8_t i = 0; i < water_count; i++) {
		free(water_pool[i].position);
		free(water_pool[i].rgba);
		water_pool[i] = (Water){0};
	}
	water_count = 0;
}
