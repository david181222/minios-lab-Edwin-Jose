#ifndef READY_QUEUE_H
#define READY_QUEUE_H

#include "pcb.h"

// Initialize the ready queue
void rq_init(void);

// Enqueue a process table index
int rq_enqueue(int idx);

// Dequeue the front process table index. Returns -1 if empty.
int rq_dequeue(void);

// Peek at front without removing. Returns -1 if empty.
int rq_peek(void);

// Current number of entries in the queue
int rq_size(void);

// Is the queue empty?
int rq_is_empty(void);

// Remove a specific index from the queue (for kill). Returns 0 if found, -1 if not.
int rq_remove(int idx);

// Print the ready queue contents
void rq_print(void);

#endif
