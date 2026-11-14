
#include <stdio.h>

#include "system_inspector.h"

int main(void) {
    SystemSnapshot snapshot;

    if (system_inspector_collect(&snapshot) != 0) {
        fprintf(stderr, "System inspection failed\n");
        return 1;
    }

    printf("=== System Snapshot ===\n");

    printf("CPU Cores: %u\n", snapshot.cpu_count);
    printf("CPU Total Ticks: %llu\n", snapshot.cpu_total_ticks);
    printf("CPU Idle Ticks: %llu\n", snapshot.cpu_idle_ticks);

    printf(
        "Memory Total: %lu KB\n",
        snapshot.memory_total_kb
    );

    printf(
        "Memory Available: %lu KB\n",
        snapshot.memory_available_kb
    );

    printf(
        "Load Average: %.2f %.2f %.2f\n",
        snapshot.load_average_1,
        snapshot.load_average_5,
        snapshot.load_average_15
    );

    printf("Uptime: %lu seconds\n", snapshot.uptime_seconds);

    return 0;
}
