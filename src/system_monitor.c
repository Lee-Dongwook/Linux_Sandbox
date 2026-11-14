
#include "system_monitor.h"
#include <stdio.h>

int system_monitor_calculate(
    const SystemSnapshot *previous,
    const SystemSnapshot *current,
    SystemMetrics *metrics
) {
    if (current == NULL || metrics == NULL) {
        return -1;
    }

    metrics->cpu_usage = 0.0;

    if (previous != NULL &&
        current->cpu_total_ticks > previous->cpu_total_ticks &&
        current->cpu_idle_ticks >= previous->cpu_idle_ticks) {

        unsigned long long total_delta =
            current->cpu_total_ticks - previous->cpu_total_ticks;

        unsigned long long idle_delta =
            current->cpu_idle_ticks - previous->cpu_idle_ticks;

        if (idle_delta <= total_delta && total_delta > 0) {
            metrics->cpu_usage =
                (double)(total_delta - idle_delta) /
                (double)total_delta * 100.0;
        }
    }

    if (current->memory_total_kb == 0 ||
        current->memory_available_kb > current->memory_total_kb) {
        return -1;
    }

    metrics->memory_used_kb =
        current->memory_total_kb - current->memory_available_kb;

    metrics->memory_available_kb =
        current->memory_available_kb;

    metrics->memory_usage =
        (double)metrics->memory_used_kb /
        (double)current->memory_total_kb * 100.0;

    metrics->cpu_count = current->cpu_count;

    metrics->load_average_1 = current->load_average_1;
    metrics->load_average_5 = current->load_average_5;
    metrics->load_average_15 = current->load_average_15;

    metrics->uptime_seconds = current->uptime_seconds;

    return 0;
}
