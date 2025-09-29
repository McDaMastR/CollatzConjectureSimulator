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


struct DyQueue_
{
	size_t size; // Number of bytes per element
	size_t count; // Number of elements currently in queue
	void** head; // Head node
	void** tail; // Tail node
};


void dyqueue_destroy(struct DyQueue_* restrict queue)
{
	if NOEXPECT (!queue) { return; }

	void** node = queue->head;
	void* next = NULL;

	while (node) {
		next = *node;
		czFree(node);
		node = next;
	}

	czFree(queue);
}

struct DyQueue_* dyqueue_create(size_t size)
{
	ASSUME(size != 0);

	struct DyQueue_* restrict queue;
	struct CzAllocFlags flags = {0};

	enum CzResult czres = czAlloc((void* restrict*) &queue, sizeof(*queue), flags);
	if NOEXPECT (czres) { return NULL; }

	queue->size = size;
	queue->count = 0;
	queue->head = NULL;
	queue->tail = NULL;

	return queue;
}

size_t dyqueue_size(struct DyQueue_* restrict queue)
{
	return queue->count;
}

bool dyqueue_enqueue(struct DyQueue_* restrict queue, const void* restrict value)
{
	ASSUME(queue->size != 0);

	size_t size = queue->size;
	size_t count = queue->count;
	void** tail = queue->tail;

	void** restrict node;
	struct CzAllocFlags flags = {0};

	enum CzResult czres = czAlloc((void* restrict*) &node, sizeof(void*) + size, flags);
	if NOEXPECT (czres) { return false; }

	*node = NULL;

	void* element = (char*) node + sizeof(void*);
	memcpy(element, value, size);

	if (count) {
		*tail = node;
	}
	else {
		queue->head = node;
	}

	queue->count = count + 1;
	queue->tail = node;

	return true;
}

void dyqueue_dequeue(struct DyQueue_* restrict queue, void* restrict value)
{
	ASSUME(queue->size != 0);
	ASSUME(queue->count != 0);
	ASSUME(queue->head != NULL);
	ASSUME(queue->tail != NULL);

	size_t size = queue->size;
	size_t count = queue->count;
	void** head = queue->head;

	void** node = *head;
	void* element = (char*) head + sizeof(void*);

	memcpy(value, element, size);
	czFree(head);

	if (count == 1) {
		queue->tail = node;
	}

	queue->count = count - 1;
	queue->head = node;
}
