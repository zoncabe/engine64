#ifndef ENGINE64_CHARACTER_WEAPON_H
#define ENGINE64_CHARACTER_WEAPON_H

#include <stdint.h>
#include <t3d/t3dmath.h>


typedef struct Character Character;

#define CHARACTER_WEAPON_DRAWN_NONE 0xFF


typedef enum {

	WEAPON_TYPE_HITSCAN,
	WEAPON_TYPE_BALLISTIC,
	WEAPON_TYPE_MELEE,
	WEAPON_TYPE_COUNT,

} WeaponType;

/* How the shoot button becomes a shot, the same split the jump uses. It has
   nothing to do with the weapon type: a hitscan laser can charge just as a
   bow does. */
typedef enum {
	SHOOT_CHARGE,
	SHOOT_SNAP,
} ShootMode;

typedef enum {

	WEAPON_SLOT_WAIST,
	WEAPON_SLOT_BACK,
	WEAPON_SLOT_MELEE,
	WEAPON_SLOT_COUNT,

} WeaponSlotID;


typedef struct WeaponDef {

	const char *mesh;            /* object name inside the character model */
	const char *bone;            /* bone the mesh is skinned to */
	const char *holster_bone;    /* reference bone while holstered */
	const char *hand_bone;       /* reference bone while drawn */

	uint8_t type;
	uint8_t shoot_mode;
	uint8_t magazine_size;
	uint8_t max_integrity;

	T3DVec3 holster_position;    /* relative to holster_bone */
	T3DQuat holster_rotation;
	T3DVec3 holding_position;    /* relative to hand_bone */
	T3DQuat holding_rotation;

} WeaponDef;

typedef struct WeaponSlot {

	const WeaponDef *weapon;     /* NULL = empty slot */
	uint8_t rounds;
	uint8_t integrity;

	uint8_t part;                /* resolved mesh part (visibility bit) */
	int16_t bone;                /* resolved bone indices */
	int16_t holster_bone;
	int16_t hand_bone;

} WeaponSlot;

/* Which model objects are weapons, in mesh part order. */
typedef struct CharacterWeaponsDef {

	const char *const *mesh;
	uint8_t mesh_count;

	/* What the character starts carrying. Only seeds the slots: equipping and
	   unequipping afterwards is runtime state on the Character. */
	const WeaponDef *weapon[WEAPON_SLOT_COUNT];

} CharacterWeaponsDef;

typedef struct CharacterWeapons {

	const CharacterWeaponsDef *def;
	WeaponSlot slot[WEAPON_SLOT_COUNT];
	uint8_t    drawn;            /* slot in hand, CHARACTER_WEAPON_DRAWN_NONE = all holstered */

} CharacterWeapons;


void character_equipWeapon  (Character *character, uint8_t slot, const WeaponDef *weapon);
void character_unequipWeapon(Character *character, uint8_t slot);

/* Steps the drawn weapon through the occupied slots, unarmed included as a
   stop of its own. dir +1 / -1. */
void character_cycleWeapon(Character *character, int8_t dir);

/* What the hand carries right now; NULL when everything is holstered. */
const WeaponDef *character_drawnWeapon(const Character *character);

/* Poses the weapon bones (holster or hand). Runs before the skeleton update. */
void characterWeapon_setBones(Character *character);


#endif
