/*
	stamina_wheel.c — radial stamina gauge, a textured particle anchored to
	the character hip bone so it follows the run cycle.

	The radial cutout comes from the N64brew GameJam 2024 setup screen:
	TILE0 holds an angular sweep gradient and TILE1 the ring artwork; the
	combiner routes the sweep into alpha, and alpha compare discards every
	pixel whose angle lies past the current progress.
*/
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dskeleton.h>

#include "physics/math/math_common.h"
#include "physics/math/math_functions.h"
#include "physics/math/vector3.h"
#include "time/time.h"
#include "viewport/viewport.h"
#include "entity/entity.h"
#include "character/character.h"
#include "player/player.h"
#include "game/game.h"
#include "particles/particles.h"
#include "ui/stamina_wheel.h"

#define STAMINA_WHEEL_BONE     "mixamorig:Hips"

/* Offset from the hip, render units: X runs along the camera right axis so
   the wheel keeps its place on screen while the camera orbits, Z stays on
   the world vertical. */
#define STAMINA_WHEEL_OFFSET_X -18.0f
#define STAMINA_WHEEL_OFFSET_Z 115.0f

#define STAMINA_WHEEL_SIZE     9
#define STAMINA_WHEEL_SCALE    1.0f

/* Color bands over the normalized stamina, and the recovery pulse. Each band
   starts blending into the next one this span before its threshold. */
#define STAMINA_WHEEL_YELLOW_BAND  0.5f
#define STAMINA_WHEEL_RED_BAND     0.2f
#define STAMINA_WHEEL_BLEND_SPAN   0.05f
#define STAMINA_WHEEL_FADE_RATE    3.0f   /* alpha per second */
#define STAMINA_WHEEL_MAX_ALPHA    0.90f  /* never fully solid */
#define STAMINA_WHEEL_PULSE_SPEED  8.0f   /* radians per second */
#define STAMINA_WHEEL_GREEN_RATE   6.0f   /* recovery lerp response */

/* Completion burst: while the settled wheel fades out it pops in scale and
   the green flashes toward white. */
#define STAMINA_WHEEL_BURST_SCALE  0.35f
#define STAMINA_WHEEL_FLASH_MIX    0.6f

static const float wheel_green[3]  = {  80.0f, 200.0f, 90.0f };
static const float wheel_yellow[3] = { 230.0f, 200.0f, 40.0f };
static const float wheel_red[3]    = { 220.0f,  50.0f, 40.0f };


static sprite_t *wheel_sweep;   /* angular gradient, becomes the cutout alpha */
static sprite_t *wheel_ring;    /* ring artwork, tinted by the particle color */

static int16_t wheel_bone = -1;
static float wheel_progress = 1.0f;
static float wheel_alpha = 0.0f;
static float wheel_color[3] = { 80.0f, 200.0f, 90.0f };
static float wheel_pulse = 0.0f;


/* Banded while draining, pulsing yellow-to-red while tired (less red as it
   refills), easing back to green once full. */
static void staminaWheel_setColor(const PlayerStats *stats, float dt)
{
	if (stats->tired) {
		wheel_pulse += dt * STAMINA_WHEEL_PULSE_SPEED;
		float red_mix = (0.5f + 0.5f * fm_sinf(wheel_pulse)) * (1.0f - stats->stamina);
		for (int i = 0; i < 3; i++)
			wheel_color[i] = wheel_yellow[i] + (wheel_red[i] - wheel_yellow[i]) * red_mix;
		return;
	}

	if (stats->stamina >= 1.0f) {
		float t = clampf(STAMINA_WHEEL_GREEN_RATE * dt, 0.0f, 1.0f);
		for (int i = 0; i < 3; i++)
			wheel_color[i] += (wheel_green[i] - wheel_color[i]) * t;
		return;
	}

	const float *lower, *upper;
	float band;
	if (stats->stamina >= STAMINA_WHEEL_YELLOW_BAND) {
		lower = wheel_yellow; upper = wheel_green;  band = STAMINA_WHEEL_YELLOW_BAND;
	} else {
		lower = wheel_red;    upper = wheel_yellow; band = STAMINA_WHEEL_RED_BAND;
	}

	float t = clampf((stats->stamina - band) / STAMINA_WHEEL_BLEND_SPAN, 0.0f, 1.0f);
	for (int i = 0; i < 3; i++)
		wheel_color[i] = lower[i] + (upper[i] - lower[i]) * t;
}

static bool staminaWheel_colorIsGreen(void)
{
	for (int i = 0; i < 3; i++)
		if (fabsf(wheel_color[i] - wheel_green[i]) > 2.0f) return false;
	return true;
}


