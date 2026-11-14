#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system_inspector.h"

static int read_cpu_info(SystemSnapshot *snapshot) {
  FILE *file = fopen("/proc/stat", "r");

  if (file == NULL) {
    return -1;
  }

  char line[512];

  if (fgets(line, sizeof(line), file) == NULL) {
    fclose(file);
    return -1;
  }

  fclose(file);

    unsigned long long user = 0;
    unsigned long long nice = 0;
    unsigned long long system = 0;
    unsigned long long idle = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;

    int parsed =
        sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user,
               &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    if (parsed < 4) {
        return -1;
    }

    snapshot->cpu_total_ticks =
        user + nice + system + idle + iowait + irq + softirq + steal;

    snapshot->cpu_idle_ticks = idle + iowait;

    file = fopen("/proc/stat", "r");

    if (file == NULL) {
        return -1;
    }

    snapshot->cpu_count = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
      unsigned int cpu_number;

      if (sscanf(line, "cpu%u", &cpu_number) == 1) {
        snapshot->cpu_count++;
      }
    }

    fclose(file);

    return 0;
}

static int read_memory_info(SystemSnapshot *snapshot) {
    FILE *file = fopen("/proc/meminfo", "r");

    if (file == NULL) {
        return -1;
    }

    char line[256];

    int found_total = 0;
    int found_available = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(
                line,
                "MemTotal: %lu kB",
                &snapshot->memory_total_kb
            ) == 1) {
            found_total = 1;
        }

        if (sscanf(
                line,
                "MemAvailable: %lu kB",
                &snapshot->memory_available_kb
            ) == 1) {
            found_available = 1;
        }

        if (found_total && found_available) {
            break;
        }
    }

    fclose(file);

    if (!found_total || !found_available) {
        return -1;
    }

    return 0;
}

static int read_load_average(SystemSnapshot *snapshot) {
    FILE *file = fopen("/proc/loadavg", "r");

    if (file == NULL) {
        return -1;
    }

    int parsed = fscanf(
        file,
        "%lf %lf %lf",
        &snapshot->load_average_1,
        &snapshot->load_average_5,
        &snapshot->load_average_15
    );

    fclose(file);

    return parsed == 3 ? 0 : -1;
}

static int read_uptime(SystemSnapshot *snapshot) {
    FILE *file = fopen("/proc/uptime", "r");

    if (file == NULL) {
        return -1;
    }

    double uptime = 0.0;

    int parsed = fscanf(file, "%lf", &uptime);

    fclose(file);

    if (parsed != 1 || uptime < 0.0) {
        return -1;
    }

    snapshot->uptime_seconds = (unsigned long)uptime;

    return 0;
}

int system_inspector_collect(SystemSnapshot *snapshot) {
    if (snapshot == NULL) {
        return -1;
    }

    memset(snapshot, 0, sizeof(*snapshot));

    if (read_cpu_info(snapshot) != 0) {
        return -1;
    }

    if (read_memory_info(snapshot) != 0) {
        return -1;
    }

    if (read_load_average(snapshot) != 0) {
        return -1;
    }

    if (read_uptime(snapshot) != 0) {
        return -1;
    }

    return 0;
}
