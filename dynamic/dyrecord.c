/* 
 * Copyright (C) 2025 Seth McDonald
 * 
 * This file is part of Collatz Conjecture Simulator.
 * 
 * Collatz Conjecture Simulator is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * 
 * Collatz Conjecture Simulator is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with Collatz Conjecture
 * Simulator. If not, see <https://www.gnu.org/licenses/>.
 */

#include "dyrecord.h"
#include "dynamic.h"


struct Node
{
	struct Node* next;
	void* memory;
	FreeCallback callback;
};

struct DyRecord_
{
	size_t count; // Number of nodes
	struct Node* top; // Last added node
};


void dyrecord_destroy(struct DyRecord_* restrict record)
{
	if CZ_NOEXPECT (!record) { return; }

	dyrecord_free(record);
	czFree(record);
}

struct DyRecord_* dyrecord_create(void)
{
	struct DyRecord_* restrict record = NULL;
	struct CzAllocFlags flags = {0};
	flags.zeroInitialise = true;

	czAlloc((void* restrict*) &record, sizeof(*record), flags);
	return record;
}

size_t dyrecord_size(struct DyRecord_* restrict record)
{
	return record->count;
}

bool dyrecord_add(struct DyRecord_* restrict record, void* restrict memory, FreeCallback callback)
{
	size_t count = record->count;
	struct Node* top = record->top;

	struct Node* restrict node;
	struct CzAllocFlags flags = {0};

	enum CzResult czres = czAlloc((void* restrict*) &node, sizeof(*node), flags);
	if CZ_NOEXPECT (czres) { return false; }

	node->next = top;
	node->memory = memory;
	node->callback = callback;

	record->count = count + 1;
	record->top = node;
	return true;
}

void* dyrecord_malloc(struct DyRecord_* restrict record, size_t size)
{
	CZ_ASSUME(size != 0);

	void* restrict memory;
	struct CzAllocFlags flags = {0};

	enum CzResult czres = czAlloc(&memory, size, flags);
	if CZ_NOEXPECT (czres) { return NULL; }

	bool bres = dyrecord_add(record, memory, czFree_stub);
	if CZ_NOEXPECT (!bres) { goto err_free_memory; }
	return memory;

err_free_memory:
	czFree(memory);
	return NULL;
}

void* dyrecord_calloc(struct DyRecord_* restrict record, size_t count, size_t size)
{
	CZ_ASSUME(count != 0);
	CZ_ASSUME(size != 0);

	void* restrict memory;
	struct CzAllocFlags flags = {0};
	flags.zeroInitialise = true;

	enum CzResult czres = czAlloc(&memory, size * count, flags);
	if CZ_NOEXPECT (czres) { return NULL; }

	bool bres = dyrecord_add(record, memory, czFree_stub);
	if CZ_NOEXPECT (!bres) { goto err_free_memory; }
	return memory;

err_free_memory:
	czFree(memory);
	return NULL;
}

void dyrecord_free(struct DyRecord_* restrict record)
{
	struct Node* node = record->top;

	while (node) {
		struct Node* next = node->next;
		void* memory = node->memory;
		FreeCallback callback = node->callback;

		callback(memory);
		czFree(node);
		node = next;
	}

	record->count = 0;
}
