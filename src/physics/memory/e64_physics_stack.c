/*
	Ported from qu3e q3Memory.cpp — altered source, not the original software.

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

#include <assert.h>
#include <string.h>

#include "physics/memory/e64_physics_stack.h"
#include "physics/memory/e64_physics_memory.h"


void physicsStack_init(PhysicsStack *s)
{
	s->memory         = NULL;
	s->entries        = (PhysicsStackEntry *)physics_alloc((int32_t)(sizeof(PhysicsStackEntry) * PHYSICS_STACK_INITIAL_ENTRIES));
	s->index          = 0;
	s->allocation     = 0;
	s->entry_count    = 0;
	s->entry_capacity = PHYSICS_STACK_INITIAL_ENTRIES;
	s->stack_size     = 0;
}


void physicsStack_shutdown(PhysicsStack *s)
{
	if (s->memory) physics_free(s->memory);
	assert(s->index == 0);
	assert(s->entry_count == 0);
	if (s->entries) physics_free(s->entries);
	s->memory  = NULL;
	s->entries = NULL;
}


void physicsStack_reserve(PhysicsStack *s, uint32_t size)
{
	assert(!s->index);
	if (size == 0) return;

	if (size >= s->stack_size) {
		if (s->memory) physics_free(s->memory);
		s->memory     = (uint8_t *)physics_alloc((int32_t)size);
		s->stack_size = size;
	}
}


void *physicsStack_allocate(PhysicsStack *s, int32_t size)
{
	assert(s->index + (uint32_t)size <= s->stack_size);

	if (s->entry_count == s->entry_capacity) {
		PhysicsStackEntry *old = s->entries;
		s->entry_capacity *= 2;
		s->entries = (PhysicsStackEntry *)physics_alloc((int32_t)(s->entry_capacity * (int32_t)sizeof(PhysicsStackEntry)));
		memcpy(s->entries, old, (size_t)s->entry_count * sizeof(PhysicsStackEntry));
		physics_free(old);
	}

	PhysicsStackEntry *entry = s->entries + s->entry_count;
	entry->size = size;
	entry->data = s->memory + s->index;
	s->index += (uint32_t)size;

	s->allocation += size;
	++s->entry_count;

	return entry->data;
}


void physicsStack_free(PhysicsStack *s, void *data)
{
	assert(s->entry_count > 0);
	PhysicsStackEntry *entry = s->entries + s->entry_count - 1;
	assert(data == entry->data);
	(void)data;

	s->index      -= (uint32_t)entry->size;
	s->allocation -= entry->size;
	--s->entry_count;
}
