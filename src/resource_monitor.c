#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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

int resource_monitor_collect(
    pid_t pid,
    ResourceSnapshot *snapshot
) {
    if (pid <= 0 || snapshot == NULL) {
        errno = EINVAL;
        return -1;
    }

    ProcessSnapshot process_snapshot;
    ProcessCpuTimes cpu_times;

    if (inspector_read_process(pid, &process_snapshot) == -1) {
        return -1;
    }

    if (inspector_read_cpu_times(pid, &cpu_times) == -1) {
        return -1;
    }

    struct timespec timestamp;

    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) == -1) {
      return -1;
    }

    ResourceSnapshot result = {
        .pid = process_snapshot.pid,
        .state = process_snapshot.state,
        .vm_rss_kb = process_snapshot.vm_rss_kb,
        .vm_size_kb = process_snapshot.vm_size_kb,
        .threads = process_snapshot.threads,
        .cpu_ticks = cpu_times.utime_ticks + cpu_times.stime_ticks,
        .cpu_usage = 0.0,
        .timestamp = timestamp
    };

    *snapshot = result;

    return 0;
}

int resource_monitor_collect_all(
    ResourceSnapshot *snapshots,
    size_t capacity,
    size_t *count
) {
    if (snapshots == NULL || count == NULL || capacity == 0) {
        errno = EINVAL;
        return -1;
    }

    *count = 0;

    ProcessSnapshot *processes = calloc(
        capacity,
        sizeof(ProcessSnapshot)
    );

    if (processes == NULL) {
        return -1;
    }

    size_t process_count = 0;

    if (inspector_collect_processes(
            processes,
            capacity,
            &process_count
        ) == -1) {
        free(processes);
        return -1;
    }

    for (size_t i = 0; i < process_count; i++) {
        ResourceSnapshot snapshot;

        if (resource_monitor_collect(
                processes[i].pid,
                &snapshot
            ) == -1) {
            continue;
        }

        snapshots[*count] = snapshot;
        (*count)++;
    }

    free(processes);

    return 0;
}

int resource_monitor_update_cpu_usage(
    const ResourceSnapshot *previous,
    size_t previous_count,
    ResourceSnapshot *current,
    size_t current_count,
    long clock_ticks
) {
    if ((previous_count > 0 && previous == NULL) ||
        (current_count > 0 && current == NULL) ||
        clock_ticks <= 0) {
        errno = EINVAL;
        return -1;
    }

    for (size_t i = 0; i < current_count; i++) {
        current[i].cpu_usage = 0.0;

        for (size_t j = 0; j < previous_count; j++) {
            if (current[i].pid != previous[j].pid) {
                continue;
            }

            CpuSample previous_sample = {
                .pid = previous[j].pid,
                .total_cpu_ticks = previous[j].cpu_ticks,
                .timestamp = previous[j].timestamp
            };

            CpuSample current_sample = {
                .pid = current[i].pid,
                .total_cpu_ticks = current[i].cpu_ticks,
                .timestamp = current[i].timestamp
            };

            double cpu_usage;

            if (resource_monitor_calculate_cpu_usage(
                    &previous_sample,
                    &current_sample,
                    clock_ticks,
                    &cpu_usage
                ) == -1) {
                return -1;
            }

            current[i].cpu_usage = cpu_usage;
            break;
        }
    }

    return 0;
}
