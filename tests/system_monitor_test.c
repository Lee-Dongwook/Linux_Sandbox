
#include <assert.h>
#include <stdio.h>

#include "system_inspector.h"
#include "system_monitor.h"

static void test_cpu_unsupported(void) {
    SystemSnapshot previous = {0};
    SystemSnapshot current = {0};
    SystemMetrics metrics = {0};

    previous.valid_fields =
        SYSTEM_SNAPSHOT_MEMORY_TOTAL |
        SYSTEM_SNAPSHOT_MEMORY_AVAILABLE;

    current.valid_fields =
        SYSTEM_SNAPSHOT_MEMORY_TOTAL |
        SYSTEM_SNAPSHOT_MEMORY_AVAILABLE;

    current.memory_total_kb = 8000;
    current.memory_available_kb = 3000;

    assert(system_monitor_calculate(
        &previous, &current, &metrics
    ) == 0);

    assert(!(metrics.valid_fields & SYSTEM_METRICS_CPU_USAGE));
    assert(metrics.valid_fields & SYSTEM_METRICS_MEMORY_USAGE);

    printf("[PASS] CPU unsupported\n");
}

static void test_memory_unsupported(void) {
    SystemSnapshot previous = {0};
    SystemSnapshot current = {0};
    SystemMetrics metrics = {0};

    previous.valid_fields = SYSTEM_SNAPSHOT_CPU_COUNTERS;
    current.valid_fields = SYSTEM_SNAPSHOT_CPU_COUNTERS;

    previous.cpu_total_ticks = 1000;
    previous.cpu_idle_ticks = 600;

    current.cpu_total_ticks = 1200;
    current.cpu_idle_ticks = 700;

    assert(system_monitor_calculate(
        &previous, &current, &metrics
    ) == 0);

    assert(metrics.valid_fields & SYSTEM_METRICS_CPU_USAGE);
    assert(!(metrics.valid_fields & SYSTEM_METRICS_MEMORY_USAGE));

    assert(metrics.cpu_usage == 50.0);

    printf("[PASS] Memory unsupported\n");
}

static void test_partial_metrics(void) {
    SystemSnapshot previous = {0};
    SystemSnapshot current = {0};
    SystemMetrics metrics = {0};

    previous.valid_fields = SYSTEM_SNAPSHOT_CPU_COUNTERS;
    current.valid_fields =
        SYSTEM_SNAPSHOT_CPU_COUNTERS |
        SYSTEM_SNAPSHOT_CPU_COUNT |
        SYSTEM_SNAPSHOT_UPTIME;

    previous.cpu_total_ticks = 1000;
    previous.cpu_idle_ticks = 600;

    current.cpu_total_ticks = 1200;
    current.cpu_idle_ticks = 700;
    current.cpu_count = 8;
    current.uptime_seconds = 3600;

    assert(system_monitor_calculate(
        &previous, &current, &metrics
    ) == 0);

    assert(metrics.valid_fields & SYSTEM_METRICS_CPU_USAGE);
    assert(metrics.valid_fields & SYSTEM_METRICS_CPU_COUNT);
    assert(metrics.valid_fields & SYSTEM_METRICS_UPTIME);

    assert(!(metrics.valid_fields & SYSTEM_METRICS_MEMORY_USAGE));
    assert(!(metrics.valid_fields & SYSTEM_METRICS_LOAD_AVERAGE));

    assert(metrics.cpu_count == 8);
    assert(metrics.uptime_seconds == 3600);

    printf("[PASS] Partial metrics\n");
}

static void test_all_metrics_supported(void) {
    SystemSnapshot previous = {0};
    SystemSnapshot current = {0};
    SystemMetrics metrics = {0};

    previous.valid_fields = SYSTEM_SNAPSHOT_CPU_COUNTERS;
    current.valid_fields =
        SYSTEM_SNAPSHOT_CPU_COUNTERS |
        SYSTEM_SNAPSHOT_CPU_COUNT |
        SYSTEM_SNAPSHOT_MEMORY_TOTAL |
        SYSTEM_SNAPSHOT_MEMORY_AVAILABLE |
        SYSTEM_SNAPSHOT_LOAD_AVERAGE |
        SYSTEM_SNAPSHOT_UPTIME;

    previous.cpu_total_ticks = 1000;
    previous.cpu_idle_ticks = 600;

    current.cpu_total_ticks = 1200;
    current.cpu_idle_ticks = 700;

    current.cpu_count = 8;
    current.memory_total_kb = 8000;
    current.memory_available_kb = 3000;

    current.load_average_1 = 1.0;
    current.load_average_5 = 0.8;
    current.load_average_15 = 0.5;

    current.uptime_seconds = 3600;

    assert(system_monitor_calculate(
        &previous, &current, &metrics
    ) == 0);

    assert(metrics.valid_fields & SYSTEM_METRICS_CPU_USAGE);
    assert(metrics.valid_fields & SYSTEM_METRICS_MEMORY_USAGE);
    assert(metrics.valid_fields & SYSTEM_METRICS_CPU_COUNT);
    assert(metrics.valid_fields & SYSTEM_METRICS_LOAD_AVERAGE);
    assert(metrics.valid_fields & SYSTEM_METRICS_UPTIME);

    assert(metrics.cpu_usage == 50.0);
    assert(metrics.memory_usage == 62.5);

    printf("[PASS] All metrics supported\n");
}

int main(void) {
    test_cpu_unsupported();
    test_memory_unsupported();
    test_partial_metrics();
    test_all_metrics_supported();

    printf("\nAll system monitor tests passed.\n");

    return 0;
}
