/*
	physics_paged_allocator.h — pool allocator with fixed-size pages
	(from qu3e q3PagedAllocator). Constant-time alloc/free for fixed-size
	structs (e.g. ContactConstraint).
*/
#ifndef PHYSICS_PAGED_ALLOCATOR_H
#define PHYSICS_PAGED_ALLOCATOR_H

#include <stdint.h>


typedef struct PhysicsBlock {
	struct PhysicsBlock *next;
} PhysicsBlock;


typedef struct PhysicsPage {
	struct PhysicsPage *next;
	PhysicsBlock       *data;
} PhysicsPage;


typedef struct PhysicsPagedAllocator {
	int32_t       block_size;
	int32_t       blocks_per_page;
	PhysicsPage  *pages;
	int32_t       page_count;
	PhysicsBlock *free_list;
} PhysicsPagedAllocator;


void  physicsPagedAllocator_init(PhysicsPagedAllocator *a, int32_t element_size, int32_t elements_per_page);
void  physicsPagedAllocator_shutdown(PhysicsPagedAllocator *a);
void *physicsPagedAllocator_allocate(PhysicsPagedAllocator *a);
void  physicsPagedAllocator_free(PhysicsPagedAllocator *a, void *data);
void  physicsPagedAllocator_clear(PhysicsPagedAllocator *a);


#endif
