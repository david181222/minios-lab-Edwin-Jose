#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static long long fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char *argv[]) {
    int n = 45;
    if (argc > 1)
        n = atoi(argv[1]);

    for (int i = 1; i <= n; i++) {
        long long result = fib(i);
        printf("[fibonacci:%d] fib(%d) = %lld\n", getpid(), i, result);
        fflush(stdout);
    }
    printf("[fibonacci:%d] Terminado!\n", getpid());
    return 0;
}
