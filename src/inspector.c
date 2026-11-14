#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <limits.h>

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

int inspector_collect_processes(
    ProcessSnapshot *snapshots,
    size_t capacity,
    size_t *count
) {
    if (snapshots == NULL || count == NULL || capacity == 0) {
        errno = EINVAL;
        return -1;
    }

    *count = 0;

    DIR *proc_dir = opendir("/proc");

    if (proc_dir == NULL) {
        return -1;
    }

    struct dirent *entry;

    while ((entry = readdir(proc_dir)) != NULL) {
        if (!isdigit((unsigned char)entry->d_name[0])) {
            continue;
        }

        char *end;
        long pid_value = strtol(entry->d_name, &end, 10);

        if (*end != '\0' || pid_value <= 0 || pid_value > INT_MAX) {
            continue;
        }

        if (*count >= capacity) {
            break;
        }

        ProcessSnapshot snapshot;

        if (inspector_read_process((pid_t)pid_value, &snapshot) == -1) {
            continue;
        }

        snapshots[*count] = snapshot;
        (*count)++;
    }

    int saved_errno = errno;
    closedir(proc_dir);

    errno = saved_errno;
    return 0;
}

int inspector_read_cpu_times(pid_t pid, ProcessCpuTimes *cpu_times) {
    if (pid <= 0 || cpu_times == NULL) {
        errno = EINVAL;
        return -1;
    }

    char path[64];

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return -1;
    }

    char line[4096];

    if (fgets(line, sizeof(line), file) == NULL) {
        int saved_errno = ferror(file) ? errno : EIO;
        fclose(file);
        errno = saved_errno;
        return -1;
    }

    fclose(file);

    /*
     * /proc/[pid]/stat:
     * field 1: pid
     * field 2: comm
     * field 3: state
     * ...
     * field 14: utime
     * field 15: stime
     *
     * comm may contain spaces and ')' characters.
     * Find the final ')' to locate the end of comm.
     */
    char *comm_end = strrchr(line, ')');

    if (comm_end == NULL || comm_end[1] != ' ' ||
        comm_end[2] == '\0') {
        errno = EIO;
        return -1;
    }

    char *cursor = comm_end + 3;

    unsigned long long utime_ticks = 0;
    unsigned long long stime_ticks = 0;

    for (int field = 4; field <= 15; field++) {
        while (*cursor == ' ') {
            cursor++;
        }

        if (*cursor == '\0' || *cursor == '\n') {
            errno = EIO;
            return -1;
        }

        errno = 0;

        char *end;
        long long value = strtoll(cursor, &end, 10);

        if (cursor == end || errno == ERANGE) {
            errno = EIO;
            return -1;
        }

        if (*end != ' ' && *end != '\n' && *end != '\0') {
            errno = EIO;
            return -1;
        }

        if (field == 14) {
            if (value < 0) {
                errno = EIO;
                return -1;
            }

            utime_ticks = (unsigned long long)value;
        } else if (field == 15) {
            if (value < 0) {
                errno = EIO;
                return -1;
            }

            stime_ticks = (unsigned long long)value;
        }

        cursor = end;
    }

    ProcessCpuTimes result = {
        .pid = pid,
        .utime_ticks = utime_ticks,
        .stime_ticks = stime_ticks
    };

    *cpu_times = result;

    return 0;
}
