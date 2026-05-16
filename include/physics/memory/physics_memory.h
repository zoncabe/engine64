/*
	physics_memory.h — low-level malloc/free wrappers + PTR_ADD helper.
	Shared by PhysicsStack / PhysicsHeap / PhysicsPagedAllocator.
*/
#ifndef PHYSICS_MEMORY_H
#define PHYSICS_MEMORY_H

#include <stdint.h>
#include <stdlib.h>


static inline void *physics_alloc(int32_t bytes) { return malloc((size_t)bytes); }
static inline void  physics_free(void *memory)   { free(memory); }

#define PHYSICS_PTR_ADD(P, BYTES) ((void*)(((uint8_t*)(P)) + (BYTES)))


#endif
