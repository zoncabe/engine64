/*
	physics_heap.h — first-fit heap allocator (from qu3e q3Heap).
	N64-sized to 256 KB; bump if the engine needs more.
*/
#ifndef PHYSICS_HEAP_H
#define PHYSICS_HEAP_H

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
