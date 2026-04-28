#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
    if (argc > 1)
        seed = (unsigned int)atoi(argv[1]);

    srand(seed);
    int attempts = 0;
    int found = 0;

    printf("[loteria:%d] Buscando 3 digitos iguales consecutivos...\n", getpid());
    fflush(stdout);

    while (!found && attempts < 100000) {
        int a = rand() % 10;
        int b = rand() % 10;
        int c = rand() % 10;
        attempts++;

        if (a == b && b == c) {
            printf("[loteria:%d] ENCONTRADO! %d%d%d en intento #%d\n",
                   getpid(), a, b, c, attempts);
            fflush(stdout);
            found = 1;
        } else if (attempts % 1000 == 0) {
            printf("[loteria:%d] Intento #%d... (%d%d%d)\n",
                   getpid(), attempts, a, b, c);
            fflush(stdout);
        }
    }

    if (!found)
        printf("[loteria:%d] No encontrado en %d intentos\n", getpid(), attempts);

    printf("[loteria:%d] Terminado!\n", getpid());
    return 0;
}
