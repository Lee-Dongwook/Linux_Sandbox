#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "ipc_server.h"

#define IPC_SOCKET_PATH "/tmp/mini-sandbox.sock"

int ipc_server_run(void) {
    int server_fd;
    struct sockaddr_un address;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    strncpy(address.sun_path, IPC_SOCKET_PATH,
            sizeof(address.sun_path) - 1
    );

    unlink(IPC_SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) == -1) {
      perror("listen");
      close(server_fd);
      unlink(IPC_SOCKET_PATH);
      return -1;
    }

    printf("IPC Server listening on %s\n", IPC_SOCKET_PATH);

    while (1) {
      int client_fd = accept(server_fd, NULL, NULL);

      if (client_fd == -1) {
        if (errno == EINTR) {
            continue;
        }

        perror("accept");
        break;
      }

      printf("Client connected\n");

      close(client_fd);
    }

    close(server_fd);
    unlink(IPC_SOCKET_PATH);

    return 0;
}
