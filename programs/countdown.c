#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int n = 10;
    if (argc > 1)
        n = atoi(argv[1]);

    for (int i = n; i > 0; i--) {
        printf("[countdown:%d] %d\n", getpid(), i);
        fflush(stdout);
        sleep(1);
    }
    printf("[countdown:%d] Terminado!\n", getpid());
    return 0;
}
