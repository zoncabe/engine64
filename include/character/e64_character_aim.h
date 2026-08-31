/*
	Camera-pitch aim bend, split evenly along a spine chain.

	The delta is never added in a bone's own frame: the aiming pose keeps the
	torso half-turned at whatever angle the clip authored, so a local axis is
	tilted with it. The bend is built once about the world's horizontal side
	axis and conjugated into each bone's frame through the rotation the chain
	actually carries this frame, which keeps it pure pitch under any twist.
*/
#ifndef ENGINE64_CHARACTER_AIM_H
#define ENGINE64_CHARACTER_AIM_H

#include <stdint.h>
#include <t3d/t3dskeleton.h>

typedef struct Character Character;
typedef struct Camera    Camera;

#define CHARACTER_AIM_MAX_BONES 4

typedef struct CharacterAimingSettings {

	const char *const *bone;   /* spine chain, root to tip */
	uint8_t count;
	float pitch_scale;         /* spine degrees per camera degree, sign included */

} CharacterAimingSettings;

/* Resolved once at create: the names above become indices so the bend never
   searches the skeleton by string, and the scale rides along so the bend has
   everything it needs without reaching back into the def. */
typedef struct CharacterAiming {

	int16_t bone[CHARACTER_AIM_MAX_BONES];
	uint8_t count;
	float   pitch_scale;

} CharacterAiming;


void characterAim_init(Character *character, const CharacterAimingSettings *settings);

/* SkeletonModifierFn; context is the Character. Weighted by the aim blend,
   so the torso straightens on its own when the mode fades. */
void characterAim_apply(T3DSkeleton *skeleton, void *context);

#endif
