
#include <stdio.h>
#include <unistd.h>

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
    return 0;
}
