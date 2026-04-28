#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char *argv[]) {
    const char *sock_path = "/tmp/minios_prodcons.sock";
    if (argc > 1)
        sock_path = argv[1];

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    // Retry connection
    int connected = 0;
    for (int attempt = 0; attempt < 50; attempt++) {
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            connected = 1;
            break;
        }
        usleep(100000);
    }

    if (!connected) {
        fprintf(stderr, "[consumidor:%d] No pude conectar a %s\n", getpid(), sock_path);
        return 1;
    }

    printf("[consumidor:%d] Conectado al productor\n", getpid());
    fflush(stdout);

    int total = 0, count = 0;
    while (1) {
        int data;
        ssize_t n = recv(fd, &data, sizeof(data), 0);
        if (n <= 0 || data == -1) break;

        count++;
        total += data;
        printf("[consumidor:%d] Consumido: %d (total=%d, n=%d)\n",
               getpid(), data, total, count);
        fflush(stdout);
        usleep(100000);
    }

    close(fd);
    printf("[consumidor:%d] Procesados %d items, suma=%d. Terminado!\n",
           getpid(), count, total);
    return 0;
}
