
#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include "system_inspector.h"

typedef struct {
    double cpu_usage;
    double memory_usage;

    unsigned long memory_used_kb;
    unsigned long memory_available_kb;

    unsigned int cpu_count;

    double load_average_1;
    double load_average_5;
    double load_average_15;

    unsigned long uptime_seconds;
} SystemMetrics;

int system_monitor_calculate(
    const SystemSnapshot *previous,
    const SystemSnapshot *current,
    SystemMetrics *metrics
);

#endif
