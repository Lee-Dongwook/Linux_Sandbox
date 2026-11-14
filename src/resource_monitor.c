#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <unistd.h>

#include "resource_monitor.h"

int resource_monitor_take_cpu_sample(
    pid_t pid,
    CpuSample *sample
) {
    if (pid <= 0 || sample == NULL) {
        errno = EINVAL;
        return -1;
    }

    ProcessCpuTimes cpu_times;

    if (inspector_read_cpu_times(pid, &cpu_times) == -1) {
        return -1;
    }

    struct timespec timestamp;

    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) == -1) {
        return -1;
    }

    CpuSample result = {
        .pid = pid,
        .total_cpu_ticks =
            cpu_times.utime_ticks + cpu_times.stime_ticks,
        .timestamp = timestamp
    };

    *sample = result;

    return 0;
}

int resource_monitor_calculate_cpu_usage(
    const CpuSample *previous,
    const CpuSample *current,
    long clock_ticks,
    double *cpu_usage
) {
    if (previous == NULL || current == NULL ||
        cpu_usage == NULL || clock_ticks <= 0) {
        errno = EINVAL;
        return -1;
    }

    if (previous->pid != current->pid ||
        previous->total_cpu_ticks > current->total_cpu_ticks) {
        errno = EINVAL;
        return -1;
    }

    double elapsed_seconds =
        (double)(current->timestamp.tv_sec -
                 previous->timestamp.tv_sec) +
        (double)(current->timestamp.tv_nsec -
                 previous->timestamp.tv_nsec) / 1000000000.0;

    if (elapsed_seconds <= 0.0) {
        errno = EINVAL;
        return -1;
    }

    unsigned long long delta_ticks =
        current->total_cpu_ticks - previous->total_cpu_ticks;

    double usage =
        ((double)delta_ticks / (double)clock_ticks)
        / elapsed_seconds * 100.0;

    *cpu_usage = usage;

    return 0;
}
