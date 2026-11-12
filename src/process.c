#include <_time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include "monitor.h"
#include "process.h"
#include "resource.h"
#include "logger.h"

static int create_exec_error_pipe(int pipe_fd[2]) {
  if (pipe(pipe_fd) == -1) {
    return -1;
  }

  int flags = fcntl(pipe_fd[1], F_GETFD);

  if (flags == -1) {
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    return -1;
  }

  if (fcntl(pipe_fd[1], F_SETFD, flags | FD_CLOEXEC) == -1) {
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    return -1;
  }

  return 0;
}

static void report_exec_error(int error_fd, int error_number) {
  const char *data = (const char *)&error_number;
  size_t remaining = sizeof(error_number);

  while (remaining > 0) {
    ssize_t written = write(error_fd, data, remaining);

    if (written == -1) {
      if (errno == EINTR) {
        continue;
      }

      break;
    }

    data += written;
    remaining -= (size_t)written;
  }

  _exit(127);
}

static int read_exec_error(int error_fd, int *exec_error) {
  size_t received = 0;

  while (received < sizeof(*exec_error)) {
    ssize_t count = read(error_fd, (char *)exec_error + received,
                         sizeof(*exec_error) - received
    );

    if (count == 0) {
      break;
    }

    if (count == -1) {
      if (errno == EINTR) {
        continue;
      }

      return -1;
    }

    received += (size_t)count;
  }

  if (received != 0 && received != sizeof(*exec_error)) {
    errno = EIO;
    return -1;
  }

  return received == sizeof(*exec_error) ? 1 : 0;
}

int process_execute(char *const command_argv[], unsigned int cpu_limit_seconds,
                    unsigned int timeout_seconds) {

  if (command_argv == NULL || command_argv[0] == NULL) {
    fprintf(stderr, "Invalid command.\n");
    return -1;
  }

  struct timespec start_time;

  if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1) {
    perror("clock_gettime");
    return -1;
  }

  int exec_error_pipe[2];

  if (create_exec_error_pipe(exec_error_pipe) == -1) {
    perror("create_exec_error_pipe");
    return -1;
  }

  pid_t child_pid = fork();

  if (child_pid == -1) {
    close(exec_error_pipe[0]);
    close(exec_error_pipe[1]);
    perror("fork");
    return -1;
  }

  if (child_pid == 0) {

    close(exec_error_pipe[0]);

    if (setpgid(0, 0) == -1) {
      perror("setpgid");
      _exit(126);
    }

    if (cpu_limit_seconds > 0) {
        if (resource_apply_cpu_limit(cpu_limit_seconds) != 0) {
            perror("setrlimit(RLIMIT_CPU)");
            _exit(126);
        }
    }
    execvp(command_argv[0], command_argv);

    int exec_error = errno;
    report_exec_error(exec_error_pipe[1], exec_error);
  }

  close(exec_error_pipe[1]);

  int status;

  int monitor_result =
      monitor_wait_for_process(child_pid, timeout_seconds, &status);

  if (monitor_result == -1) {
    close(exec_error_pipe[0]);
    perror("monitor_wait_for_process");
    return -1;
  }

  int exec_error = 0;
  int exec_error_result = read_exec_error(exec_error_pipe[0], &exec_error);

  close(exec_error_pipe[0]);

  if (exec_error_result == -1) {
        perror("read_exec_error");
        return -1;
  }

  if (exec_error_result == 1) {
        errno = exec_error;
        perror("execvp");
  }

  int timed_out = (monitor_result == 1);
  int exit_code;

  if (timed_out) {
    fprintf(stderr, "Execution timed out.\n");
    exit_code = 124;
  } else if (WIFEXITED(status)) {
    exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    int signal_number = WTERMSIG(status);

    fprintf(stderr, "Child terminated by signal: %d\n", signal_number);

    exit_code = 128 + signal_number;
  } else {
    exit_code = 1;
  }

  struct timespec end_time;

  long duration_ms = 0;

  if (clock_gettime(CLOCK_MONOTONIC, &end_time) == 0) {
    duration_ms = (end_time.tv_sec - start_time.tv_sec) * 1000L +
                  (end_time.tv_nsec - start_time.tv_nsec) / 1000000L;

  } else {
    perror("clock_gettime");
  }

  if (logger_log_execution(command_argv, exit_code, timed_out, duration_ms) !=
      0) {
    perror("logger_log_execution");
  }

  return exit_code;
}

