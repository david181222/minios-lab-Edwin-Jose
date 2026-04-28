#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int total_mb = 10;
    if (argc > 1)
        total_mb = atoi(argv[1]);

    int chunk_kb = 256;
    int chunks = (total_mb * 1024) / chunk_kb;
    int allocated = 0;

    printf("[memory:%d] Pidiendo %d MB en bloques de %d KB\n",
           getpid(), total_mb, chunk_kb);
    fflush(stdout);

    for (int i = 0; i < chunks; i++) {
        void *p = malloc(chunk_kb * 1024);
        if (!p) {
            printf("[memory:%d] malloc fallo en bloque %d\n", getpid(), i);
            break;
        }
        memset(p, 0xAA, chunk_kb * 1024); // Touch every page
        allocated += chunk_kb;

        if ((i + 1) % 4 == 0) { // Report every 1 MB
            printf("[memory:%d] Acumulado: %d KB (%.1f MB)\n",
                   getpid(), allocated, allocated / 1024.0);
            fflush(stdout);
        }
        usleep(50000); // 50ms between allocations
    }

    printf("[memory:%d] Total: %d KB (%.1f MB). Terminado!\n",
           getpid(), allocated, allocated / 1024.0);
    return 0;
}
