#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE /* exposes SO_NOSIGPIPE under _POSIX_C_SOURCE */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <cjson/cJSON.h>

#include "ipc_server.h"
#include "system_inspector.h"
#include "system_monitor.h"

#define IPC_SOCKET_PATH "/tmp/mini-sandbox.sock"
#define IPC_MAX_MESSAGE_SIZE 4096
#define IPC_BACKLOG 5

static SystemSnapshot previous_snapshot;

/* A write to a disconnected client must fail with EPIPE, not kill the server.
 * Linux: per-send MSG_NOSIGNAL flag. macOS: per-socket SO_NOSIGPIPE option. */
#ifdef __APPLE__
#define IPC_SEND_FLAGS 0

static int ipc_disable_sigpipe(int fd) {
    int on = 1;
    return setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
}
#else
#define IPC_SEND_FLAGS MSG_NOSIGNAL

static int ipc_disable_sigpipe(int fd) {
    (void)fd;
    return 0;
}
#endif

static int ipc_send_all(int client_fd, const char *buffer, size_t length) {
  size_t sent = 0;

  while (sent < length) {
    ssize_t result = send(client_fd, buffer + sent, length - sent, IPC_SEND_FLAGS);

    if (result == -1) {
      if (errno == EINTR) {
          continue;
      }
      return -1;
    }

    if (result == 0) {
        return -1;
    }

    sent += (size_t)result;
  }

  return 0;
}

static int ipc_send_json(int client_fd, cJSON *response) {
    char *serialized = cJSON_PrintUnformatted(response);

    if (serialized == NULL) {
      return -1;
    }

    size_t length = strlen(serialized);

    int result = ipc_send_all(client_fd, serialized, length);

    if (result == 0) {
        result = ipc_send_all(client_fd, "\n", 1);
    }

    free(serialized);

    return result;
}

static cJSON *ipc_create_error(int request_id, const char *code, const char *message) {
  cJSON *response = cJSON_CreateObject();
  cJSON *error = cJSON_CreateObject();

  if (response == NULL || error == NULL) {
    cJSON_Delete(response);
    cJSON_Delete(error);
    return NULL;
  }

    cJSON_AddNumberToObject(response, "version", 1);
    cJSON_AddNumberToObject(response, "request_id", request_id);
    cJSON_AddStringToObject(response, "status", "error");

    cJSON_AddStringToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);

    cJSON_AddItemToObject(response, "error", error);

    return response;
}

