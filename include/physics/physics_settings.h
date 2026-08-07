
#ifndef PHYSICS_SETTINGS_H
#define PHYSICS_SETTINGS_H

#include "physics/math/math_common.h"

#define PHYSICS_SLEEP_LINEAR      0.01f
#define PHYSICS_SLEEP_ANGULAR     ((3.0f / 180.0f) * PI)
#define PHYSICS_SLEEP_TIME        0.5f
#define PHYSICS_BAUMGARTE         0.2f
#define PHYSICS_PENETRATION_SLOP  0.05f

#define PHYSICS_SOLVER_ITERATIONS 2
#define PHYSICS_TIMESTEP          (1.0f / 60.0f)

/* Cap on the steps a single frame may run. Without it a long frame queues up
   more steps than the next frame can afford, which only makes it longer. */
#define PHYSICS_MAX_SUBSTEPS      4

#endif
