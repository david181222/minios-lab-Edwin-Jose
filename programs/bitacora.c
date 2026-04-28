#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int interval_ms = 500;
    if (argc > 1)
        interval_ms = atoi(argv[1]);

    char filename[64];
    snprintf(filename, sizeof(filename), "bitacora_%d.log", getpid());

    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("fopen");
        return 1;
    }

    printf("[bitacora:%d] Escribiendo a %s cada %d ms\n", getpid(), filename, interval_ms);
    fflush(stdout);

    for (int entry = 1; entry <= 50; entry++) {
        time_t now = time(NULL);
        struct tm *tm = localtime(&now);
        char ts[32];
        strftime(ts, sizeof(ts), "%H:%M:%S", tm);

        fprintf(f, "[%s] Entrada #%d — PID %d\n", ts, entry, getpid());
        fflush(f);

        printf("[bitacora:%d] Entrada #%d escrita\n", getpid(), entry);
        fflush(stdout);

        usleep(interval_ms * 1000);
    }

    fclose(f);
    printf("[bitacora:%d] Terminado! Log en %s\n", getpid(), filename);
    return 0;
}
