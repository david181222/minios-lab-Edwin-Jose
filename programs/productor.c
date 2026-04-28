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

    unlink(sock_path);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(server_fd, 1) < 0) { perror("listen"); return 1; }

    printf("[productor:%d] Esperando consumidor en %s\n", getpid(), sock_path);
    fflush(stdout);

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) { perror("accept"); return 1; }

    printf("[productor:%d] Consumidor conectado!\n", getpid());
    fflush(stdout);

    for (int i = 1; i <= 30; i++) {
        int data = i * i; // Produce squares
        send(client_fd, &data, sizeof(data), 0);
        printf("[productor:%d] Producido: %d^2 = %d\n", getpid(), i, data);
        fflush(stdout);
        usleep(150000);
    }

    // Send termination signal
    int end = -1;
    send(client_fd, &end, sizeof(end), 0);

    close(client_fd);
    close(server_fd);
    unlink(sock_path);
    printf("[productor:%d] Terminado!\n", getpid());
    return 0;
}
