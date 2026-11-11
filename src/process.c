#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "process.h"

int process_demo(void) {
  pid_t child_pid;

  printf("[Parent] Starting process demo.\n");
  printf("[Parent] PID: %d\n", (int)getpid());

  child_pid = fork();

  if (child_pid < 0) {
    perror("fork");
    return -1;
  }

  if (child_pid == 0) {
    printf("[Child] PID: %d\n", (int)getpid());
    printf("[Child] PPID: %d\n", (int)getppid());

    fflush(stdout);
    _exit(EXIT_SUCCESS);
  }

  printf("[Parent] Created child PID: %d\n", (int)child_pid);

  int status;
  pid_t result = waitpid(child_pid, &status, 0);

  if (result < 0) {
    perror("waitpid");
    return -1;
  }

  if (WIFEXITED(status)) {
    printf(
            "[Parent] Child exited with status: %d\n",
            WEXITSTATUS(status)
        );
  }

  printf("[Parent] Process demo completed.\n");

  return 0;
}