static int ipc_read_request(int client_fd, char *buffer, size_t capacity) {
    size_t length = 0;

    while (length < capacity - 1) {
        char character;

        ssize_t received = recv(client_fd, &character, 1, 0);

        if (received == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (received == 0) {
            return 0;
        }
        if (character == '\n') {
            buffer[length] = '\0';
            return 1;
        }
        buffer[length++] = character;
    }

    return -2;
}

static cJSON *ipc_handle_system_metrics(int request_id) {
    SystemSnapshot current_snapshot;
    SystemMetrics metrics;

    if (system_inspector_collect(&current_snapshot) != 0) {
        return ipc_create_error(
            request_id,
            "INSPECTOR_ERROR",
            "Failed to collect system information"
        );
    }

    if (system_monitor_calculate(
            &previous_snapshot,
            &current_snapshot,
            &metrics
        ) != 0) {
        return ipc_create_error(
            request_id,
            "MONITOR_ERROR",
            "Failed to calculate system metrics"
        );
    }

    cJSON *response = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();

    if (response == NULL || data == NULL) {
        cJSON_Delete(response);
        cJSON_Delete(data);
        return NULL;
    }

    cJSON_AddNumberToObject(response, "version", 1);
    cJSON_AddNumberToObject(response, "request_id", request_id);
    cJSON_AddStringToObject(response, "status", "ok");

    cJSON_AddNumberToObject(data, "cpu_usage", metrics.cpu_usage);
    cJSON_AddNumberToObject(data, "memory_usage", metrics.memory_usage);

    cJSON_AddNumberToObject(
        data, "memory_used_kb", (double)metrics.memory_used_kb
    );

    cJSON_AddNumberToObject(
        data, "memory_available_kb", (double)metrics.memory_available_kb
    );

    cJSON_AddNumberToObject(data, "cpu_count", metrics.cpu_count);

    cJSON_AddNumberToObject(
        data, "load_average_1", metrics.load_average_1
    );

    cJSON_AddNumberToObject(
        data, "load_average_5", metrics.load_average_5
    );

    cJSON_AddNumberToObject(
        data, "load_average_15", metrics.load_average_15
    );

    cJSON_AddNumberToObject(
        data, "uptime_seconds", (double)metrics.uptime_seconds
    );

    cJSON_AddItemToObject(response, "data", data);

    previous_snapshot = current_snapshot;

    return response;
}

static cJSON *ipc_handle_request(const char *buffer) {
    cJSON *request = cJSON_Parse(buffer);

    if (request == NULL) {
        return ipc_create_error(
            0,
            "INVALID_JSON",
            "Request is not valid JSON"
        );
    }

    cJSON *version = cJSON_GetObjectItemCaseSensitive(
        request, "version"
    );

    cJSON *request_id = cJSON_GetObjectItemCaseSensitive(
        request, "request_id"
    );

    cJSON *command = cJSON_GetObjectItemCaseSensitive(
        request, "command"
    );

    int id = 0;

    if (cJSON_IsNumber(request_id) &&
        request_id->valuedouble >= 0 &&
        request_id->valuedouble <= INT_MAX &&
        request_id->valuedouble == (double)request_id->valueint) {
        id = request_id->valueint;
    } else {
        cJSON_Delete(request);

        return ipc_create_error(
            0,
            "INVALID_REQUEST",
            "request_id must be a non-negative integer"
        );
    }

    if (!cJSON_IsNumber(version) ||
        version->valuedouble != 1) {
        cJSON_Delete(request);

        return ipc_create_error(
            id,
            "UNSUPPORTED_VERSION",
            "Unsupported protocol version"
        );
    }

    if (!cJSON_IsString(command) ||
        command->valuestring == NULL) {
        cJSON_Delete(request);

        return ipc_create_error(
            id,
            "INVALID_REQUEST",
            "command must be a string"
        );
    }

    cJSON *response = NULL;

    if (strcmp(command->valuestring, "system.metrics") == 0) {
        response = ipc_handle_system_metrics(id);
    } else {
        response = ipc_create_error(
            id,
            "UNKNOWN_COMMAND",
            "Unsupported command"
        );
    }

    cJSON_Delete(request);

    return response;
}


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

    if (system_inspector_collect(&previous_snapshot) != 0) {
        fprintf(stderr, "Failed to initialize system snapshot\n");
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

      if (ipc_disable_sigpipe(client_fd) == -1) {
        perror("setsockopt(SO_NOSIGPIPE)");
        close(client_fd);
        continue;
      }

      char buffer[IPC_MAX_MESSAGE_SIZE + 1];

      int read_result = ipc_read_request(
          client_fd,
          buffer,
          sizeof(buffer)
      );

      if (read_result == 1) {
        cJSON *response = ipc_handle_request(buffer);

        if (response != NULL) {
          if (ipc_send_json(client_fd, response) != 0) {
                perror("send");
          }

          cJSON_Delete(response);
        } else {
          fprintf(stderr, "Failed to create IPC response\n");
        }
      } else if (read_result == -2) {
        cJSON *response = ipc_create_error(
            0, "MESSAGE_TOO_LARGE", "Request exceeds maximum message size");

        if (response != NULL) {
            ipc_send_json(client_fd, response);
            cJSON_Delete(response);
        }
      } else if (read_result == -1) {
          perror("recv");
      }

      close(client_fd);
    }

    close(server_fd);
    unlink(IPC_SOCKET_PATH);

    return 0;
}
