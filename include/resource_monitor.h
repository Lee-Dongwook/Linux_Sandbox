#ifndef RESOURCE_MONITOR_H
#define RESOURCE_MONITOR_H

#include <time.h>

#include "inspector.h"

typedef struct {
    pid_t pid;
    unsigned long long total_cpu_ticks;
    struct timespec timestamp;
} CpuSample;

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

#endif
