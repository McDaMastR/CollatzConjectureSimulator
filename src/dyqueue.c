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

#include "dyqueue.h"
#include "debug.h"


typedef struct DyQueue_T
{
	size_t size;  // # bytes per element
	size_t count; // # elements currently in queue
	void** head;  // Head node
	void** tail;  // Tail node
} DyQueue_T;


void dyqueue_destroy(restrict DyQueue queue)
{
	if EXPECT_FALSE (!queue) return;

	void** node = queue->head;
	void*  next = NULL;

	while (node) {
		next = *node;
		free(node);
		node = (void**) next;
	}

	free(queue);
}

DyQueue dyqueue_create(size_t size)
{
	ASSUME(size != 0);

	DyQueue queue = (DyQueue) malloc(sizeof(DyQueue_T));

	if EXPECT_FALSE (!queue) { MALLOC_FAILURE(queue, sizeof(DyQueue_T)); return NULL; }

	queue->size  = size;
	queue->count = 0;
	queue->head  = NULL;
	queue->tail  = NULL;

	return queue;
}

size_t dyqueue_size(restrict DyQueue queue)
{
	return EXPECT_TRUE (queue) ? queue->count : 0;
}

void* dyqueue_add(restrict DyQueue queue, const void* restrict value)
{
	ASSUME(queue->size != 0);

	size_t size  = queue->size;
	size_t count = queue->count;
	void** tail  = queue->tail;

	void** node = (void**) malloc(sizeof(void*) + size);

	if EXPECT_FALSE (!node) { MALLOC_FAILURE(node, sizeof(void*) + size); return NULL; }

	*node = NULL;

	memcpy((char*) node + sizeof(void*), value, size);

	if (count) { *tail       = node; }
	else       { queue->head = node; }

	queue->count = count + 1;
	queue->tail  = node;

	return (char*) node + sizeof(void*);
}

void dyqueue_pop(restrict DyQueue queue, void* restrict value)
{
	ASSUME(queue->size != 0);
	ASSUME(queue->count != 0);

	size_t size  = queue->size;
	size_t count = queue->count;
	void** head  = queue->head;

	void** node = (void**) *head;

	memcpy(value, (char*) head + sizeof(void*), size);

	free(head);

	if (count == 1) { queue->tail = node; }

	queue->count = count - 1;
	queue->head  = node;
}
