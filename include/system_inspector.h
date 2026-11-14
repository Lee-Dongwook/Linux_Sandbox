#ifndef SYSTEM_INSPECTOR_H
#define SYSTEM_INSPECTOR_H

typedef struct {
  unsigned int cpu_count;
  unsigned long long cpu_total_ticks;
  unsigned long long cpu_idle_ticks;

  unsigned long memory_total_kb;
  unsigned long memory_available_kb;

  double load_average_1;
  double load_average_5;
  double load_average_15;

  unsigned long uptime_seconds;
} SystemSnapshot;

int system_inspector_collect(SystemSnapshot *snapshot);

#endif
