#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pcb.h"
#include "ready_queue.h"

// Initialize the scheduler
void scheduler_init(void);

// Create a new process from a binary path with optional argument. Returns process table index, or -1 on error.
int scheduler_create_process(const char *path, const char *arg);

// Get the index of the currently running process (-1 if none)
int scheduler_get_running(void);

// Start scheduling (dequeues first process, starts timer)
void scheduler_start(int slice_ms);

// Stop scheduling (stops timer, kills all processes)
void scheduler_stop(void);

// SIGALRM handler — performs the context switch
void scheduler_tick(int signum);

// SIGCHLD handler — detects terminated child processes
void scheduler_sigchld(int signum);

// Install the SIGCHLD handler
void scheduler_install_sigchld(void);

// Returns 1 if scheduler is actively running
int scheduler_is_running(void);

// Compute elapsed ms between two timespecs
double timespec_diff_ms(struct timespec end, struct timespec start);

#endif
