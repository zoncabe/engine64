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
	First-fit heap allocator, sized to 256 KB for the N64.
*/
#ifndef ENGINE64_PHYSICS_HEAP_H
#define ENGINE64_PHYSICS_HEAP_H

#include <stdint.h>


#define PHYSICS_HEAP_SIZE               (256 * 1024)
#define PHYSICS_HEAP_INITIAL_CAPACITY   64


typedef struct PhysicsHeader {
	struct PhysicsHeader *next;
	struct PhysicsHeader *prev;
	int32_t size;
} PhysicsHeader;


typedef struct PhysicsFreeBlock {
	PhysicsHeader *header;
	int32_t size;
} PhysicsFreeBlock;


typedef struct PhysicsHeap {
	PhysicsHeader    *memory;
	PhysicsFreeBlock *free_blocks;
	int32_t           free_block_count;
	int32_t           free_block_capacity;
} PhysicsHeap;


void  physicsHeap_init(PhysicsHeap *h);
void  physicsHeap_shutdown(PhysicsHeap *h);
void *physicsHeap_allocate(PhysicsHeap *h, int32_t size);
void  physicsHeap_free(PhysicsHeap *h, void *memory);


#endif
