#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    long limit = 10000;
    if (argc > 1)
        limit = atol(argv[1]);

    for (long n = 2; n <= limit; n++) {
        int is_prime = 1;
        for (long d = 2; d * d <= n; d++) {
            if (n % d == 0) { is_prime = 0; break; }
        }
        if (is_prime) {
            printf("[primos:%d] %ld\n", getpid(), n);
            fflush(stdout);
        }
    }
    printf("[primos:%d] Terminado!\n", getpid());
    return 0;
}
