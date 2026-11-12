#include <_time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "monitor.h"
#include "process.h"
#include "resource.h"
#include "logger.h"

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
  
  pid_t child_pid = fork();

  if (child_pid < 0) {
    perror("fork");
    return -1;
  }

  if (child_pid == 0) {

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

    perror("execvp");
    _exit(127);
  }

  int status;

  int monitor_result =
      monitor_wait_for_process(child_pid, timeout_seconds, &status);

  if (monitor_result == -1) {
    perror("monitor_wait_for_process");
    return -1;
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
