#ifndef SYSTEM_INSPECTOR_H
#define SYSTEM_INSPECTOR_H

typedef enum {
  SYSTEM_SNAPSHOT_CPU_COUNTERS = 1U << 0,
  SYSTEM_SNAPSHOT_CPU_COUNT = 1U << 1,
  SYSTEM_SNAPSHOT_MEMORY_TOTAL = 1U << 2,
  SYSTEM_SNAPSHOT_MEMORY_AVAILABLE = 1U << 3,
  SYSTEM_SNAPSHOT_LOAD_AVERAGE = 1U << 4,
  SYSTEM_SNAPSHOT_UPTIME = 1U << 5
} SystemSnapshotField;

typedef struct {
  unsigned int valid_fields;

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
