/*
	Procedural water surface.

	Drives a subdivided plane through meshDeform, the same path the cloth
	uses: a welded collision mesh of the plane seeds the points, a sum of
	sine waves moves them, and analytic normals from the same sum keep the
	shading and any generated UVs alive.

	The two texture layers scroll through the material's tile settings, so
	the mesh must be drawn through the per-frame material path (recorded
	objects), never a fully recorded display list.
*/
#ifndef ENGINE64_WATER_H
#define ENGINE64_WATER_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dmodel.h>

#include "physics/math/e64_vector3.h"
#include "physics/buoyancy/e64_buoyancy.h"
#include "sound/e64_sound.h"

#define WATER_MAX_WAVES    3
#define WATER_MAX_SURFACES 2


struct RigidBody;
struct PhysicsWorld;


/* One directional sine. Direction is in the horizontal plane and gets
   normalized at creation; amplitude is in metres, like the cloth. A single
   sine reads as a machine, so stack two or three with unrelated frequencies. */
typedef struct WaterWave {
	float direction_x;
	float direction_y;
	float amplitude;    /* metres */
	float frequency;    /* radians per metre along the direction */
	float speed;        /* radians per second */
} WaterWave;

/* Authoring side. The particles are seeded from a welded collision mesh of
   the same plane, exactly like the cloth, so the topology comes from the
   asset. Scroll speeds are in texels per second; wrap is the texture size in
   texels, so the offset can fold instead of growing without bound. */
typedef struct WaterDef {

	const char *mesh_path;

	/* Splash of this surface, played for whatever enters it. One is picked
	   at random; the plunge speed sets the volume. */
	const SoundID *entry_sound;
	uint8_t        entry_sound_count;
	
	WaterWave wave[WATER_MAX_WAVES];
	uint8_t   wave_count;

	float scroll_a[2];   /* tile 0, s/t texels per second */
	float scroll_b[2];   /* tile 1 */
	float wrap_a;        /* texture size of tile 0, texels */
	float wrap_b;        /* texture size of tile 1 */

	/* Water tint, written as vertex color and scaled by the wave height:
	   crests lighter, troughs darker, like the lava of t3d's example 04.
	   The material multiplies it in through SHADE. */
	uint8_t color[3];

	/* Buoyancy, fed to the physics world when the surface is bound to a
	   sensor body. Zero means the default: fresh water and a mild drag. */
	float density;        /* kg/m3 */
	float linear_drag;    /* per-second rate on the linear velocity */
	float angular_drag;   /* per-second rate on the angular velocity */

} WaterDef;


typedef struct Water {
	Vector3 *position;   /* animated points, metres; the mesh reads these */
	Vector3 *normal;     /* analytic, from the wave derivatives */
	Vector3 *rest;       /* the flat pose the waves displace from */
	uint8_t *rgba;       /* tint shaded by wave height, 4 per point */
	uint16_t count;

	float amplitude_sum; /* of every wave; normalises the height for shading */
	float base_z;        /* resting surface height, metres */

	BuoyancyVolume volume;   /* lent to the physics world by water_bindPhysics */

	float time;
	float offset_a[2];   /* accumulated scroll, texels */
	float offset_b[2];

	const bool *culled;  /* the mesh's flag; waves are skipped out of view */

	WaterDef def;

	/* Handed to the render path so the material setup scrolls the tiles. */
	T3DModelDrawConf conf;
} Water;


/* Loads the collision mesh, seeds the points at rest and registers the
   surface for update. NULL if the pool is full or the mesh did not load. */
Water *water_create(const WaterDef *def);

/* Advances every registered surface. Time always moves, so a culled pool
   does not freeze mid-wave; only the per-point work is skipped. */
void water_update(float delta);

/* Height of the surface over any world (x, y), metres: the resting height
   plus the same wave sum the render points use, evaluated analytically. */
float water_surfaceHeight(const Water *water, float x, float y);

/* Binds the surface to the static body carrying the water's sensor shape
   and registers the pair as a buoyancy volume in the world. From then on
   every dynamic body inside the sensor floats against these waves. */
void water_bindPhysics(Water *water, struct RigidBody *body, struct PhysicsWorld *world);

/* Deletes every registered surface. Runs with scene3d_unload. */
void water_clear(void);

#endif
