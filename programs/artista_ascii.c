#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int size = 15;
    if (argc > 1)
        size = atoi(argv[1]);

    printf("[artista:%d] Dibujando patron de %d lineas\n", getpid(), size);
    fflush(stdout);

    // Diamond pattern
    for (int i = 1; i <= size; i++) {
        printf("[artista:%d] ", getpid());
        int width = (i <= (size + 1) / 2) ? i : size + 1 - i;
        for (int s = 0; s < (size + 1) / 2 - width; s++) printf(" ");
        for (int j = 0; j < 2 * width - 1; j++) printf("*");
        printf("\n");
        fflush(stdout);
        usleep(200000); // 200ms between lines
    }
    printf("[artista:%d] Terminado!\n", getpid());
    return 0;
}
