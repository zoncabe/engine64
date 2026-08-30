# 02 character physics

A body in a room, and the smallest world that makes it show everything it can do: walk and run it, jump it, slide it along a wall, push it into the water and up the ladder.

Every piece of the scene is there because it exercises one part of the character controller. Nothing else is: no animation, no weapons, no sounds, no stamina spent.

### The scene

The room is 5000 across, five quads of a thousand a side, and its collision is the triangle mesh built from the same model, so what is drawn is what is walked on.

| Piece | What it is for |
| --- | --- |
| The mound | A slope: how the body climbs one and how it holds still on it |
| The platform, 500 tall | A wall to slide along, and an edge to fall off |
| The cube, the ball and the capsule | Three shapes to walk into, one flat, one round, one both |
| The ladder | A climbable volume: the one place the body leaves the floor without jumping |
| The pool, sunk into the middle quad | A buoyancy volume with a moving surface: float, sink and swim |

Physics runs in metres and the world is drawn a hundred times bigger, which is why the capsule that stands 180 units tall on screen is 1.8 in every collider.

### The character

The engine brings no movement of its own. What the body does is the `CharacterDef` in `prefabs/example_character.c` and nothing else: the capsule it wears, the three gaits it runs on, how it jumps, how much it steers in the air, how fast it climbs and how deep it floats. What that file leaves out does not happen.

Its body is kinematic. The solver never moves it: it moves itself, then walks out of whatever it ended up inside, classifies each contact as floor, wall or ceiling, and answers the volumes it is standing in. That whole step is one call, `scene3d_updateCharacters`, and it is the only physics this example runs.

The jump is `JUMP_SNAP`: the body leaves the floor on the press, and while A stays down the rise pays less gravity. Tapping it is a short hop, holding it all the way is the full one. The other mode, `JUMP_CHARGE`, crouches first and launches with what it gathered; the settings for it are in the same file, unused.

The ladder is asked for with the stick, never a button. Walking at the rungs grabs them, up and down climbs and descends, and A lets go. Both are read against the ladder's own facing, so the camera never changes what a push means.

### Controls

| Action | Button |
| --- | --- |
| Move | Stick |
| Sprint | Z |
| Jump, and let go of the ladder | A |
| Climb | Stick, on a ladder |
| Turn and tilt the camera | C buttons |
| Pull the camera in and out | L and R |
| Narrow and widen the lens | D-Up and D-Down |

### Building

`ENGINE_DIR` at the top of the `Makefile` points at the engine checkout, two levels up from here. With the Libdragon toolchain and Tiny3D installed:

```
make
```
