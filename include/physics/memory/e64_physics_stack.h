/*
	Ported from qu3e q3Memory.h — altered source, not the original software.

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

/*
	LIFO stack allocator. Reserve() takes one contiguous block; Allocate() and
	Free() must be requested and returned in strict LIFO order.
*/
#ifndef ENGINE64_PHYSICS_STACK_H
#define ENGINE64_PHYSICS_STACK_H

#include <stdint.h>


#define PHYSICS_STACK_INITIAL_ENTRIES 64


typedef struct PhysicsStackEntry {
	uint8_t *data;
	int32_t  size;
} PhysicsStackEntry;


typedef struct PhysicsStack {
	uint8_t           *memory;
	PhysicsStackEntry *entries;
	uint32_t           index;
	int32_t            allocation;
	int32_t            entry_count;
	int32_t            entry_capacity;
	uint32_t           stack_size;
} PhysicsStack;


void  physicsStack_init(PhysicsStack *s);
void  physicsStack_shutdown(PhysicsStack *s);
void  physicsStack_reserve(PhysicsStack *s, uint32_t size);
void *physicsStack_allocate(PhysicsStack *s, int32_t size);
void  physicsStack_free(PhysicsStack *s, void *data);


#endif
