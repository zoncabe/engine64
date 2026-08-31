/*
	tpx (tinyPX) particle buffers. tpx is its own RSP ucode, independent of
	t3d: nothing set up in t3d carries over, so every draw copies the current
	t3d state first and pushes the buffer matrix on tpx's own stack.
*/
#include <assert.h>
#include <malloc.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/tpx.h>

#include "viewport/e64_viewport.h"
#include "particles/e64_particles.h"


#define PARTICLES_MAX 8

static Particle particle[PARTICLES_MAX];
static uint8_t particle_count;


void particles_init(void)
{
	tpx_init((TPXInitParams){});
}

Particle *particles_add(const Particle *def)
{
	assert(particle_count < PARTICLES_MAX);

	Particle *added = &particle[particle_count++];
	*added = *def;
	return added;
}

void particles_update(const GameContext *ctx, uint8_t fb_index)
{
	for (int i = 0; i < particle_count; i++)
		particle[i].update(&particle[i], ctx, fb_index);
}

void particles_draw(void)
{
	for (int i = 0; i < particle_count; i++) {
		Particle *drawn = &particle[i];
		if (!drawn->visible || !drawn->matrix) continue;

		drawn->set_render_state();

		if (drawn->textured) particleBuffer_drawTextured(&drawn->buffer, drawn->matrix);
		else                 particleBuffer_draw(&drawn->buffer, drawn->matrix);
	}
}


ParticleBuffer particleBuffer_create(ParticleType type, uint32_t count)
{
	assert(count % 2 == 0);

	uint32_t pair_size = type == PARTICLE_S8 ? sizeof(TPXParticleS8) : sizeof(TPXParticleS16);

	ParticleBuffer buffer = {
		.type   = type,
		.count  = count,
		.matrix = malloc_uncached(sizeof(T3DMat4FP) * FB_COUNT),
	};
	buffer.s8 = malloc_uncached(pair_size * count / 2);

	return buffer;
}

void particleBuffer_delete(ParticleBuffer *buffer)
{
	free_uncached(buffer->s8);
	free_uncached(buffer->matrix);
	*buffer = (ParticleBuffer){0};
}

void particleBuffer_setMatrix(ParticleBuffer *buffer, const float scale[3], const float rotation[3], const float position[3], uint8_t fb_index)
{
	t3d_mat4fp_from_srt_euler(&buffer->matrix[fb_index], scale, rotation, position);
}


static void particleBuffer_drawWithMatrix(const ParticleBuffer *buffer, const T3DMat4FP *matrix, bool textured)
{
	tpx_state_from_t3d();
	tpx_matrix_push((T3DMat4FP *)matrix);
	tpx_state_set_scale(1.0f, 1.0f);

	if (textured) {
		tpx_state_set_tex_params(0, 0);
		if (buffer->type == PARTICLE_S8) tpx_particle_draw_tex_s8(buffer->s8, buffer->count);
		else                             tpx_particle_draw_tex_s16(buffer->s16, buffer->count);
	} else {
		if (buffer->type == PARTICLE_S8) tpx_particle_draw_s8(buffer->s8, buffer->count);
		else                             tpx_particle_draw_s16(buffer->s16, buffer->count);
	}

	tpx_matrix_pop(1);
}

void particleBuffer_draw(const ParticleBuffer *buffer, const T3DMat4FP *matrix)
{
	particleBuffer_drawWithMatrix(buffer, matrix, false);
}

/* Expects the texture already uploaded via rdpq: the ucode never loads or
   switches textures itself. */
void particleBuffer_drawTextured(const ParticleBuffer *buffer, const T3DMat4FP *matrix)
{
	particleBuffer_drawWithMatrix(buffer, matrix, true);
}
