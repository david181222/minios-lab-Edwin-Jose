#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// CPU-bound program with NO kernel syscalls in the hot loop.
//
// Argument: approximate duration in seconds (default 20).
//
// The inner loop is pure arithmetic (no I/O, no sleep). The deadline check
// uses clock_gettime(CLOCK_MONOTONIC) which on Linux x86_64 is served by
// the vDSO — it runs entirely in user space without entering the kernel.
// Check frequency is tuned so <0.01% of PTRACE_GETREGS samples land in
// the vDSO; the rest land in this binary's own text segment, making the
// PC vary visibly in the dashboard on each context switch.
int main(int argc, char *argv[]) {
    double seconds = 20.0;
    if (argc > 1) seconds = atof(argv[1]);

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("[busy:%d] Corriendo ~%.1f s de compute puro\n", getpid(), seconds);
    fflush(stdout);

    // ~4M iterations between deadline checks (~1-4 ms on modern CPUs)
    const unsigned long CHECK_EVERY = 1UL << 22;

    volatile unsigned long sum = 0;
    unsigned long total_iters = 0;

    for (;;) {
        for (unsigned long k = 0; k < CHECK_EVERY; k++) {
            sum += total_iters * 7u + 3u;
            sum ^= (sum << 13);
            sum ^= (sum >> 7);
            total_iters++;
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (double)(now.tv_sec - start.tv_sec)
                       + (double)(now.tv_nsec - start.tv_nsec) / 1e9;
        if (elapsed >= seconds) break;
    }

    printf("[busy:%d] sum=%lu iter=%lu  Terminado!\n",
           getpid(), sum, total_iters);
    return 0;
}
