#include "monitor.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

static int sock_fd = -1;
static int enabled = 0;

static double get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

// Try to connect to the bridge server. Returns 0 on success, -1 on failure.
// socket() and connect() are async-signal-safe per POSIX, so this can be
// called from emit_line (which is invoked from SIGALRM handler).
static int try_connect(void) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, MONITOR_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    // Set non-blocking so writes never block the scheduler
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    sock_fd = fd;
    return 0;
}

static void emit_line(const char *json) {
    if (!enabled) return;

    // If not connected, try to connect now. The bridge may have started
    // after `log on`, or this may be the first event after enabling.
    if (sock_fd < 0) {
        if (try_connect() < 0) return; // Bridge not reachable, drop event
    }

    size_t len = strlen(json);
    ssize_t ret = write(sock_fd, json, len);

    if (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        // Connection broken (bridge died or socket closed).
        // Close and let next emit reconnect.
        close(sock_fd);
        sock_fd = -1;
    }
}

int monitor_init(const char *socket_path) {
    (void)socket_path;
    // No setup needed — connection happens lazily on first emit.
    return 0;
}

void monitor_close(void) {
    if (sock_fd >= 0) {
        close(sock_fd);
        sock_fd = -1;
    }
}

void monitor_set_enabled(int val) {
    enabled = val;

    if (enabled) {
        // Try to connect proactively so the user gets feedback
        if (sock_fd < 0) {
            if (try_connect() == 0) {
                printf("Monitor conectado al bridge en %s\n", MONITOR_SOCKET_PATH);
            } else {
                printf("Advertencia: bridge no esta corriendo.\n");
                printf("Levantalo con: cd bridge && node index.js\n");
                printf("Los eventos se enviaran automaticamente cuando el bridge arranque.\n");
            }
        }
    }
}

int monitor_is_enabled(void) {
    return enabled;
}

void monitor_emit_switch(pid_t from_pid, pid_t to_pid, int slice_ms) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"type\":\"CONTEXT_SWITCH\",\"ts\":%.3f,\"from\":%d,\"to\":%d,\"slice_ms\":%d}\n",
             get_timestamp(), from_pid, to_pid, slice_ms);
    emit_line(buf);
}

void monitor_emit_created(pid_t pid, const char *name) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"type\":\"PROCESS_CREATED\",\"ts\":%.3f,\"pid\":%d,\"name\":\"%s\"}\n",
             get_timestamp(), pid, name);
    emit_line(buf);
}

void monitor_emit_terminated(pid_t pid, double cpu_ms, int switches) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"type\":\"PROCESS_TERMINATED\",\"ts\":%.3f,\"pid\":%d,\"cpu_ms\":%.1f,\"switches\":%d}\n",
             get_timestamp(), pid, cpu_ms, switches);
    emit_line(buf);
}

void monitor_emit_registers(pid_t pid, unsigned long long pc, unsigned long long sp) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"type\":\"REGISTERS\",\"ts\":%.3f,\"pid\":%d,\"pc\":\"0x%llx\",\"sp\":\"0x%llx\"}\n",
             get_timestamp(), pid, pc, sp);
    emit_line(buf);
}

void monitor_emit_slice_changed(int old_ms, int new_ms) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"type\":\"SLICE_CHANGED\",\"ts\":%.3f,\"old_ms\":%d,\"new_ms\":%d}\n",
             get_timestamp(), old_ms, new_ms);
    emit_line(buf);
}
