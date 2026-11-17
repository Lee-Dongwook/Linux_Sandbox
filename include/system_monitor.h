
#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include "system_inspector.h"

typedef enum {
    SYSTEM_METRICS_CPU_USAGE = 1U << 0,
    SYSTEM_METRICS_MEMORY_USAGE = 1U << 1,
    SYSTEM_METRICS_CPU_COUNT = 1U << 2,
    SYSTEM_METRICS_LOAD_AVERAGE = 1U << 3,
    SYSTEM_METRICS_UPTIME = 1U << 4
} SystemMetricsField;

typedef struct {
    unsigned int valid_fields;
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