static void staminaWheel_setInput(Particle *particle, const GameContext *ctx, uint8_t fb_index)
{
	const Player *player = &ctx->player[0];
	const Character *character = player->character;

	if (!character) { particle->visible = false; return; }

	float dt = time_get()->delta;
	staminaWheel_setColor(&player->stats, dt);

	/* Fade in whenever stamina is short; fade out while aiming, or once the
	   color has settled back on green so the lerp is seen before the wheel
	   leaves. */
	bool aiming  = ctx->viewport->camera.spherical.state == CAMERA_SPHERICAL_AIMING;
	bool settled = player->stats.stamina >= 1.0f && staminaWheel_colorIsGreen();
	float fade_step = STAMINA_WHEEL_FADE_RATE * dt;
	wheel_alpha += clampf((aiming || settled ? 0.0f : STAMINA_WHEEL_MAX_ALPHA) - wheel_alpha, -fade_step, fade_step);

	particle->visible = wheel_alpha > 0.0f;
	if (!particle->visible) return;

	wheel_progress = player->stats.stamina;

	/* 0 while active; climbs to 1 along the settled fade-out. */
	float burst = settled ? 1.0f - wheel_alpha / STAMINA_WHEEL_MAX_ALPHA : 0.0f;

	for (int i = 0; i < 3; i++) {
		float flash = wheel_color[i] + (255.0f - wheel_color[i]) * STAMINA_WHEEL_FLASH_MIX * burst;
		particle->buffer.s8[0].colorA[i] = (uint8_t)flash;
	}

	const T3DSkeleton *skeleton = &character->animation.main;
	if (wheel_bone < 0) {
		wheel_bone = (int16_t)t3d_skeleton_find_bone((T3DSkeleton *)skeleton, STAMINA_WHEEL_BONE);
		if (wheel_bone < 0) { particle->visible = false; return; }
	}

	T3DVec3 bone_position;
	T3DQuat bone_rotation;
	character_getBonePose(skeleton, wheel_bone, &bone_position, &bone_rotation);

	/* Model space to world with the same matrix the renderer builds for the
	   mesh, then the offset keeps the wheel clear of the body. */
	const RenderTransform *transform = &character->entity->transform;
	T3DMat4 world;
	t3d_mat4_from_srt_euler(&world,
		(float[3]){transform->scale.x,                 transform->scale.y,                 transform->scale.z},
		(float[3]){deg_to_rad(transform->rotation.x),  deg_to_rad(transform->rotation.y),  deg_to_rad(transform->rotation.z)},
		(float[3]){transform->position.x,              transform->position.y,              transform->position.z}
	);

	T3DVec4 position;
	t3d_mat4_mul_vec3(&position, &world, &bone_position);

	const Camera *camera = &ctx->viewport->camera;
	Vector3 forward = vector3_difference(&camera->target, &camera->position);
	Vector3 right = vector3_cross(&forward, &(Vector3){0.0f, 0.0f, 1.0f});
	right = vector3_normalized(&right);

	float scale = STAMINA_WHEEL_SCALE * (1.0f + STAMINA_WHEEL_BURST_SCALE * burst);

	particleBuffer_setMatrix(&particle->buffer,
		(float[3]){scale, scale, scale},
		(float[3]){0.0f, 0.0f, 0.0f},
		(float[3]){
			position.v[0] + right.x * STAMINA_WHEEL_OFFSET_X,
			position.v[1] + right.y * STAMINA_WHEEL_OFFSET_X,
			position.v[2] + STAMINA_WHEEL_OFFSET_Z,
		},
		fb_index);

	particle->matrix = &particle->buffer.matrix[fb_index];
}

static void staminaWheel_setRenderState(void)
{
	rdpq_set_mode_standard();
	rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);

	/* Verbatim from the N64brew GameJam 2024 setup screen (drawprogress):
	   cycle 1 tints the ring (TEX1) with prim and takes alpha from the sweep
	   (TEX0); cycle 2 hands the sweep to the alpha compare, which cuts the
	   wheel at the current progress angle. Prim color comes from the
	   particle color instead of rdpq_set_prim_color. */
	rdpq_mode_combiner(RDPQ_COMBINER2(
		(TEX1,0,PRIM,0),  (0,0,0,TEX0),
		(0,0,0,COMBINED), (0,0,0,TEX1)
	));
	rdpq_mode_alphacompare((1.0f - wheel_progress) * 255.0f);

	/* tpx maps UVs to an 8x8px base no matter the real size: scale_log
	   stretches the 32px textures over the whole particle. */
	rdpq_texparms_t parms = { .s.scale_log = -2, .t.scale_log = -2 };
	rdpq_tex_multi_begin();
	rdpq_sprite_upload(TILE0, wheel_sweep, &parms);
	rdpq_sprite_upload(TILE1, wheel_ring, &parms);
	rdpq_tex_multi_end();
}

float stamina_wheel_getProgress(void)
{
	return wheel_progress;
}

void stamina_wheel_init(void)
{
	Particle wheel = {
		.buffer           = particleBuffer_create(PARTICLE_S8, 2),
		.update           = staminaWheel_setInput,
		.set_render_state = staminaWheel_setRenderState,
		.textured         = true,
	};

	/* One particle at the buffer origin; the pair rule leaves B at size 0.
	   Color goes out as prim color, its alpha is a texture offset. */
	wheel.buffer.s8[0] = (TPXParticleS8){
		.posA   = { 0, 0, 0 },
		.sizeA  = STAMINA_WHEEL_SIZE,
		.colorA = { 80, 200, 90, 0 },
	};

	wheel_sweep = sprite_load("rom:/textures/CircleMask.i8.sprite");
	wheel_ring  = sprite_load("rom:/textures/CircleProgress.i8.sprite");

	particles_add(&wheel);
}
