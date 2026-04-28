#ifndef MONITOR_H
#define MONITOR_H

#include <sys/types.h>

#define MONITOR_SOCKET_PATH "/tmp/minios.sock"

// Initialize the monitor (no-op for sockets, connection happens on demand)
int monitor_init(const char *socket_path);

// Close the monitor connection
void monitor_close(void);

// Enable/disable event emission
void monitor_set_enabled(int enabled);
int  monitor_is_enabled(void);

// Emit events (no-op if disabled or not connected)
void monitor_emit_switch(pid_t from_pid, pid_t to_pid, int slice_ms);
void monitor_emit_created(pid_t pid, const char *name);
void monitor_emit_terminated(pid_t pid, double cpu_ms, int switches);
void monitor_emit_registers(pid_t pid, unsigned long long pc, unsigned long long sp);
void monitor_emit_slice_changed(int old_ms, int new_ms);

#endif
