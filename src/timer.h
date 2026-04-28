#ifndef TIMER_H
#define TIMER_H

// Initialize the timer with the given time slice in milliseconds.
// Installs the SIGALRM handler.
void timer_init(int slice_ms, void (*tick_handler)(int));

// Change the time slice (takes effect on next tick)
void timer_set_slice(int slice_ms);

// Get the current time slice in ms
int timer_get_slice(void);

// Stop the timer
void timer_stop(void);

// Start/restart the timer with the current slice
void timer_start(void);

#endif
