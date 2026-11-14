#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "inspector.h"

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

    ProcessCpuTimes previous;
    ProcessCpuTimes current;

    struct timespec start_time;
    struct timespec end_time;

    long clock_ticks = sysconf(_SC_CLK_TCK);

    if (clock_ticks <= 0) {
        fprintf(stderr, "Invalid clock tick value\n");
        return 1;
    }

    if (inspector_read_cpu_times(getpid(), &previous) == -1) {
        perror("inspector_read_cpu_times");
        return 1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1) {
        perror("clock_gettime");
        return 1;
    }

    volatile unsigned long long counter = 0;

    for (unsigned long long i = 0; i < 500000000ULL; i++) {
        counter += i;
    }

    if (inspector_read_cpu_times(getpid(), &current) == -1) {
        perror("inspector_read_cpu_times");
        return 1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end_time) == -1) {
        perror("clock_gettime");
        return 1;
    }

    double elapsed_seconds = (double)(end_time.tv_sec - start_time.tv_sec) + (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    unsigned long long previous_ticks =
        previous.utime_ticks + previous.stime_ticks;

    unsigned long long current_ticks =
        current.utime_ticks + current.stime_ticks;

    if (current_ticks < previous_ticks || elapsed_seconds <= 0.0) {
        fprintf(stderr, "Invalid CPU sampling result\n");
        return 1;
    }

    unsigned long long delta_ticks = current_ticks - previous_ticks;

    double cpu_usage = ((double)delta_ticks / (double)clock_ticks) / elapsed_seconds * 100.0; 

    
    printf("\nCPU Monitoring Result:\n");
    printf("PID: %d\n", current.pid);
    printf("Previous CPU Ticks: %llu\n", previous_ticks);
    printf("Current CPU Ticks: %llu\n", current_ticks);
    printf("Delta CPU Ticks: %llu\n", delta_ticks);
    printf("Elapsed Time: %.3f seconds\n", elapsed_seconds);
    printf("CPU Usage: %.2f%%\n", cpu_usage);
    printf("Counter: %llu\n", counter);

    return 0;
}
