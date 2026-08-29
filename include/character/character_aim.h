/*
	Camera-pitch aim bend, split evenly along a spine chain.

	The delta is never added in a bone's own frame: the aiming pose keeps the
	torso half-turned at whatever angle the clip authored, so a local axis is
	tilted with it. The bend is built once about the world's horizontal side
	axis and conjugated into each bone's frame through the rotation the chain
	actually carries this frame, which keeps it pure pitch under any twist.
*/
#ifndef CHARACTER_AIM_H
#define CHARACTER_AIM_H

#include <stdint.h>
#include <t3d/t3dskeleton.h>

typedef struct Character Character;

#define CHARACTER_AIM_MAX_BONES 4


typedef struct CharacterAimDef {

	const char *const *bone;   /* chain, root to tip */
	uint8_t count;
	float pitch_scale;         /* spine degrees per camera degree, sign included */

} CharacterAimDef;

typedef struct CharacterAim {

	const CharacterAimDef *def;
	int16_t bone[CHARACTER_AIM_MAX_BONES];

	/* The spring arm's own control-driven pitch, read in place: wired when a
	   player takes the body, NULL on a body nobody drives. */
	const float *pitch;

} CharacterAim;


void characterAim_init(Character *character);

/* SkeletonModifierFn; context is the Character. Weighted by the aim blend,
   so the torso straightens on its own when the mode fades. */
void characterAim_apply(T3DSkeleton *skeleton, void *context);

#endif
