#ifndef MONITOR_H
#define MONITOR_H

#include <sys/types.h>

int monitor_wait_for_process(pid_t pid, unsigned int timeout_seconds,
                             int *status);

#endif
