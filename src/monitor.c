#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>

#include "monitor.h"

#define MONITOR_INTERVAL_NS 10000000L

static int has_timed_out(const struct timespec *start,
                         const struct timespec *now,
                         unsigned int timeout_seconds) {
  time_t elapsed_seconds = now->tv_sec - start->tv_sec;

  if (now->tv_nsec < start->tv_nsec) {
    elapsed_seconds--;
  }

  return elapsed_seconds >= (time_t)timeout_seconds;
}

int monitor_wait_for_process(pid_t pid, unsigned int timeout_seconds,
                             int *status) {
  struct timespec start;
  struct timespec now;

  if (status == NULL) {
    return -1;
  }

  if (timeout_seconds > 0) {
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        return -1;
    }
  }

  while (1) {
    pid_t result = waitpid(pid, status, WNOHANG);

    if (result == pid) {
        return 0;
    }

    if (result == -1) {
      if (errno == EINTR) {
          continue;
      }

      return -1;
    }

    if (timeout_seconds > 0) {
      if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
                return -1;
      }

      if (has_timed_out(&start, &now, timeout_seconds)) {
        if (kill(pid, SIGKILL) == -1 && errno != ESRCH) {
          return -1;
        }

        while (waitpid(pid, status, 0) == -1) {
          if (errno != EINTR) {
            return -1;
          }
        }

        return 1;
      }
    }

    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = MONITOR_INTERVAL_NS
    };

    while (nanosleep(&interval, &interval) == -1) {
      if (errno != EINTR) {
        return -1;
      }
    }
  }
}
