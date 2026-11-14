
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>

#include "system_inspector.h"
#include "system_monitor.h"

int main(void) {
    SystemSnapshot previous;
    SystemSnapshot current;
    SystemMetrics metrics;

    if (system_inspector_collect(&previous) != 0) {
        fprintf(stderr, "First collection failed\n");
        return 1;
    }

    sleep(1);

    if (system_inspector_collect(&current) != 0) {
        fprintf(stderr, "Second collection failed\n");
        return 1;
    }

    if (system_monitor_calculate(
            &previous,
            &current,
            &metrics
        ) != 0) {
        fprintf(stderr, "Metric calculation failed\n");
        return 1;
    }

    printf("=== System Metrics ===\n");
    printf("CPU Usage: %.2f%%\n", metrics.cpu_usage);
    printf("CPU Cores: %u\n", metrics.cpu_count);

    printf(
        "Memory Usage: %.2f%% (%lu / %lu KB)\n",
        metrics.memory_usage,
        metrics.memory_used_kb,
        current.memory_total_kb
    );

    printf(
        "Memory Available: %lu KB\n",
        metrics.memory_available_kb
    );

    printf(
        "Load Average: %.2f %.2f %.2f\n",
        metrics.load_average_1,
        metrics.load_average_5,
        metrics.load_average_15
    );

    printf("Uptime: %lu seconds\n", metrics.uptime_seconds);

    return 0;
}
