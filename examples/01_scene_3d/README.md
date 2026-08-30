# 01 scene 3d

The smallest world engine64 can put on screen: a room, a capsule standing in the middle of it, and a camera to look around.

Everything in `main.c` is content. The assets, where they are placed, the light, the camera and the one game state that draws them are all data the game hands over; the engine supplies the rest.

### Camera

The scene definition carries the whole camera: lens, clipping planes, arm length, angles, and the settings the arm runs on. The engine keeps no defaults, so a value left out is zero and the camera behaves accordingly.

Control is a binding: the game names a button per action and the engine does the rest every frame.

| Action | Button |
| --- | --- |
| Turn and tilt | C buttons |
| Pull in and out | L and R |
| Narrow and widen the lens | D-Up and D-Down |

### Building

`ENGINE_DIR` at the top of the `Makefile` points at the engine checkout, two levels up from here. With the Libdragon toolchain and Tiny3D installed:

```
make
```
