#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char *argv[]) {
    const char *sock_path = "/tmp/minios_pingpong.sock";
    if (argc > 1)
        sock_path = argv[1];

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    // Retry connection (server might not be ready yet)
    int connected = 0;
    for (int attempt = 0; attempt < 50; attempt++) {
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            connected = 1;
            break;
        }
        usleep(100000); // 100ms retry
    }

    if (!connected) {
        fprintf(stderr, "[pp_client:%d] No pude conectar a %s\n", getpid(), sock_path);
        return 1;
    }

    printf("[pp_client:%d] Conectado a %s\n", getpid(), sock_path);
    fflush(stdout);

    int counter = 1;
    for (int i = 0; i < 20; i++) {
        printf("[pp_client:%d] Enviando: %d\n", getpid(), counter);
        fflush(stdout);

        send(fd, &counter, sizeof(counter), 0);

        ssize_t n = recv(fd, &counter, sizeof(counter), 0);
        if (n <= 0) break;

        printf("[pp_client:%d] Respuesta: %d\n", getpid(), counter);
        fflush(stdout);

        counter++;
        usleep(200000);
    }

    close(fd);
    printf("[pp_client:%d] Terminado!\n", getpid());
    return 0;
}
