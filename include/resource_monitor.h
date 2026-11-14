#ifndef RESOURCE_MONITOR_H
#define RESOURCE_MONITOR_H

#include <time.h>
#include <stddef.h>

#include "inspector.h"

typedef struct {
    pid_t pid;
    unsigned long long total_cpu_ticks;
    struct timespec timestamp;
} CpuSample;

typedef struct {
    pid_t pid;
    char state;
    unsigned long vm_rss_kb;
    unsigned long vm_size_kb;
    unsigned int threads;
    unsigned long long cpu_ticks;
    double cpu_usage;
} ResourceSnapshot;

int resource_monitor_take_cpu_sample(
    pid_t pid,
    CpuSample *sample
);

int resource_monitor_calculate_cpu_usage(
    const CpuSample *previous,
    const CpuSample *current,
    long clock_ticks,
    double *cpu_usage
);

int resource_monitor_collect(pid_t pid, ResourceSnapshot *snapshot);

int resource_monitor_collect_all(
    ResourceSnapshot *snapshots,
    size_t capacity,
    size_t *count
);

#endif
