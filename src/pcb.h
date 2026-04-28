#ifndef PCB_H
#define PCB_H

#include <sys/types.h>
#include <time.h>
#include "platform/platform.h"

#define MAX_PROCESSES 10
#define MAX_NAME_LEN  64

typedef enum {
    PROC_NEW,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED
} proc_state_t;

typedef struct {
    pid_t             pid;
    char              name[MAX_NAME_LEN];
    proc_state_t      state;
    cpu_context_t     registers;
    int               regs_valid;       // 1 if registers were captured successfully
    struct timespec   created_at;
    struct timespec   last_started;
    double            cpu_time_ms;
    double            wait_time_ms;
    int               context_switches;
} pcb_t;

// Global process table
extern pcb_t process_table[MAX_PROCESSES];
extern int   process_count;

// Initialize a PCB entry
void pcb_init(pcb_t *pcb, pid_t pid, const char *name);

// Return a string representation of the state
const char *pcb_state_name(proc_state_t state);

// Print a single PCB
void pcb_print(const pcb_t *pcb);

// Print the full process table
void pcb_print_table(void);

#endif
