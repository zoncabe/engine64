#include <assert.h>
#include <string.h>

#include "physics/memory/physics_heap.h"
#include "physics/memory/physics_memory.h"


void physicsHeap_init(PhysicsHeap *h)
{
	h->memory = (PhysicsHeader *)physics_alloc(PHYSICS_HEAP_SIZE);
	h->memory->next = NULL;
	h->memory->prev = NULL;
	h->memory->size = PHYSICS_HEAP_SIZE;

	h->free_blocks = (PhysicsFreeBlock *)physics_alloc((int32_t)(sizeof(PhysicsFreeBlock) * PHYSICS_HEAP_INITIAL_CAPACITY));
	h->free_block_count    = 1;
	h->free_block_capacity = PHYSICS_HEAP_INITIAL_CAPACITY;

	h->free_blocks->header = h->memory;
	h->free_blocks->size   = PHYSICS_HEAP_SIZE;
}


void physicsHeap_shutdown(PhysicsHeap *h)
{
	physics_free(h->memory);
	physics_free(h->free_blocks);
	h->memory      = NULL;
	h->free_blocks = NULL;
}


void *physicsHeap_allocate(PhysicsHeap *h, int32_t size)
{
	int32_t size_needed = size + (int32_t)sizeof(PhysicsHeader);
	PhysicsFreeBlock *first_fit = NULL;

	for (int32_t i = 0; i < h->free_block_count; ++i) {
		PhysicsFreeBlock *block = h->free_blocks + i;
		if (block->size >= size_needed) { first_fit = block; break; }
	}
	if (!first_fit) return NULL;

	PhysicsHeader *node     = first_fit->header;
	PhysicsHeader *new_node = (PhysicsHeader *)PHYSICS_PTR_ADD(node, size_needed);
	node->size = size_needed;

	first_fit->size   -= size_needed;
	first_fit->header  = new_node;

	new_node->next = node->next;
	if (node->next) node->next->prev = new_node;
	node->next     = new_node;
	new_node->prev = node;

	return PHYSICS_PTR_ADD(node, sizeof(PhysicsHeader));
}


void physicsHeap_free(PhysicsHeap *h, void *memory)
{
	assert(memory);
	PhysicsHeader *node = (PhysicsHeader *)PHYSICS_PTR_ADD(memory, -(int32_t)sizeof(PhysicsHeader));

	PhysicsHeader    *next            = node->next;
	PhysicsHeader    *prev            = node->prev;
	PhysicsFreeBlock *next_block      = NULL;
	int32_t           prev_block_idx  = ~0;
	PhysicsFreeBlock *prev_block      = NULL;
	int32_t           free_block_count = h->free_block_count;

	for (int32_t i = 0; i < free_block_count; ++i) {
		PhysicsFreeBlock *block  = h->free_blocks + i;
		PhysicsHeader    *header = block->header;
		if (header == next)      next_block = block;
		else if (header == prev) { prev_block = block; prev_block_idx = i; }
	}

	int merged = 0;

	if (prev_block) {
		merged = 1;
		prev->next = next;
		if (next) next->prev = prev;

		prev_block->size += node->size;
		prev->size        = prev_block->size;

		if (next_block) {
			next_block->header = prev;
			next_block->size  += prev->size;
			prev->size         = next_block->size;

			PhysicsHeader *nextnext = next->next;
			prev->next = nextnext;
			if (nextnext) nextnext->prev = prev;

			assert(h->free_block_count);
			assert(prev_block_idx != ~0);
			--h->free_block_count;
			h->free_blocks[prev_block_idx] = h->free_blocks[h->free_block_count];
		}
	}
	else if (next_block) {
		merged = 1;
		next_block->header = node;
		next_block->size  += node->size;
		node->size         = next_block->size;

		PhysicsHeader *nextnext = next->next;
		if (nextnext) nextnext->prev = node;
		node->next = nextnext;
	}

	if (!merged) {
		PhysicsFreeBlock block;
		block.header = node;
		block.size   = node->size;

		if (h->free_block_count == h->free_block_capacity) {
			PhysicsFreeBlock *old_blocks = h->free_blocks;
			int32_t           old_cap    = h->free_block_capacity;

			h->free_block_capacity *= 2;
			h->free_blocks = (PhysicsFreeBlock *)physics_alloc((int32_t)(sizeof(PhysicsFreeBlock) * h->free_block_capacity));
			memcpy(h->free_blocks, old_blocks, sizeof(PhysicsFreeBlock) * (size_t)old_cap);
			physics_free(old_blocks);
		}

		h->free_blocks[h->free_block_count++] = block;
	}
}
