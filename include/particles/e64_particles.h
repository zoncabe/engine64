#ifndef ENGINE64_PARTICLES_H
#define ENGINE64_PARTICLES_H

#include <stdbool.h>
#include <stdint.h>
#include <t3d/t3dmath.h>
#include <t3d/tpx.h>

/* tpx stores particles interleaved in pairs, so a buffer always holds an even
   count. S8 keeps local coords in one byte (16 bytes per pair) for local
   effects; S16 covers a larger range (24 bytes per pair) for world placement. */
typedef enum {

	PARTICLE_S8,
	PARTICLE_S16,

} ParticleType;

typedef struct {

	ParticleType type;
	uint32_t count;

	union {
		TPXParticleS8  *s8;
		TPXParticleS16 *s16;
	};

	T3DMat4FP *matrix;   /* one per framebuffer */

} ParticleBuffer;


typedef struct GameContext GameContext;
typedef struct Particle Particle;

/* Dedicated input function: reads whatever drives the effect and fills
   visibility and this frame's matrix. */
typedef void (*ParticleUpdate)(Particle *particle, const GameContext *ctx, uint8_t fb_index);

/* rdpq state (combiner, textures) set right before the buffer is drawn. */
typedef void (*ParticleSetRenderState)(void);

struct Particle {

	ParticleBuffer buffer;
	ParticleUpdate update;
	ParticleSetRenderState set_render_state;
	bool textured;
	bool visible;
	const T3DMat4FP *matrix;   /* the one written this frame */

};


void particles_init(void);

Particle *particles_add(const Particle *def);
void particles_update(const GameContext *ctx, uint8_t fb_index);
void particles_draw(void);

ParticleBuffer particleBuffer_create(ParticleType type, uint32_t count);
void particleBuffer_delete(ParticleBuffer *buffer);
void particleBuffer_setMatrix(ParticleBuffer *buffer, const float scale[3], const float rotation[3], const float position[3], uint8_t fb_index);
void particleBuffer_draw(const ParticleBuffer *buffer, const T3DMat4FP *matrix);
void particleBuffer_drawTextured(const ParticleBuffer *buffer, const T3DMat4FP *matrix);

#endif
