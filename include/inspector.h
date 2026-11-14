#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <sys/types.h>
#include <stddef.h>

typedef struct {
  pid_t pid;
  pid_t ppid;
  char state;
  unsigned long vm_rss_kb;
  unsigned long vm_size_kb;
  unsigned int threads;
} ProcessSnapshot;

typedef struct {
    pid_t pid;
    unsigned long long utime_ticks;
    unsigned long long stime_ticks;
} ProcessCpuTimes;

int inspector_read_process(pid_t pid, ProcessSnapshot *snapshot);
int inspector_collect_processes(ProcessSnapshot *snapshots, size_t capacity,
                                size_t *count);
int inspector_read_cpu_times(pid_t pid, ProcessCpuTimes *cpu_times);

#endif
