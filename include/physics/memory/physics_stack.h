/*
	physics_stack.h — LIFO stack allocator (from qu3e q3Stack).
	Reserve() reserva un bloque contiguo; Allocate() y Free() deben pedirse/
	devolverse en orden LIFO estricto.
*/
#ifndef PHYSICS_STACK_H
#define PHYSICS_STACK_H

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
