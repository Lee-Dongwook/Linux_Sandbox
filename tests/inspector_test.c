#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "inspector.h"
#include "resource_monitor.h"

int main(void) {
    ProcessSnapshot snapshot;

    if (inspector_read_process(getpid(), &snapshot) == -1) {
        perror("inspector_read_process");
        return 1;
    }

    printf("PID: %d\n", snapshot.pid);
    printf("PPID: %d\n", snapshot.ppid);
    printf("State: %c\n", snapshot.state);
    printf("VmRSS: %lu kB\n", snapshot.vm_rss_kb);
    printf("VmSize: %lu kB\n", snapshot.vm_size_kb);
    printf("Threads: %u\n", snapshot.threads);

    ProcessSnapshot snapshots[1024];
    size_t count = 0;

    if (inspector_collect_processes(
        snapshots, 1024, &count) == -1) {
        perror("inspector_collect_processes");
        return 1;
    }

    printf("\nCollected processes: %zu\n", count);

    for (size_t i = 0; i < count && i < 10; i++) {
        printf(
            "PID: %d | PPID: %d | State: %c | RSS: %lu kB\n",
            snapshots[i].pid,
            snapshots[i].ppid,
            snapshots[i].state,
            snapshots[i].vm_rss_kb
        );
    }

    long clock_ticks = sysconf(_SC_CLK_TCK);

    if (clock_ticks <= 0) {
        fprintf(stderr, "Invalid clock tick value\n");
        return 1;
    }

    CpuSample previous;
    CpuSample current;

    if (resource_monitor_take_cpu_sample(getpid(), &previous) == -1) {
        perror("resource_monitor_take_cpu_sample");
        return 1;
    }

    volatile unsigned long long counter = 0;

    for (unsigned long long i = 0; i < 500000000ULL; i++) {
        counter += i;
    }

    if (resource_monitor_take_cpu_sample(getpid(), &current) == -1) {
        perror("resource_monitor_take_cpu_sample");
        return 1;
    }

    double cpu_usage;

    if (resource_monitor_calculate_cpu_usage(
        &previous,
        &current,
        clock_ticks,
        &cpu_usage
    ) == -1) {
        perror("resource_monitor_calculate_cpu_usage");
        return 1;   
    }
    
    printf("\nCPU Monitoring Result:\n");
    printf("PID: %d\n", current.pid);
    printf("Previous CPU Ticks: %llu\n", previous.total_cpu_ticks);
    printf("Current CPU Ticks: %llu\n", current.total_cpu_ticks);
    printf(
        "Delta CPU Ticks: %llu\n",
        current.total_cpu_ticks - previous.total_cpu_ticks
    );
    printf("CPU Usage: %.2f%%\n", cpu_usage);
    printf("Counter: %llu\n", counter);


    ResourceSnapshot resource_snapshot;

if (resource_monitor_collect(
        getpid(),
        &resource_snapshot
    ) == -1) {
    perror("resource_monitor_collect");
    return 1;
}

    printf("\nResource Snapshot:\n");
    printf("PID: %d\n", resource_snapshot.pid); 
    printf("State: %c\n", resource_snapshot.state);
    printf("RSS: %lu kB\n", resource_snapshot.vm_rss_kb);
    printf("Virtual Memory: %lu kB\n", resource_snapshot.vm_size_kb);
    printf("Threads: %u\n", resource_snapshot.threads);
    printf("CPU Ticks: %llu\n", resource_snapshot.cpu_ticks);
    printf("CPU Usage: %.2f%%\n", resource_snapshot.cpu_usage);


    ResourceSnapshot all_snapshots[1024];
    size_t resource_count = 0;

    if (resource_monitor_collect_all(
        all_snapshots,
        1024,
        &resource_count
    ) == -1) {
        perror("resource_monitor_collect_all");
        return 1;
    }

    printf("\nAll Resource Snapshots: %zu\n", resource_count);

    for (size_t i = 0; i < resource_count && i < 10; i++) {
        printf(
            "PID: %d | State: %c | RSS: %lu kB | "
            "Threads: %u | CPU Ticks: %llu\n",
            all_snapshots[i].pid,
            all_snapshots[i].state,
            all_snapshots[i].vm_rss_kb,
            all_snapshots[i].threads,
            all_snapshots[i].cpu_ticks
        );
    }
    return 0;
}
