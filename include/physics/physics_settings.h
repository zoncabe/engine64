/*
	Ported from qu3e q3Settings.h — altered source, not the original software.

	Copyright (c) 2014 Randy Gaul http://www.randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef PHYSICS_SETTINGS_H
#define PHYSICS_SETTINGS_H

#include "physics/math/math_common.h"

#define PHYSICS_SLEEP_LINEAR      0.01f
#define PHYSICS_SLEEP_ANGULAR     ((3.0f / 180.0f) * PI)
#define PHYSICS_SLEEP_TIME        0.4f
#define PHYSICS_BAUMGARTE         0.2f
#define PHYSICS_PENETRATION_SLOP  0.01f

#define PHYSICS_SOLVER_ITERATIONS 8
#define PHYSICS_TIMESTEP          (1.0f / 60.0f)

/* Margin added to every AABB in the broadphase tree, metres per side. Sets
   both the pair/wake distance (one margin per body, so twice this between
   surfaces) and how far a body may drift before its leaf is reinserted. */
#define PHYSICS_AABB_FATTENER     0.1f

/* Longest step a frame may take. Past this the simulation slows down instead
   of integrating a huge dt, which is what keeps a hiccup from exploding it. */
#define PHYSICS_MAX_TIMESTEP      (1.0f / 20.0f)

/* Cloths step on their own fixed clock of PHYSICS_TIMESTEP: a Verlet cloth's
   look is tuned to its step size. Cap on cloth steps one frame may run;
   below 60/cap FPS the cloth slows down instead of piling up debt. */
#define PHYSICS_CLOTH_MAX_SUBSTEPS 3

#endif
