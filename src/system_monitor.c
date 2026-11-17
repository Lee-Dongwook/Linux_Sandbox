
#include "system_monitor.h"
#include "system_inspector.h"
#include <stdio.h>
#include <string.h>

int system_monitor_calculate(
    const SystemSnapshot *previous,
    const SystemSnapshot *current,
    SystemMetrics *metrics
) {
    if (current == NULL || metrics == NULL) {
        return -1;
    }

    memset(metrics, 0, sizeof(*metrics));

    if (previous != NULL &&
        (previous->valid_fields & SYSTEM_SNAPSHOT_CPU_COUNTERS) &&
        (current->valid_fields & SYSTEM_SNAPSHOT_CPU_COUNTERS) &&
        current->cpu_total_ticks > previous->cpu_total_ticks &&
        current->cpu_idle_ticks >= previous->cpu_idle_ticks) {

        unsigned long long total_delta =
            current->cpu_total_ticks - previous->cpu_total_ticks;

        unsigned long long idle_delta =
            current->cpu_idle_ticks - previous->cpu_idle_ticks;

        if (idle_delta <= total_delta && total_delta > 0) {
          metrics->cpu_usage =
              (double)(total_delta - idle_delta) / (double)total_delta * 100.0;

          metrics->valid_fields |= SYSTEM_METRICS_CPU_USAGE;
        }
    }

    if ((current->valid_fields & SYSTEM_SNAPSHOT_MEMORY_TOTAL) &&
        (current->valid_fields & SYSTEM_SNAPSHOT_MEMORY_AVAILABLE) &&
        current->memory_total_kb > 0 &&
        current->memory_available_kb <= current->memory_total_kb) {

      metrics->memory_used_kb =
          current->memory_total_kb - current->memory_available_kb;

      metrics->memory_available_kb = 
        current->memory_available_kb;

      metrics->memory_usage = (double)metrics->memory_used_kb /
                              (double)current->memory_total_kb * 100.0;

      metrics->valid_fields |= SYSTEM_METRICS_MEMORY_USAGE;
    }

    if (current->valid_fields & SYSTEM_SNAPSHOT_CPU_COUNT) {
        metrics->cpu_count = current->cpu_count;
        metrics->valid_fields |= SYSTEM_METRICS_CPU_COUNT;
    }

    if (current->valid_fields & SYSTEM_SNAPSHOT_LOAD_AVERAGE) {
        metrics->load_average_1 = current->load_average_1;
        metrics->load_average_5 = current->load_average_5;
        metrics->load_average_15 = current->load_average_15;

        metrics->valid_fields |= SYSTEM_METRICS_LOAD_AVERAGE;
    }

    if (current->valid_fields & SYSTEM_SNAPSHOT_UPTIME) {
        metrics->uptime_seconds = current->uptime_seconds;
        metrics->valid_fields |= SYSTEM_METRICS_UPTIME;
    }
    
    return 0;
}
