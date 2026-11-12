#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

typedef enum {
  PROCESS_TERMINATION_EXIT,
  PROCESS_TERMINATION_SIGNAL,
  PROCESS_TERMINATION_TIMEOUT,
  PROCESS_TERMINATION_EXEC_ERROR
} ProcessTerminationReason;

typedef struct {
  int exit_code;
  int signal_number;
  int exec_errno;
  ProcessTerminationReason termination_reason;
} ProcessResult;

int process_execute(char *const command_argv[], unsigned int cpu_limit_seconds,
                    unsigned int timeout_seconds,
                    unsigned long memory_limit_mb);

#endif
