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
	if NOEXPECT (!record) { return; }

	dyrecord_free(record);
	free(record);
}

struct DyRecord_* dyrecord_create(void)
{
	size_t size = sizeof(struct DyRecord_);
	struct DyRecord_* record = malloc(size);
	if NOEXPECT (!record) { MALLOC_FAILURE(record, size); return NULL; }

	record->count = 0;
	record->top = NULL;

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

	size_t size = sizeof(struct Node);
	struct Node* node = malloc(size);
	if NOEXPECT (!node) { MALLOC_FAILURE(node, size); return false; }

	node->next = top;
	node->memory = memory;
	node->callback = callback;

	record->count = count + 1;
	record->top = node;

	return true;
}

void* dyrecord_malloc(struct DyRecord_* restrict record, size_t size)
{
	ASSUME(size != 0);

	void* memory = malloc(size);
	if NOEXPECT (!memory) { MALLOC_FAILURE(memory, size); return NULL; }

	bool bres = dyrecord_add(record, memory, free);
	if NOEXPECT (!bres) { free(memory); return NULL; }

	return memory;
}

void* dyrecord_calloc(struct DyRecord_* restrict record, size_t count, size_t size)
{
	ASSUME(count != 0);
	ASSUME(size != 0);

	void* memory = calloc(count, size);
	if NOEXPECT (!memory) { CALLOC_FAILURE(memory, count, size); return NULL; }

	bool bres = dyrecord_add(record, memory, free);
	if NOEXPECT (!bres) { free(memory); return NULL; }

	return memory;
}

void dyrecord_free(struct DyRecord_* restrict record)
{
	struct Node* node = record->top;

	while (node) {
		struct Node* next = node->next;
		void* memory = node->memory;
		FreeCallback callback = node->callback;

		callback(memory);
		free(node);

		node = next;
	}

	record->count = 0;
}
