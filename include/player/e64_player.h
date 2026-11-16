#ifndef ENGINE64_PLAYER_H
#define ENGINE64_PLAYER_H

#include "controller/e64_controller.h"
#include "character3d/e64_character3d_movement.h"
#include "character2d/e64_character2d_movement.h"

namespace e64 {

class Character3D;
class Character2D;


namespace player {

/* Which kind of body the seat drives. An empty seat is zero, so a player
   nobody took has no body rather than a 3D one that is missing. */
typedef enum {

	CHARACTER_NONE,
	CHARACTER_3D,
	CHARACTER_2D,

} CharacterType;

}


typedef struct Player {

	player::CharacterType type;

	/* The body and the command that drives it, together: they are the same
	   kind or the seat makes no sense. Which one is live is the type's to
	   say. */
	union {
		struct {
			Character3D *character;
			character3d::MovementCommand cmd;
		} character3d;

		struct {
			Character2D *character;
			character2d::MovementCommand cmd;
		} character2d;
	};

} Player;


namespace player {

Player *get(void);
void init(void);
/* Seats a player on the body it drives. Seating one kind of body leaves the
   seat driving that kind and no other. */
void setCharacter3D(PlayerID id, Character3D *character);
void setCharacter2D(PlayerID id, Character2D *character);
void update(void);

}

}

#endif
