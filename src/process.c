#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "monitor.h"
#include "process.h"
#include "resource.h"

int process_execute(char *const command_argv[], unsigned int cpu_limit_seconds,
                    unsigned int timeout_seconds) {

  if (command_argv == NULL || command_argv[0] == NULL) {
    fprintf(stderr, "Invalid command.\n");
    return -1;
  }
  
  pid_t child_pid = fork();

  if (child_pid < 0) {
    perror("fork");
    return -1;
  }

  if (child_pid == 0) {

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

  if (monitor_result == 1) {
    fprintf(stderr, "Execution timed out.\n");
    return 124;
  }

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  if (WIFSIGNALED(status)) {
    int signal_number = WTERMSIG(status);

    fprintf(stderr, "Child terminated by signal: %d\n", signal_number);

    return 128 + signal_number;
  }

  return -1;
}
