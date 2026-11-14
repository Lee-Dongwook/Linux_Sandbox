#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "inspector.h"

int inspector_read_process(pid_t pid, ProcessSnapshot *snapshot) {
  if (pid <= 0 || snapshot == NULL) {
    errno = EINVAL;
    return -1;
  }

  char path[64];

  snprintf(path, sizeof(path), "/proc/%d/status", pid);

  FILE *file = fopen(path, "r");

  if (file == NULL) {
    return -1;
  }

  char line[256];

  ProcessSnapshot result = {0};
  result.pid = pid;

  while (fgets(line, sizeof(line), file) != NULL) {
    if (strncmp(line, "Name:", 5) == 0) {
        continue;
    }

    if (strncmp(line, "State:", 6) == 0) {
      char *value = strchr(line, ':');

      if (value != NULL) {
          value++;
        while (*value == ' ' || *value == '\t') {
          value++;
        }

        result.state = *value;
      }
    }

    else if (strncmp(line, "PPid:", 5) == 0) {
        sscanf(line + 5, "%d", &result.ppid);
    }

    else if (strncmp(line, "VmRSS:", 6) == 0) {
        sscanf(line + 6, "%lu", &result.vm_rss_kb);
    }

    else if (strncmp(line, "VmSize:", 7) == 0) {
        sscanf(line + 7, "%lu", &result.vm_size_kb);
    }

    else if (strncmp(line, "Threads:", 8) == 0) {
        sscanf(line + 8, "%u", &result.threads);
    }
  }

  if (ferror(file)) {
    int saved_errno = errno;
    fclose(file);
    errno = saved_errno;
    return -1;
  }

  fclose(file);

  if (result.state == '\0') {
    errno = EIO;
    return -1;
  }

  *snapshot = result;

  return 0;
}
