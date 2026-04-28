#include "pcb.h"
#include <stdio.h>
#include <string.h>

pcb_t process_table[MAX_PROCESSES];
int   process_count = 0;

void pcb_init(pcb_t *pcb, pid_t pid, const char *name) {
    memset(pcb, 0, sizeof(*pcb));
    pcb->pid = pid;
    strncpy(pcb->name, name, MAX_NAME_LEN - 1);
    pcb->name[MAX_NAME_LEN - 1] = '\0';
    pcb->state = PROC_NEW;
    pcb->regs_valid = 0;
    pcb->cpu_time_ms = 0.0;
    pcb->wait_time_ms = 0.0;
    pcb->context_switches = 0;
    clock_gettime(CLOCK_MONOTONIC, &pcb->created_at);
}

// ANSI color codes
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_RED     "\033[31m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_GRAY    "\033[90m"
#define ANSI_RESET   "\033[0m"

const char *pcb_state_name(proc_state_t state) {
    switch (state) {
        case PROC_NEW:        return ANSI_BLUE   "NEW"        ANSI_RESET;
        case PROC_READY:      return ANSI_YELLOW "READY"      ANSI_RESET;
        case PROC_RUNNING:    return ANSI_GREEN  "RUNNING"    ANSI_RESET;
        case PROC_BLOCKED:    return ANSI_RED    "BLOCKED"    ANSI_RESET;
        case PROC_TERMINATED: return ANSI_GRAY   "TERMINATED" ANSI_RESET;
        default:              return "UNKNOWN";
    }
}

void pcb_print(const pcb_t *pcb) {
    printf("  PID: %-6d  Nombre: %-16s  Estado: %-10s  CPU: %8.1f ms  Espera: %8.1f ms  Switches: %d\n",
           pcb->pid, pcb->name, pcb_state_name(pcb->state),
           pcb->cpu_time_ms, pcb->wait_time_ms, pcb->context_switches);
}

void pcb_print_table(void) {
    printf("%-6s %-16s %-12s %10s %10s %9s\n",
           "PID", "NOMBRE", "ESTADO", "CPU(ms)", "ESPERA(ms)", "SWITCHES");
    printf("─────────────────────────────────────────────────────────────────────\n");
    for (int i = 0; i < process_count; i++) {
        printf("%-6d %-16s %-12s %10.1f %10.1f %9d\n",
               process_table[i].pid,
               process_table[i].name,
               pcb_state_name(process_table[i].state),
               process_table[i].cpu_time_ms,
               process_table[i].wait_time_ms,
               process_table[i].context_switches);
    }
}
